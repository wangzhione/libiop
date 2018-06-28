#ifndef _H_SIMPLEC_IOP_POLL
#define _H_SIMPLEC_IOP_POLL

#include <iop_def.h>

//
// iop_poll_init - 通信的底层接口
// base		: 总的iop对象管理器
// maxsz	: 开启的最大处理数
// return	: SBase 表示成功
//
extern int iop_poll_init(iopbase_t base, unsigned maxsz);

#endif // !_H_SIMPLEC_IOP_POLL