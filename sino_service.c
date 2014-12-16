#include "sino_service.h"
#include "sino_comm.h"
#include "read_conf.h"
#include "yhepoll.h"
#include "yhpath.h"


#include "CWAC.h"

#include <sys/stat.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <pthread.h>

#include <stdio.h>
#include <sqlite3.h>

#define SINO_STAT_PORT        12345
#define TIMEOUT_SEC           10

#define TID                   (unsigned)pthread_self()
#define FIRMWARE_UPGRADE_INFO "firmware_upgrade_log"
#define CREATE_DB_TAB(s) "create table "s\
        "(mac varchar(20),interface verchar(16),addr verchar(46),"\
        "version verchar(20),stat int,date verchar(14));"

sqlite3 * db = 0;

epoll_manager *em;                      // epoll manage handler

struct list ls = LIST_HEAD_INIT(ls);

int create_socket_v4_server_for_addr                                                                       
    (int type, const struct sockaddr *addr, socklen_t alen, int qlen)
{
    int is;
    int reuse = 1;

    if ((is = socket(addr->sa_family, type, 0)) < 0)
        goto errquit;

    /* 
     * set SO_REUSEADDR for server socket 
     */
    if (setsockopt(is, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(int)) < 0)
        goto errquit;

    if (bind(is, addr, alen) < 0) 
        goto errquit;   

    if (type == SOCK_STREAM || type == SOCK_SEQPACKET) 
    {
        if (listen(is, qlen) < 0)
            goto errquit;
    }
    return is;
errquit:
    close(is);
    return -1;
}

int create_socket_v4_server(int type, char* ip, int port, int listenlen)
{
    struct sockaddr_in addr;

    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr(ip);

    return create_socket_v4_server_for_addr
        (type, (struct sockaddr*)&addr, sizeof(struct sockaddr), listenlen);
}

void client_disc(fd_event *fe)
{
    wtpc *pc = fe->ptr;
    if (pc == 0)  return; 
    list_remove(&pc->node);
    free(fe->ptr); fe->ptr = 0; 
    wtps--;
    notify_em_fd_event_release(fe,1);
}

char * fmt_time(char *p, size_t len)
{
    time_t timee;
    struct tm tmm;

    timee = time(0);
    localtime_r(&timee, &tmm);
    strftime(p, len, "%Y%m%d%H%M%S", &tmm);
    return p;
}

#define CHECK_DATA   \
    if (local.magic != SS_MAGIC) {\
        CWLog("receives fd %d invalid %d bytes, magic check error, discard. %s %d", fd, pc->len, __FILE__, __LINE__);\
        pc->len = 0;\
        return;\
    }
void client_in_nonblock_et(fd_event *fe)
{
    int fd = fe->fd;
    char buff[sizeof(packet_stat)] = {0};
    int n;

    packet_stat *netpack = 0;
    packet_stat local;
    wtpc        *pc   = (wtpc*)fe->ptr;
    char sql[256];
    char *err;

    if (pc == 0)  return; 
eintr:
    while ((n = read(fd, buff, sizeof(buff))) > 0) {
        pc->len += n;
        if (n != sizeof(packet_stat)) {
            pc->time = time(0);
            netpack = (packet_stat*)pc->buf;
            memcpy(pc->buf+pc->len, buff, n);
            if (pc->len >= sizeof(packet_stat)) {
                pack_net_to_local(netpack, &local);
                CHECK_DATA; //  break if failed;
                CWLog("Get joins packet, fd %d info: %d, %s, %s, %s, %s\n", 
                        fd, local.stat, local.ver, local.mac, local.ip, local.ifname);
                goto __insert_db;
            }
        } else {
            netpack = (packet_stat*)buff;
            pack_net_to_local(netpack, &local);
            CHECK_DATA; // break if failed
            CWLog("Get full packet, fd %d info: %d, %s, %s, %s, %s\n", 
                    fd, local.stat, local.ver, local.mac, local.ip, local.ifname);
            goto __insert_db;
        }
    }
    if (n < 0) {
        if (errno == EAGAIN) return;
        if (errno == EINTR)  goto eintr;
        CWLog("client [%05d] read ret %d to error %d:%s\n", fd, n, errno, strerror(errno));
        client_disc(fe);
        return;
    } else if (n == 0) {
        CWLog("client [%05d] read ret %d to close\n", fd, n);
        client_disc(fe);
        return;
    }
    if (pc->len == 0) {
        CWLog("Clean up the wrong date client [%05d]\n", fd);
        client_disc(fe);
    }
    return;
    char stime[15];
__insert_db:
    fmt_time(stime, sizeof(stime));
    snprintf(sql, sizeof(sql),"insert into %s(mac, version, stat, date, interface, addr) "
            "values('%s', '%s', '%d', '%s', '%s', '%s');", FIRMWARE_UPGRADE_INFO,
            local.mac, local.ver, local.stat, stime, local.ifname, local.ip);
    if (sqlite3_exec(db, sql, 0, 0, &err) != SQLITE_OK) {
        CWLog("%s %s %d insert err: %s", local.mac, local.ver, local.stat, err);
        sqlite3_free(err);
    } else 
        CWLog("%s %s %d", local.mac, local.ver, local.stat);
    client_disc(fe);
    return;
}
void client_err(fd_event *fe)
{
    CWLog("client %d client_err\n", fe->fd);
    client_disc(fe);
}
void client_hup(fd_event *fe)
{
    CWLog("client %d client_hup\n", fe->fd);
    client_disc(fe);
}

