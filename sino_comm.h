#ifndef __SINOIX_INFO_H_YH__
#define __SINOIX_INFO_H_YH__

#include <stdint.h>
#include <netinet/in.h>

typedef unsigned char uchar;

/* WTP's updata module */
#define TFTPBOOT "/tftpboot"

/*
   Vendor Specific Payload format
   ---------------------------------------------------
   HEAD fields
   head field 1: Magic head 0x789A (uint16_t)
   head field 2: Element number (uint16_t)
   head field 3: Datas length (uint16_t)
   ---------------------------------------------------
   Data fields (struct sino_comm)
   element 1 	: sino_elem 1
   element 2 	: sino_elem 2
   .	: sino_elem .
   element n 	: sino_elem n
   ----------------------------------------------------
 */

enum element_types {
    //SINO_SUCCESS  = 0,
    //SINO_ERROR    = 1,
    SI_SSID = 2,
    SI_CHANNEL,
    SI_PASSWD,
    SI_WTP_VERSION, 
    //SI_WTP_SHA1,    // 
    SI_WTP_MD5,     // 
    SI_WTP_LEN,    // file size 
    SI_TFTP_ADDR,   // ip or domain
    SI_TFTP_PORT,   // default udp port 69
};

#pragma pack(push)
#pragma pack(1)

#define SINOIX_MAX_SIZE 2048

#define SI_MAGIC 0x89abcdef
typedef struct {
    uint32_t  magic; // SI_MAGIC, Vendor Identifier
    uint16_t  elems;
    uint16_t  len;
} sino_head;

typedef struct {
    uint16_t  type; 
    uint16_t  len;
    uchar     info[];   // uint_least8_t
} sino_elem;

typedef struct {
    uint16_t  offset; // Local byte order, local use only
    uint16_t  pos;
    sino_head *head;  // Pointer to the .sino_buff
    char      sino_buff[SINOIX_MAX_SIZE];
} sino_data;

extern sino_data  sino;

enum {
    DOWN_STAT_DOWNING,
};

enum {
    UPSTAT_SUC,         // 下载完成
    UPSTAT_ING,         // 开始或者下载中
    UPSTAT_ERR_TFTP,    // tftp下载错误
    UPSTAT_ERR_MD5,     // md5校验错误,可能md5码错误或文件下载不完整
    //UPSTAT_ERR_LEN,     // 下载后文件长度不对
};
#define SS_MAGIC 0xffffaaaa
typedef struct {
    uint32_t  magic;   // SS_MAGIC
    uint16_t  stat;    // status code
    char      ver[32]; // version number
    char      mac[18]; // ff:ff:ff:ff:ff:ff len 6*2(ff) + 5(:)  + 1(null)
    char      ip[INET6_ADDRSTRLEN*2];  // INET6_ADDRSTRLEN 46
} packet_stat;
#pragma pack(pop)

int sino_assemble_head(void* buff, int size);
//int sino_assemble_data(void* buff, int size, int type, uchar* info, int len);
int sino_fill_head_len(sino_head* head);
int sino_elem_size(sino_head* head);
int sino_packet_size(sino_head* head);
int sino_total_elem_len(sino_head* head);

void        sino_data_init(sino_data *sino);
int         sino_data_head(sino_data *sino);
sino_elem*  sino_data_push(sino_data *sino, int type, uchar *info, int len);
void        sino_data_elem_pop_init(sino_data *sino);
sino_elem*  sino_data_elem_pop(sino_data *sino);
sino_elem * sino_data_elem_pop_first(sino_data *sino); // init + pop 1 times

int sino_lock();
int sino_unlock();

void sino_service_start();

void pack_net_stat(packet_stat *pack, int stat, char *ver, char *mac, char *ip);
void pack_net_to_local(packet_stat *net_pack, packet_stat *local_pack);

#endif /* __SINOIX_INFO_H_YH__ */
