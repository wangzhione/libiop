#include <iop_server.h>

static int _iop_tcp_fdispatch(iopbase_t base, uint32_t id, uint32_t events, void * arg) {
	int r, n;
	iop_t iop = base->iops + id;
	ioptcp_t sarg = iop->sarg;

	// 销毁事件
	if (events & EV_DELETE) {
		sarg->fdestroy(base, id, arg);
		return SufBase;
	}

	// 读事件
	if (events & EV_READ) {
		n = iop_recv(base, id);
		// 服务器关闭, 直接返回关闭操作
		if (n == ErrClose)
			return ErrClose;

		if (n < SufBase) {
			r = sarg->ferror(base, id, EV_READ, arg);
			return r;
		}

		for (;;) {
			// 读取链接关闭
			n = sarg->fparser(iop->rbuf->str, iop->rbuf->len);
			if (n < SufBase) {
				r = sarg->ferror(base, id, EV_CREATE, arg);
				if (r < SufBase)
					return r;
				break;
			}
			if (n == SufBase)
				break;

			r = sarg->fprocessor(base, id, iop->rbuf->str, n, arg);
			if (SufBase <= r) {
				if (n == iop->rbuf->len) {
					iop->rbuf->len = 0;
					break;
				}
				tstr_popup(iop->rbuf, n);
				continue;
			}
			return r;
		}

	}

	// 写事件
	if (events & EV_WRITE) {
		if (iop->sbuf->len <= 0)
			return iop_mod(base, id, events);

		n = socket_send(iop->s, iop->sbuf->str, iop->sbuf->len);
		if (n < SufBase) {
			if (socket_errno != SOCKET_EINPROGRESS && socket_errno != SOCKET_EWOULDBOCK) {
				r = sarg->ferror(base, id, EV_WRITE, arg);
				if (r < SufBase)
					return r;
			}
			return SufBase;
		}
		if (n == SufBase) return SufBase;

		if ((uint32_t)n >= iop->sbuf->len)
			iop->sbuf->len = 0;
		else
			tstr_popup(iop->sbuf, n);
	}

	// 超时时间处理
	if (events & EV_TIMEOUT) {
		r = sarg->ferror(base, id, EV_TIMEOUT, arg);
		if (r < SufBase)
			return r;
	}

	return SufBase;
}

static int _iop_add_connect(iopbase_t base, uint32_t id, uint32_t events, void * arg) {
	int r;
	iop_t iop;
	socket_t s;
	ioptcp_t sarg = arg;

	if (events & EV_READ) {
		iop = base->iops + id;
		s = socket_accept(iop->s, NULL, NULL);
		if (INVALID_SOCKET == s) {
			RETURN(ErrFd, "socket_accept is error id = %u.", id);
		}

		r = iop_add(base, s, EV_READ, sarg->timeout, _iop_tcp_fdispatch, NULL);
		if (r < SufBase) {
			socket_close(r);
			RETURN(r, "iop_add EV_READ timeout = %d, r = %u", sarg->timeout, r);
		}

		iop = base->iops + r;
		iop->sarg = arg;
		sarg->fconnect(base, r, NULL);
	}

	return SufBase;
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
// return		: 成功返回>=0的id, 失败返回 -1 ErrBase
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
		RETURN(ErrBase, "socket_tcp error host, post = %s | %u.", host, port);
	}

	// 构建系统参数
	sarg = calloc(1, sizeof(struct ioptcp));
	if (NULL == sarg) {
		socket_close(s);
		RETURN(ErrAlloc, "calloc struct ioptcp is error!");
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
	base->tplist = vlist_add(base->tplist, sarg);

	return iop_add(base, s, EV_READ, INVALID_SOCKET, _iop_add_connect, sarg);
}