#ifndef _H_THREAD
#define _H_THREAD

#include "struct.h"
#include <pthread.h>
#include <semaphore.h>

//
// pthread_end - 等待启动线程结束
// tid      : 线程id
// return   : void
//
inline void pthread_end(pthread_t tid) {
    pthread_join(tid, NULL);
}

//
// pthread_run - 异步启动线程
// id       : &tid 线程id地址
// frun     : 运行的主体
// arg      : 运行参数
// return   : 返回线程构建结果, 0 is success
//
#define pthread_run(id, frun, arg)                                  \
pthread_run_(&(id), (node_f)(frun), (void *)(intptr_t)(arg))
inline int pthread_run_(pthread_t * id, node_f frun, void * arg) {
    return pthread_create(id, NULL, (start_f)frun, arg);
}

//
// pthread_async - 异步启动分离线程
// frun     : 运行的主体
// arg      : 运行参数
// return   : 返回 0 is success
// 
#define pthread_async(frun, arg)                                    \
pthread_async_((node_f)(frun), (void *)(intptr_t)(arg))
inline int pthread_async_(node_f frun, void * arg) {
    int ret;
    pthread_t tid;
    pthread_attr_t attr;

    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    ret = pthread_create(&tid, &attr, (start_f)frun, arg);
    pthread_attr_destroy(&attr);

    return ret;
}

#endif//_H_THREAD
