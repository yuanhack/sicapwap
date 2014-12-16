#include "sino_comm.h"
#include "read_conf.h"
#include "yhepoll.h"
#include "CWAC.h"

#include <sys/stat.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <pthread.h>

pthread_mutex_t sinomutex = PTHREAD_MUTEX_INITIALIZER;
sino_data  sino;

int sino_lock()
{
    return pthread_mutex_lock(&sinomutex);
}
int sino_unlock()
{
    return pthread_mutex_unlock(&sinomutex);
}

void sino_data_init(sino_data *sino)
{
    memset(sino->sino_buff, 0, SINOIX_MAX_SIZE);
    sino->head = (sino_head*)sino->sino_buff;
    sino->offset = 0;
    sino->pos    = -1;
    sino->head->len = 0;
}

// init sino_head
int sino_assemble_head(void* buff, int size) {
    if (buff == 0 || size < sizeof(sino_head)) return 0;
    sino_head* head = buff;
    head->magic = htonl(SI_MAGIC);
    head->elems = htons(0);
    head->len   = htons(0);
    return sizeof(sino_head);
}

// sino_data call API
int sino_data_head(sino_data *sino)
{
    sino->offset = sino_assemble_head(sino->sino_buff, sizeof(sino->sino_buff));
    return sino->offset;
}

// return assembled datas length (sino_elem + info-len) 
static int sino_assemble_data(void* buff, int size, int type, uchar* info, int len) 
{
    if (buff == 0 || size < sizeof(sino_elem) + len) return 0;
    sino_elem* data = buff;
    data->type = htons(type);
    data->len  = htons(len);
    memcpy(data->info, info, len);
    return sizeof(sino_elem) + len;
}
// sino_data call API
sino_elem* sino_data_push(sino_data *sino, int type, uchar *info, int len) 
{
    int asslen = 0;
    if (!(asslen = sino_assemble_data(sino->sino_buff + sino->offset, 
                    sizeof(sino->sino_buff) - sino->offset, type, info, len)))
        return 0;
    sino->offset     += asslen;
    sino->head->len   = htons(ntohs(sino->head->len) + len + sizeof(sino_elem));
    sino->head->elems = htons(ntohs(sino->head->elems) + 1);
    /*printf("\tAslen %d, offset %d, elems %d, count len %d\n", 
      asslen, sino->offset, ntohs(sino->head->elems), ntohs(sino->head->len));*/
    return (sino_elem*)((void*)sino->sino_buff + sino->offset - asslen);
}
// sino_data call API
sino_elem* sino_data_elem_push(sino_data *sino, sino_elem *elem) 
{
    return sino_data_push(sino, elem->type, elem->info, elem->len);
}

// sino_data call API
void sino_data_elem_pop_init(sino_data *sino)
{
    sino->pos = sizeof(sino_head); // pos -> next elem
    //sino_elem *elem = (void*)sino->head + sino->pos; // first elem
}
// sino_data call API
sino_elem * sino_data_elem_pop(sino_data *sino)
{
    if (sino->pos <= 0) return 0;
    sino_elem *now = (void*)sino->head + sino->pos;
    sino->pos = sino->pos + sizeof(sino_elem) + ntohs(now->len); // next pos
    return now;
}
sino_elem * sino_data_elem_pop_first(sino_data *sino)
{
    sino_data_elem_pop_init(sino);
    return sino_data_elem_pop(sino);
}

/* total elems lengths  */
int sino_total_elem_len(sino_head* head) {
    int i, len = 0, elems = 0;
    if (head == 0) return 0;
    sino_elem *data = (void*)head + sizeof(sino_head);
    elems = ntohs(head->elems);
    for (i = 0; i < elems; ++i) {
        len += sizeof(sino_elem) + ntohs(data->len);
        data = (void*)data + sizeof(sino_elem) + ntohs(data->len); 
    }
    return len;  
}
/*
 */
int sino_fill_head_len(sino_head* head) 
{
    head->len = htons(sino_total_elem_len(head));
    return ntohs(head->len); 
}
int sino_elem_size(sino_head* head) 
{
    return ntohs(head->len);
}
int sino_packet_size(sino_head* head) 
{
    return sizeof(sino_head) + sino_elem_size(head);
}

void pack_net_stat(packet_stat *pack, int stat, char *ver, char *mac, char *ip, char *ifname)
{
    pack->magic = htonl(SS_MAGIC);
    pack->stat  = htons(stat);
    snprintf(pack->ver,     sizeof(pack->ver), "%s", ver);
    snprintf(pack->mac,     sizeof(pack->mac), "%s", mac);
    snprintf(pack->ip,      sizeof(pack->ip),  "%s", ip);
    snprintf(pack->ifname,  sizeof(pack->ifname),  "%s", ifname);
}
void pack_net_to_local(packet_stat *net_pack, packet_stat *local_pack)
{
    local_pack->magic = ntohl(net_pack->magic);
    local_pack->stat  = ntohs(net_pack->stat);
    snprintf(local_pack->ver,   sizeof(local_pack->ver), "%s", net_pack->ver);
    snprintf(local_pack->mac,   sizeof(local_pack->mac), "%s", net_pack->mac);
    snprintf(local_pack->ip,    sizeof(local_pack->ip),  "%s", net_pack->ip);
    snprintf(local_pack->ifname,sizeof(local_pack->ifname),  "%s", net_pack->ifname);
}



// Function in CWVendorPayloadsAC.c file
//CWBool CWAssembleMsgSinoixVendorPayload(CWProtocolMessage *msgPtr, sino_head *sino);
// 20141111 Yuan Hong 
