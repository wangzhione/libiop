#include <iop_server.h>

struct iopserver {
    uint32_t timeout;
    iop_parse_f fparser;
    iop_processor_f fprocessor;
    iop_f fconnect;
    iop_f fdestroy;
    iop_event_f ferror;
};

static int _fdispatch(iopbase_t base, uint32_t id, uint32_t events, void * arg) {
	int r, n;
	iop_t iop = base->iops + id;
    struct iopserver * sarg = iop->sarg;

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
            // EINPROGRESS : 进程正在处理; EWOULDBOCK : 当前缓冲区已经写满可以继续写
			if (errno != EINPROGRESS && errno != EWOULDBOCK) {
				r = sarg->ferror(base, id, EV_WRITE, arg);
				if (r < SufBase)
					return r;
			}
			return SufBase;
		}
		if (n == SufBase) return SufBase;

		if (n >= (int)iop->sbuf->len)
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

static int _fconnect(iopbase_t base, uint32_t id, uint32_t events, struct iopserver * sarg) {
	int r;
	iop_t iop;
	socket_t s;

	if (events & EV_READ) {
		iop = base->iops + id;
		s = socket_accept(iop->s, NULL);
		if (INVALID_SOCKET == s) {
			RETURN(ErrFd, "socket_accept is error id = %u.", id);
		}

		r = iop_add(base, s, EV_READ, sarg->timeout, _fdispatch, NULL);
		if (r < SufBase) {
			socket_close(r);
			RETURN(r, "iop_add EV_READ timeout = %d, r = %u", sarg->timeout, r);
		}

		iop = base->iops + r;
		iop->sarg = sarg;
		sarg->fconnect(base, r, iop->arg);
	}

	return SufBase;
}

// struct iopserver 对象创建
static int _iopserver_create(iopbase_t base,
    const char * host, uint16_t port, uint32_t timeout,
    iop_parse_f fparser, iop_processor_f fprocessor,
    iop_f fconnect, iop_f fdestroy, iop_event_f ferror) {
    struct iopserver * sarg;
	// 构建 socket tcp 服务
	socket_t s = socket_tcp(host, port);
	if (s == INVALID_SOCKET) {
		RETURN(ErrBase, "socket_tcp err = %s | %u.", host, port);
	}

	// 构建系统参数, 并逐个填充内容
	if ((sarg = malloc(sizeof(struct iopserver))) == NULL) {
		socket_close(s);
		RETURN(ErrAlloc, "calloc struct ioptcp is error!");
	}
	sarg->timeout = timeout;
    sarg->fparser = fparser;
    sarg->fprocessor = fprocessor;
	sarg->fconnect = fconnect;
	sarg->fdestroy = fdestroy;
	sarg->ferror = ferror;
	base->tplist = vlist_add(base->tplist, sarg);
    // 添加主 iop 对象, 永不超时
	return iop_add(base, s, EV_READ, -1, (iop_event_f)_fconnect, sarg);
}

//
// iop_server - 启动一个 iop tcp server, 开始监听处理
// host         : 服务器ip
// port         : 服务器端口
// timeout      : 超时时间阀值
// fparser      : 协议解析器
// fprocessor   : 数据处理器
// fconnect     : 当连接创建时候回调
// fdestroy     : 退出时间的回调
// ferror       : 错误的时候回调
// return       : 返回 iop 对象, 结束时候可以调用 iop_end
//
iopbase_t 
iop_server(const char * host, uint16_t port, uint32_t timeout,
    iop_parse_f fparser, iop_processor_f fprocessor,
    iop_f fconnect, iop_f fdestroy, iop_event_f ferror) {
    int r;
    iopbase_t base = iop_create();
    if (NULL == base)
        CERR_EXIT("iop_create is error!");
    
    r = _iopserver_create(base, host, port, timeout, 
        fparser, fprocessor, fconnect, fdestroy, ferror);
    if (r < SufBase) {
        iop_delete(base);
        CERR_EXIT("_iopserver_create base error r = %p, %d.", base, r);
    }
    
    r = iop_run(base);
    if (r < SufBase) {
        iop_delete(base);
        CERR_EXIT("iop_run base error r = %p, %d.", base, r);
    }

    return base;
}