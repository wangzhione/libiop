#ifndef _H_IOP_LIBIOP
#define _H_IOP_LIBIOP

#include <iop_poll.h>

//
// iop_create - 创建新的 iopbase_t 对象, io 调度对象
// return   : 失败返回 NULL
//
extern iopbase_t iop_create(void);

//
// iop_delete - 销毁 iopbase_t 对象
// base     : 待销毁的 io 调度对象
// return   : void
//
extern void iop_delete(iopbase_t base);

//
// iop_dispatch - 启动一次事件调度
// base     : io 调度对象
// return   : 本次调度处理事件总数
//
extern int iop_dispatch(iopbase_t base);

//
// iop_add - 添加一个新的事件对象到 iopbase 调度对象集中
// base     : io 调度对象
// s        : socket 处理句柄
// events   : 处理事件类型 EV_XXX
// to       : 超时时间, '-1' 表示永不超时
// fevent   : 事件回调函数
// arg      : 用户参数
// return   : 成功返回 iop 的 id, 失败返回 SOCKET_ERROR
//
extern uint32_t iop_add(iopbase_t base, 
    socket_t s, uint32_t events, uint32_t to, iop_event_f fevent, void * arg);

//
// iop_del - iop 销毁事件
// base     : io 调度对象
// id       : iop 事件 id
// return   : >=0 成功, <0 表示失败
//
extern int iop_del(iopbase_t base, uint32_t id);

//
// iop_mod - 修改iop事件订阅事件
// base     : io事件集基础对象 
// id       : iop事件id
// events   : 新的 events 事件
// return   : >= SBase 成功. 否则 表示失败
//
extern int iop_mod(iopbase_t base, uint32_t id, uint32_t events);

// iop_xxxx 轮询事件的发送接收操作, 发送没变化, 接收放在接收缓冲区
extern int iop_send(iopbase_t base, uint32_t id, const void * data, uint32_t len);
extern int iop_recv(iopbase_t base, uint32_t id);

#endif//_H_IOP_LIBIOP
