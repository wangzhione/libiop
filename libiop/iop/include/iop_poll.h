#ifndef _H_SIMPLEC_IOP_POLL
#define _H_SIMPLEC_IOP_POLL

#include <iop_def.h>

//
// iop_init_pool - 通信的底层接口
// base		: 总的iop对象管理器
// maxsz	: 开启的最大处理数
// return	: SufBase 表示成功
//
extern int iop_init_pool(iopbase_t base, unsigned maxsz);

#endif // !_H_SIMPLEC_IOP_POLL