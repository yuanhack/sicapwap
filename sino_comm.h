#ifndef __SINOIX_INFO_H_YH__
#define __SINOIX_INFO_H_YH__

#include <stdint.h>
#include <netinet/in.h>

typedef unsigned char uchar;

// Updata process config 
#define TFTPBOOT        "/tftpboot"
#define REPORT_IF_NAME  "report-if.name"

// 固件下载保存路径
#define FIRMWARE_IMAGE_PATH "/tmp/firmware_image"
// 固件下载成功后信息保存文件
#define FIRMWARE_IMAGE_INFO "/tmp/firmware_image.info"

// 记录固件当前版本的文件，由固件制作时指定
#define FIRMWARE_VERSION    "/etc/firmware_version"

/* Vendor Specific Payload format
   ---------------------------------------------------
   sino_head + sino_elem(n)
   --------------------------------------------------- */

enum element_types {
    SI_SUCCESS  = 0,
    SI_ERROR    = 1,
    SI_SSID = 2,
    SI_CHANNEL,
    SI_PASSWD,

    // firmware image info
    SI_FI_VERSION, 
    SI_FI_MD5,     
    SI_FI_LEN,    // file size 
    SI_TFTP_ADDR,   // ip or domain
    SI_TFTP_PORT,   // default udp port 69
    SI_REPT_ADDR,   // ip or domain
    SI_REPT_PORT,   // default udp port 69
};

#pragma pack(push)
#pragma pack(1)

#define SINOIX_MAX_SIZE 2048

#define SI_MAGIC 0x89abcdef
// network transmission use
typedef struct {
    uint32_t  magic; // SI_MAGIC, Vendor Identifier
    uint16_t  elems; // elem's number
    uint16_t  len;   // elem's length
} sino_head;
typedef struct {
    uint16_t  type; 
    uint16_t  len;
    uchar     info[];   // uint_least8_t
} sino_elem;

// local process use
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
    UPSTAT_ERR_TFTP,    // tftp下载时错误
    UPSTAT_ERR_MD5,     // md5校验错误,可能md5码错误或文件下载不完整
    UPSTAT_ERR_SAVEINFO,// 正确下载固件和校验成功,但保存信息文件时出错,例如写文件错误
    UPSTAT_ERR_LACK_SPACE,// 下载固件前时检查/tmp下空间不足
};
#define SS_MAGIC 0xffffaaaa
typedef struct {
    uint32_t  magic;   // SS_MAGIC
    uint16_t  stat;    // status code
    char      ver[32]; // version number
    char      ifname[16];
    char      mac[18]; // ff:ff:ff:ff:ff:ff len 6*2(ff) + 5(:)  + 1(null)
    char      ip[INET6_ADDRSTRLEN];  // INET6_ADDRSTRLEN 46
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

void pack_net_stat(packet_stat *pack, int stat, char *ver, char *mac, char *ip, char *ifname);
void pack_net_to_local(packet_stat *net_pack, packet_stat *local_pack);

#endif /* __SINOIX_INFO_H_YH__ */
