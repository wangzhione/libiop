#ifndef _H_IOP_SERVER_LIBIOP
#define _H_IOP_SERVER_LIBIOP

#include <iop.h>

typedef struct iops * iops_t;

//
// iops_run - 启动一个 iop tcp server, 开始监听处理
// host         : 服务器地址
// timeout      : 超时时间阀值
// fparser      : 协议解析器
// fprocessor   : 数据处理器
// fconnect     : 当连接创建时候回调
// fdestroy     : 退出时间的回调
// ferror       : 错误的时候回调
// return       : 返回 iops_t 对象, 调用 iops_end 同步结束
//
extern iops_t iops_run(const char * host, uint32_t timeout,
                       iop_parse_f fparser, iop_processor_f fprocessor,
                       iop_f fconnect, iop_f fdestroy, iop_event_f ferror);

//
// iops_end - 结束一个 iops 服务
// iops     : iops_run 返回的对象
// return   : void
//
extern void iops_end(iops_t iops);

#endif // !_H_IOP_SERVER_LIBIOP
