#ifndef _H_SIMPLEC_IOP_SERVER
#define _H_SIMPLEC_IOP_SERVER

#include <iop.h>

typedef struct iops * iops_t;

//
// iops_run - 启动一个 iop tcp server, 开始监听处理
// host         : 服务器ip
// port         : 服务器端口
// timeout      : 超时时间阀值
// fparser      : 协议解析器
// fprocessor   : 数据处理器
// fconnect     : 当连接创建时候回调
// fdestroy     : 退出时间的回调
// ferror       : 错误的时候回调
// return       : 返回 iops_t 对象, 结束时候可以调用 iops_end
//
extern iops_t iops_run(const char * host, uint16_t port, uint32_t timeout,
                       iop_parse_f fparser, iop_processor_f fprocessor,
                       iop_f fconnect, iop_f fdestroy, iop_event_f ferror);

//
// iops_end - 结束一个 iops 服务
// iops     : iops_run 返回的对象
// return   : void
//
extern void iops_end(iops_t iops);

#endif // !_H_SIMPLEC_IOP_SERVER
