#ifndef __YH_SINO_DB_H__
#define __YH_SINO_DB_H__

#include <mysql/mysql.h>

#ifdef __cplusplus
extern "C"
{
#endif

extern char db_ip[256];
extern int  db_port;
extern char db_user[64];
extern char db_pass[64];
extern char db_name[64];
extern MYSQL * mysql;

int mysql_initialize();
void mysql_uninit();

int mysql_conn();
void mysql_disconn();

int ap_online();
int ap_offline();


#ifdef __cplusplus
}
#endif
#endif /* __YH_SINO_DB_H__ */