void client_conn(epoll_manager *em, int fd)
{
    if (setfd_nonblock(fd) < 0) { close(fd); return; }

    fd_event_handle fe = fd_event_new();
    if (fe == 0) { 
        CWLog("Add online client fd %d fd_event_new error: %s", fd, strerror(errno));
        close(fd); 
        return; 
    }

    fd_event_init(fe, em, fd);

    wtpc *pc = calloc(1, sizeof(wtpc));
    if (!pc) {
        CWLog("Add online client fd %d calloc error: %s", fd, strerror(errno));
        close(fd);
        free(fe);
        return;
    }
    //printf("alloc %p\n", pc);

    fd_event_set(fe, EPOLLET, 0);
    fd_event_set(fe, EPOLLIN, client_in_nonblock_et);

    if (notify_em_fd_event_add(fe) < 0) {
        CWLog("Add online client fd %d failed", fd);
        close(fd);
        free(fe); 
        free(pc); 
        return;
    }

    fe->ptr  = pc;
    pc->time = time(0);
    pc->fe = fe;

    list_append(&ls, &pc->node); // insert into wtpc link
    ++wtps;

    CWLog("fd %d online time %ld", fe->fd, pc->time);
    //printf("fd %d online time %ld\n", fe->fd, pc->time);
}

void tcp_server_in(fd_event* fe)
{
    int fd;
    while (1) {
        fd = accept(fe->fd, 0, 0);
        if (fd < 0) {
            if (errno != EAGAIN) CWLog("accept error: %s", strerror(errno));
            return;                             
        }
        //printf("server [%05d] accept ret: %d\n", fe->fd, fd);
        client_conn(fe->em, fd);
    }
}

void  cb_epoll_after(const epoll_manager * const em)
{
    wtpc *pc;
    time_t now = time(0);
    
    if (wtps <= 0) return;

    list_for_each_entry(pc, wtpc, &ls, node) {
        if (now - pc->time < TIMEOUT_SEC) break; // later all no timeout
        CWLog("wtps %d, fd %d time[%ld-%ld] is timeout.", wtps, pc->fe->fd, pc->time, now);
        //printf("wtps %d, time[%ld-%ld] is timeout.\n", wtps, pc->time, now);
        client_disc(pc->fe);
    }
}

//void sino_epoll_thread()
void sino_epoll_open()
{
    pthread_t tid = pthread_self();
    pthread_detach(tid);

    em = Em_open(999, 1000, 0, 0, cb_epoll_after); // 1000 milliseconds
    Em_run(em);
    CWLog("Startup Epoll Thread %x done", (unsigned)em->tid);

    int tcpsfd = create_socket_v4_server(SOCK_STREAM, (char*)"0.0.0.0", SINO_STAT_PORT, 1000);
    if (tcpsfd < 0) {
        CWLog("create_socket_v4_server tcp error: %s\n", strerror(errno));
        //printf("create_socket_v4_server tcp error: %s\n", strerror(errno));
        exit(1);
    }
    CWLog("Image Status Receive Server Create Success: 0.0.0.0:%d TCP", SINO_STAT_PORT);

    fd_event_handle festcp = fd_event_handle_new();
    fd_event_init(festcp, em, tcpsfd);
    fd_event_set(festcp, EPOLLIN, tcp_server_in);
    fd_event_set(festcp, EPOLLET, 0);
    setfd_nonblock(tcpsfd);
    if (notify_em_fd_event_add(festcp) < 0) {
        CWLog("Image Status Receive server Join Epoll failed: %s", strerror(errno));
        exit(1);
    }
    CWLog("Image Status Receive server Join Epoll Success");
}
void db_open()
{
    char path[1024], errinfo[1024];
    if (propath(path, sizeof(path), errinfo, sizeof(errinfo)) < 0) {
        CWLog("%s", errinfo);
        exit(1);
    }
    strcat(path, "/");
    strcat(path, "wtps.db");

    int ret;

    if ((ret = sqlite3_open(path, &db)) != SQLITE_OK) {
        CWLog(": %s", sqlite3_errmsg(db));
        exit(1);
    }
    ret = sqlite3_exec(db, CREATE_DB_TAB(FIRMWARE_UPGRADE_INFO), 0, 0, 0);
    if (ret != SQLITE_OK && ret != SQLITE_ERROR) {
        CWLog("ret %d: %s", ret, sqlite3_errmsg(db));
        //printf("ret %d: %s\n", ret, sqlite3_errmsg(db));
        exit(1);
    }
    CWLog("connect %s success\n", path);
}
void sino_service_start()
{
    CWLog("Sinoix Service Startup ...");
    db_open();
    sino_epoll_open();
    CWLog("Sinoix Service startup done");
}

