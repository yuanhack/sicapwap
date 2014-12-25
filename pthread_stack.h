#ifndef __YH_PTHREAD_STACK_H__
#define __YH_PTHREAD_STACK_H__

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#ifdef __cplusplus
extern "C"
{
#endif

/* thread's stack size */
#define THREAD_STACK_SIZE               1024 * 1024

pthread_attr_t    g_thread_stack_attr;

int __pthread_attr_init__ (pthread_attr_t * pattr);
int __pthread_attr_setstacksize__ (pthread_attr_t * pattr, size_t size);

// link -lpthread

#ifdef __cplusplus
}
#endif
#endif /* __YH_PTHREAD_STACK_H__ */
