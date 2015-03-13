#include "sino_db.h"
#include "CWAC.h"

#include <pthread.h>

char db_ip[256] = "172.10.10.145";
int  db_port = 3306;
char db_user[64] = "root";
char db_pass[64] = "acmanage";
char db_name[64] = "ac";
MYSQL * mysql;

pthread_mutex_t __db_mutex = PTHREAD_MUTEX_INITIALIZER;

int db_lock()
{
    return pthread_mutex_lock(&__db_mutex);
}
int db_unlock()
{
    return pthread_mutex_unlock(&__db_mutex);
}


int mysql_initialize()
{
    if (mysql_library_init(0,0,0) == 0)
        return 0; 
    else 
        return -1;
}
void mysql_uninit()
{
	mysql_library_end();
}

static inline
char * fmt_time(char *p, size_t len)
{
    time_t timee;
    struct tm tmm;

    timee = time(0);
    localtime_r(&timee, &tmm);
    strftime(p, len, "%Y-%m-%d %H:%M:%S", &tmm);
    return p;
}

int mysql_conn()
{
    if ( !(mysql = mysql_init((MYSQL*)0)) ) {
        CWLog("mysql init error");
    }
    mysql_options(mysql, MYSQL_SET_CHARSET_NAME, "utf8");
    if (!(mysql_real_connect(mysql, db_ip, db_user, db_pass, db_name, db_port, 
                    NULL, CLIENT_LOCAL_FILES))) {
        CWLog("mysql connect error !!!");
        return -1;
    }
    CWLog("mysql connect succ");

    char sql[1024];
    int  n;
    // ALTER TABLE `info_ap` ADD unique(`mac`);
    n = snprintf(sql, sizeof(sql), "ALTER TABLE `info_ap` ADD unique(`mac`);");
    if (mysql_real_query(mysql, sql, n)) {
        CWLog("Mysql info_ap Set mac unique failed");
        exit(1);
    }
	return 0;
}

void mysql_disconn()
{
    mysql_close(mysql);
}

int ap_online(const char *mac)
{
    char sql[1024], time[32];
    int n;

    fmt_time(time, sizeof(time));

    n = snprintf(sql, sizeof(sql), 
            "INSERT INTO "
            " info_ap(mac,onlinetime) "
            " VALUES('%s', '%s') "
            "ON DUPLICATE KEY UPDATE onlinetime=values(onlinetime)"
            , mac, time);
    printf("%s\n", sql);
    db_lock();
    if (mysql_real_query(mysql, sql, n)) {
        db_unlock();
        CWLog("update online fail %s %s", mac, time);
        return -1;
    } else {
        CWLog("update online succ %s %s", mac, time);
    }
    db_unlock();
     
	return 0;
}
int ap_offline(const char *mac)
{
    char sql[1024], time[32];
    int n;

    fmt_time(time, sizeof(time));

    n = snprintf(sql, sizeof(sql), "UPDATE info_ap SET offlinetime='%s' WHERE mac='%s'"
            , time, mac);
    db_lock();
    if (mysql_real_query(mysql, sql, n)) {
        db_unlock();
        CWLog("update offline fail %s %s", mac, time);
        return -1;
    } else {
        CWLog("update offline succ %s %s", mac, time);
    }
    db_unlock();
     
	return 0;
}
