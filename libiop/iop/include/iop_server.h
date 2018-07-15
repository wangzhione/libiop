#ifndef _H_IOP_SERVER_LIBIOP
#define _H_IOP_SERVER_LIBIOP

#include <iop.h>

// iops iop server 服务对象
typedef struct iops * iops_t;

//
// iops_create - 创建 iop tcp server 对象并开始监听处理
// host         : 服务器地址 ip:port
// timeout      : 超时时间阀值
// fparser      : 协议解析器
// fprocessor   : 数据处理器
// fconnect     : 当连接创建时候回调
// fdestroy     : 退出时候的回调
// ferror       : 错误的时候回调
// return       : NULL is error, iops_delete 会采用同步方式结束
//
extern iops_t iops_create(const char * host, 
                          uint32_t timeout, 
                          iop_parse_f fparser, 
                          iop_processor_f fprocessor, 
                          iop_f fconnect, 
                          iop_f fdestroy, 
                          iop_event_f ferror);

//
// iops_delete - 结束一个 iops 服务
// iops     : iops_create 返回的对象
// return   : void
//
extern void iops_delete(iops_t iops);

#endif // !_H_IOP_SERVER_LIBIOP
