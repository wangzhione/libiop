#ifndef _H_SIMPLEC_IOP
#define _H_SIMPLEC_IOP

#include <iop_def.h>
#include <pthread.h>

//
// iop_create - 创建新的iopbase_t 对象, io模型集
// return	: iobase_t 模型,失败返回NULL
//
extern iopbase_t iop_create(void);

//
// iop_delete - 销毁iopbase_t 对象
// base		: 待销毁的io基础对象
// return	: void
//
extern void iop_delete(iopbase_t base);

//
// iop_dispatch - 启动一次事件调度
// base		: io调度对象
// return	: 本次调度处理事件总数
//
extern int iop_dispatch(iopbase_t base);

//
// iop_run - 启动循环事件调度,直到退出
// base		: io事件集基础对象
// return	: void
//
extern void iop_run(iopbase_t base);

//
// iop_run_pthread - 开启一个线程来跑这个轮询事件
// base		: iop 操作类型
// tid		: 返回的线程id指针
// return	: >=SufBase 表示成功, 否则失败
//
extern int iop_run_pthread(iopbase_t base, pthread_t * tid);

//
// iop_end_pthread - 结束一个线程的iop调度
// base		: iop 操作类型
// tid		: 返回的线程id指针
// return	: void
//
extern void iop_end_pthread(iopbase_t base, pthread_t * tid);

//
// iop_stop - 退出循环事件调度
// base		: io事件集基础对象
// return	: void
//
extern void iop_stop(iopbase_t base);

//
// iop_add - 添加一个新的事件对象到iopbase 调度事件集中
// base		: io事件集基础对象
// s		: socket 处理句柄
// ets		: 处理事件类型 EV_XXX
// to		: 超时时间, '-1' 表示永不超时
// fev		: 事件回调函数
// arg		: 用户参数
// return	: 成功返回iop的id, 失败返回SOCKET_ERROR
//
extern uint32_t iop_add(iopbase_t base, socket_t s, uint32_t ets, uint32_t to, iop_event_f fev, void * arg);

// 
// iop_del - iop销毁事件
// base		: io事件集基础对象 
// id		: iop事件id
// return	: >=0 成功, <0 表示失败
//
extern int iop_del(iopbase_t base, uint32_t id);

//
// iop_mod - 修改iop事件订阅事件
// base		: io事件集基础对象 
// id		: iop事件id
// events	: 新的events事件
// return	: >=0 成功, <0 表示失败
//
extern int iop_mod(iopbase_t base, uint32_t id, uint32_t events);

// iop 轮询事件的发送接收操作, 发送没变化, 接收放在接收缓冲区
extern int iop_send(iopbase_t base, uint32_t id, const void * data, uint32_t len);
extern int iop_recv(iopbase_t base, uint32_t id);

#endif // !_H_SIMPLEC_IOP
