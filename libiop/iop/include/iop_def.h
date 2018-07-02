#ifndef _H_SIMPLEC_IOP_DEF
#define _H_SIMPLEC_IOP_DEF

#include <tstr.h>
#include <socket.h>

//
// IOP_XXX 是处理内部事件
//
#define IOP_FREE        (0)         // 释放操作
#define IOP_IO          (1)         // IO操作

//
// EV_XXX 是特定消息处理动作的标识
//
#define EV_READ         (1 << 0)    // 读事件
#define EV_WRITE        (1 << 1)    // 写事件
#define EV_CREATE       (1 << 2)    // 创建事件 
#define EV_DELETE       (1 << 3)    // 销毁事件
#define EV_TIMEOUT      (1 << 4)    // 超时事件

//
// _INT_XXX 系统运行中用到的参数
//
#define _INT_DISPATCH   (500)       // 事件调度的时间间隔 毫秒
#define _INT_KEEPALIVE  (60)        // 心跳包检查 秒
#define _INT_POLL       (1024)      // events limit
#define _INT_SEND       (1 << 23)   // socket send buf 最大8M	
#define _INT_RECV       (1 << 12)   // 4k 接收缓冲区

typedef struct iop * iop_t;
typedef struct iopbase * iopbase_t;

//
// iop_parse_f - 协议解析回调函数
// buf		: 数据内存首地址
// len		: 处理数据长度
// return	: 返回0表示还有更多数据需要解析, -1代表协议错误, >0 表示解析成功一个包了
//
typedef int (* iop_parse_f)(const char * buf, uint32_t len);

//
// iop_processor_f - 数据处理器
// base		: iopbase 结构指针, iop基础对象集
// id		: iop对象的id
// buf		: 数据包起始点
// len		: 数据包长度
// arg		: 自带的参数
// return	: -1代表要关闭连接, 0代表正常
//
typedef int (* iop_processor_f)(iopbase_t base, uint32_t id, char * buf, uint32_t len, void * arg);

//
// iop_event_f - 事件毁掉函数, 返回EBase代表要删除对象, 返回SBase代表正常
// base		: iopbase 结构指针, iop基础对象集
// id		: iop对象的id
// arg		: 自带的参数
// return	: -1代表要删除事件, 0代表不删除
//
typedef int (* iop_event_f)(iopbase_t base, uint32_t id, uint32_t events, void * arg);


// 调度处理事件
typedef int (* iop_dispatch_f)(iopbase_t base, uint32_t id);

//
// iop_f - 基础事件的回调函数
// base		: iopbase 结构指针, iop基础对象集
// id		: iop对象的id
// arg		: 自带的参数
// return	: -1表示失败, 0表示成功, 自己定义处理
//
typedef void (* iop_f)(iopbase_t base, uint32_t id, void * arg);


//
// iop结构, 每一个iop对象都会对应一个iop结构
//
struct iop {
	uint32_t id;            // 对应的id
	socket_t s;             // 对应的socket
	uint8_t type;           // 对象类型 IOP_XXX 0:free, 1:io, 2:timer
	int prev;               // 上一个对象
	int next;               // 下一个对象
	uint32_t events;        // 关注的事件
	uint32_t timeout;       // 超时值
	iop_event_f fevent;     // 事件毁掉函数
	void * arg;             // 用户指定参数, 由用户负责释放资源
	void * sarg;            // 系统指定参数, 由系统自动释放资源

	struct tstr sbuf[1];    // 发送缓冲区, 希望保存在栈上
	struct tstr rbuf[1];    // 接收缓冲区
	time_t lastt;           // 最后一次调度时间
};

struct iopop {
	void (* ffree)(iopbase_t);                              // 资源释放接口
	int  (* fdispatch)(iopbase_t, uint32_t);                // 模型调度接口
	int  (* fadd)(iopbase_t, uint32_t, socket_t, uint32_t); // 添加事件接口
	int  (* fdel)(iopbase_t, uint32_t, socket_t);           // 删除事件接口
	int  (* fmod)(iopbase_t, uint32_t, socket_t, uint32_t); // 修改事件接口
};

struct iopbase {
	iop_t iops;             // 所有的iop对象
	uint32_t maxio;         // 最大并发io数
	uint32_t maxbuf;        // 单个发送或接收缓存的最大值
	uint32_t freehead;      // 可用iop列表头
	uint32_t freetail;      // 可用iop列表尾,最后一个
	uint32_t iohead;        // 已用io类型的iop列表

	int dispatchval;        // 调度的事件间隔
	struct iopop op;        // 事件模型的内部实现
	void * mdata;           // 事件模型特定数据

	time_t curt;            // 当前调度时间
	time_t lastt;           // 上次调度时间
	time_t lastkeepalivet;  // 最后一次心跳的时间
};

//
// IOP_CB - iop处理帮助宏
// base		: iopbase_t, iop 对象的管理器
// iop		: 当前处理的 iop 对象
// events	: 事件合集
//
#define IOP_CB(base, iop, events) \
	do { \
		if(iop->type != IOP_FREE) { \
			int $type = iop->fevent(base, iop->id, events, iop->arg); \
			if ($type >= SBase) \
				iop->lastt = base->curt; \
			else { \
                extern int iop_del(iopbase_t base, uint32_t id); \
				iop_del(base, iop->id); \
            } \
		} \
	} while(0)

#endif // !_H_SIMPLEC_IOP_DEF