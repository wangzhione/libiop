#ifndef _H_SIMPLEC_IOP_SERVER
#define _H_SIMPLEC_IOP_SERVER

#include <iop.h>

typedef struct ioptcp {
	char host[_INT_HOST];
	uint16_t port;
	uint32_t timeout;

	iop_event_f ferror;
	iop_f fconnect;
	iop_f fdestroy;
	iop_parse_f fparser;
	iop_processor_f fprocessor;
} *ioptcp_t;

//
// iop_add_ioptcp - 添加tcp服务
// base         : iop对象集
// host         : 服务器ip
// port         : 服务器端口
// timeout      : 超时时间阀值
// fparser      : 协议解析器
// fprocessor   : 数据处理器
// fconnect     : 当连接创建时候回调
// fdestroy     : 退出时间的回调
// ferror       : 错误的时候回调
// return       : 成功返回>=0的id, 失败返回 -1 ErrBase
//
extern int iop_add_ioptcp(iopbase_t base,
	const char * host, uint16_t port, uint32_t timeout,
	iop_parse_f fparser, iop_processor_f fprocessor,
	iop_f fconnect, iop_f fdestroy, iop_event_f ferror);

#endif // !_H_SIMPLEC_IOP_SERVER
