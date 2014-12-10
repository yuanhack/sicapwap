#include "yhpath.h"


// 该函数获取到的目录路径不是 '/' 结尾的
int propath(char *buff, int len, char *errinfo, int errlen)
{
	char *p, *path; 
	int ret; 

    int syslen = sysconf(_PC_PATH_MAX);

    path = calloc(syslen, sizeof(char));
    if (path == 0) {
        snprintf(errinfo, errlen, "propath(), calloc() error: %s", strerror(errno));
        return -1;
    }
    memset(buff, 0, len);
    memset(errinfo, 0, errlen);

	ret = readlink("/proc/self/exe", path, syslen);
    if (ret < 0){
        snprintf(errinfo, errlen, "propath(), readlink() error: %s", strerror(errno));
        return -1;
    } else if (ret == syslen-1) { 
        // 如果缓冲满了, 缓冲path中被填充的内容大小为 syslen - 1
        // readlink不会造成溢出, 但可能会造成path放不下剩余内容, 造成路径不足
        snprintf(errinfo, errlen, "propath() call readlink(): buffer is too small");
        return -1;
    }

	p = strrchr(path, '/');  // 找到 path 中的最后一个 '/'
    *p = 0;  

    if (strlen(path) >= len) {
        snprintf(errinfo, errlen, "propath() failed: buffer is too small");
        printf("len is too small\n");
        return -1;
    }

	strncpy(buff, path, len);
	return strlen(buff);
}

//#include<stdio.h>
//#include<limits.h>
//#include<unistd.h>
//int main()
//{
//    printf("%ld\n", sysconf(_PC_PATH_MAX));
//    return 0;
//}
