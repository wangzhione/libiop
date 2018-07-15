#ifndef _H_IOP_POLL_LIBIOP
#define _H_IOP_POLL_LIBIOP

#include <iop_def.h>

//
// iop_poll - 为 iop base 对象注入 poll 处理行为
// base     : 总的 iop 对象基础管理器
// return   : SBase 表示成功
//
extern int iop_poll(iopbase_t base);

#endif //_H_IOP_POLL_LIBIOP
