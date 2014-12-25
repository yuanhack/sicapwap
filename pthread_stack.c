#include "pthread_stack.h"

pthread_attr_t g_thread_stack_attr;

int __pthread_attr_init__(pthread_attr_t * pattr)
{
    if (pattr == 0) pattr = &g_thread_stack_attr;
    return pthread_attr_init(pattr);
}

int __pthread_attr_setstacksize__(pthread_attr_t * pattr, size_t size)
{
    if (size  <= 0) size = THREAD_STACK_SIZE;
    if (pattr == 0) pattr = &g_thread_stack_attr;
    return pthread_attr_setstacksize(pattr, size);
}

