#ifndef __YH_UPDATE_STATUS_H__
#define __YH_UPDATE_STATUS_H__


typedef enum {
    WTP_UP_SUC, // 更新成功
    WTP_UP_ING, // 更新中
    WTP_UP_ERR_DOWN, // 下载时出错
    WTP_UP_ERR_MD5, // 校验时出错
} wtp_update_stat_code;


#endif /* __YH_UPDATE_STATUS_H__ */
