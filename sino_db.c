#include "sino_db.h"

char db_ip[256];
int  db_port;
char db_user[64];
char db_pass[64];
char db_name[64];
MYSQL * mysql;

int mysql_initialize()
{
	mysql_library_init(0,0,0);
	return 0; 
}
int mysql_uninit()
{
	mysql_library_end();
	return 0; 
}

int mysql_conn()
{
	return 0;
}

int ap_online()
{
	return 0;
}
int ap_offline()
{
	return 0;
}