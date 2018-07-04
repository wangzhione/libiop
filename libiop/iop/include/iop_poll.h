#ifndef _H_IOP_POLL_LIBIOP
#define _H_IOP_POLL_LIBIOP

#include <iop_def.h>

//
// iop_poll_init - 通信的底层初始化操作
// base     : 总的 iop 对象基础管理器
// return   : SBase 表示成功
//
extern int iop_poll_init(iopbase_t base);

#endif //_H_IOP_POLL_LIBIOP
