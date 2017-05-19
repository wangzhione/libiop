#include <iop_server.h>

static int _iop_tcp_fdispatch(iopbase_t base, uint32_t id, uint32_t events, void * arg) {
	int r, n;
	iop_t iop = base->iops + id;
	ioptcp_t sarg = iop->sarg;

	// 销毁事件
	if (events & EV_DELETE) {
		sarg->fdestroy(base, id, arg);
		return Success_Base;
	}

	// 读事件
	if (events & EV_READ) {
		n = iop_recv(base, id);
		if (n < Success_Base) {
			r = sarg->ferror(base, id, EV_READ, arg);
			return r;
		}

		// 服务器关闭, 直接返回关闭操作
		if (n == Success_Close)
			return Success_Close;
		
		for(;;) {
			// 读取链接关闭
			n = sarg->fparser(iop->rbuf->data, iop->rbuf->size);
			if (n < Success_Base) {
				r = sarg->ferror(base, id, EV_CREATE, arg);
				if(r < Success_Base)
					return r;
				break;
			}
			if (n == Success_Base)
				break;

			r = sarg->fprocessor(base, id, iop->rbuf->data, n, arg);
			if (Success_Base <= r) {
				if (n == iop->rbuf->size) {
					iop->rbuf->size = 0;
					break;
				}
				ibuf_popfront(iop->rbuf, n);
				continue;
			}
			return r;
		}

	}

	// 写事件
	if (events & EV_WRITE) {
		if (iop->sbuf->size <= 0)
			return iop_mod(base, id, events);

		n = socket_send(iop->s, iop->sbuf->data, iop->sbuf->size);
		if (n < Success_Base) {
			if (socket_errno != SOCKET_EINPROGRESS && socket_errno != SOCKET_EWOULDBOCK) {
				r = sarg->ferror(base, id, EV_WRITE, arg);
				if (r < Success_Base)
					return r;
			}
			return Success_Base;
		}
		if (n == Success_Base) return Success_Base;

		if ((uint32_t)n >= iop->sbuf->size)
			iop->sbuf->size = 0;
		else
			ibuf_popfront(iop->sbuf, n);
	}

	// 超时时间处理
	if (events & EV_TIMEOUT) {
		r = sarg->ferror(base, id, EV_TIMEOUT, arg);
		if (r < Success_Base) 
			return r;
	}

	return Success_Base;
}

static int _iop_add_connect(iopbase_t base, uint32_t id, uint32_t events, void * arg) {
	int r;
	iop_t iop;
	socket_t s;
	ioptcp_t sarg = arg;
	if (events & EV_DELETE) {
		RETURN(Success_Base, "tcp server destroy[%s:%u].", sarg->host, sarg->port);
	}

	if (events & EV_READ) {
		iop = base->iops + id;
		s = socket_accept(iop->s, NULL, NULL);
		if (INVALID_SOCKET == s) {
			RETURN(Error_Fd, "socket_accept is error s = %u.", iop->s);
		}

		r = iop_add(base, s, EV_READ, sarg->timeout, _iop_tcp_fdispatch, NULL);
		if (r < Success_Base) {
			socket_close(r);
			RETURN(r, "iop_add EV_READ timeout = %d, r = %u", sarg->timeout, r);
		}

		iop = base->iops + r;
		iop->sarg = arg;
		sarg->fconnect(base, r, NULL);
	}
	return Success_Base;
}



// 添加数据到 ilist_t base->tplist 后面
static ilist_t _ilist_add(ilist_t list, void * arg) {
	ilist_t node = malloc(sizeof(struct ilist));
	if (!node) {
		RETURN(list, "malloc sizeof(struct ilist) is error!");
	}
	node->data = arg;
	node->next = NULL;

	if (!list) 
		return node;
	list->next = node;
	return list;
}

//
// iop_add_ioptcp - 添加tcp服务
// base			: iop对象集
// host			: 服务器ip
// port			: 服务器端口
// timeout		: 超时时间阀值,单位秒
// fparser		: 协议解析器
// fprocessor	: 数据处理器
// fconnect		: 当连接创建时候回调
// fdestroy		: 退出时间的回调
// ferror		: 错误的时候回调
// return		: 成功返回>=0的id, 失败返回 -1 Error_Base
//
int iop_add_ioptcp(iopbase_t base,
	const char * host, uint16_t port, uint32_t timeout,
	iop_parse_f fparser, iop_processor_f fprocessor,
	iop_f fconnect, iop_f fdestroy, iop_event_f ferror) {
	socket_t s;
	ioptcp_t sarg;

	// 构建socket tcp 服务
	s = socket_tcp(host, port);
	if (s == INVALID_SOCKET) {
		RETURN(Error_Base, "socket_tcp error host, post = %s | %u.", host, port);
	}

	// 构建系统参数
	sarg = calloc(1, sizeof(struct ioptcp));
	if (NULL == sarg) {
		socket_close(s);
		RETURN(Error_Alloc, "calloc struct ioptcp is error!");
	}

	// 开始复制内容
	strncpy(sarg->host, host, _INT_HOST - 1);
	sarg->host[_INT_HOST - 1] = '\0';
	
	sarg->port = port;
	sarg->timeout = timeout;
	sarg->fconnect = fconnect;
	sarg->fdestroy = fdestroy;
	sarg->ferror = ferror;
	sarg->fparser = fparser;
	sarg->fprocessor = fprocessor;
	base->tplist = _ilist_add(base->tplist, sarg);

	return iop_add(base, s, EV_READ, INVALID_SOCKET, _iop_add_connect, sarg);
}