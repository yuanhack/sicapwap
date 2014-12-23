#ifndef __YH_READ_CONF_H__
#define __YH_READ_CONF_H__

#ifdef __cplusplus
extern "C"
{
#endif

#pragma pack(push)
#pragma pack(1)

#pragma pack(pop)

    char *jump_space (char *p);
    char *strstr_after(const char *haystack, const char *needle);
    void set_item_i(const char *buff, const char *item, int *pin);
    void set_item_l(const char *buff, const char *item, long *plo);
    void set_item_d(const char *buff, const char *item, double *pdo);
    void set_item_s(const char *buff, const char *item, char *ps);

    void set_line_comment_symbol(const char *symbol);

    int stritem_usable(const char *linehead, const char *item, const char *sym);
    char * next_line(const char *p); 
    char * fine_first_line_usable(const char *block, const char *item, const char *sym);

    int read_conf(const char *conf, const char *comm_sym);

    char tftp_locat[1024]; // for firmware_image.info tftp_location=addr:port
    char firmware_version[65]; // for firmware.info firmware_version=x.x
    char firmware_md5[65];    // 128bit -> 32, 256bit->64
    char firmware_len[10];    // 4294967296 4096M 4G
    char tftp_addr[1000]; 
    int  tftp_port;
    char rept_locat[1024]; // 上报服务地址 rept_location=addr:port
    char rept_addr[1000];  // ip 或 域名
    int  rept_port;        // 端口

#ifdef __cplusplus
}
#endif
#endif /* __YH_READ_CONF_H__ */
