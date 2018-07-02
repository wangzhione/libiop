#include <pthread.h>
#include <iop_server.h>

struct iops {
    iopbase_t base;         // iop 调度总对象

    pthread_t tid;          // 奔跑线程
    volatile bool irun;     // true 表示 iops 对象奔跑

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
    struct iops * sarg = iop->sarg;

	// 销毁事件
	if (events & EV_DELETE) {
		sarg->fdestroy(base, id, arg);
		return SBase;
	}

	// 读事件
	if (events & EV_READ) {
		n = iop_recv(base, id);
		// 服务器关闭, 直接返回关闭操作
		if (n == EClose)
			return EClose;

		if (n < SBase) {
			r = sarg->ferror(base, id, EV_READ, arg);
			return r;
		}

		for (;;) {
			// 读取链接关闭
			n = sarg->fparser(iop->rbuf->str, iop->rbuf->len);
			if (n < SBase) {
				r = sarg->ferror(base, id, EV_CREATE, arg);
				if (r < SBase)
					return r;
				break;
			}
			if (n == SBase)
				break;

			r = sarg->fprocessor(base, id, iop->rbuf->str, n, arg);
			if (SBase <= r) {
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
		if (n < SBase) {
            // EINTR : 进程还可以处理; EAGIN : 当前缓冲区已经写满可以继续写
			if (errno != EINTR && errno != EAGAIN) {
				r = sarg->ferror(base, id, EV_WRITE, arg);
				if (r < SBase)
					return r;
			}
			return SBase;
		}
		if (n == SBase) return SBase;

		if (n >= (int)iop->sbuf->len)
			iop->sbuf->len = 0;
		else
			tstr_popup(iop->sbuf, n);
	}

	// 超时时间处理
	if (events & EV_TIMEOUT) {
		r = sarg->ferror(base, id, EV_TIMEOUT, arg);
		if (r < SBase)
			return r;
	}

	return SBase;
}

static int _fconnect(iopbase_t base, uint32_t id, uint32_t events, struct iops * sarg) {
	int r;
	iop_t iop;
	socket_t s;

	if (events & EV_READ) {
		iop = base->iops + id;
		s = socket_accept(iop->s, NULL);
		if (INVALID_SOCKET == s) {
			RETURN(EFd, "socket_accept is error id = %u.", id);
		}

		r = iop_add(base, s, EV_READ, sarg->timeout, _fdispatch, NULL);
		if (r < SBase) {
			socket_close(r);
			RETURN(r, "iop_add EV_READ timeout = %d, r = %u", sarg->timeout, r);
		}

		iop = base->iops + r;
		iop->sarg = sarg;
		sarg->fconnect(base, r, iop->arg);
	}

	return SBase;
}

// struct iops 对象创建
static struct iops * _iops_create(const char * host, uint32_t timeout,
    iop_parse_f fparser, iop_processor_f fprocessor,
    iop_f fconnect, iop_f fdestroy, iop_event_f ferror) {
    // 构建 socket tcp 服务
    socket_t s = socket_tcp(host);
    if (INVALID_SOCKET == s) {
        RETNUL("socket_tcp err host is %s", host);
    }

    struct iops * p = malloc(sizeof(struct iops));
    // 如果创建最终 iopbase_t 对象失败, 直接返回
    if ((p->base = iop_create()) == NULL) {
        free(p); socket_close(s);
        RETNUL("iop_create is error!");
    }

    p->irun = true;
    p->timeout = timeout;
    p->fparser = fparser;
    p->fprocessor = fprocessor;
    p->fconnect = fconnect;
    p->fdestroy = fdestroy;
    p->ferror = ferror;
    // 添加主 iop 对象, 永不超时
    if (SOCKET_ERROR == iop_add(p->base, s, EV_READ, -1, (iop_event_f)_fconnect, p)) {
        iop_delete(p->base);
        free(p); socket_close(s);
        RETNUL("iop_add is read -1 error!");
    }

    return p;
}

static void _iops_run(struct iops * iops) {
    iopbase_t base = iops->base;
    while (iops->irun) {
        iop_dispatch(base);
    }
}

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
iops_t 
iops_run(const char * host, uint32_t timeout,
    iop_parse_f fparser, iop_processor_f fprocessor,
    iop_f fconnect, iop_f fdestroy, iop_event_f ferror) {
    struct iops * iops = _iops_create(host, timeout, 
        fparser, fprocessor, fconnect, fdestroy, ferror);
    if (NULL == iops) {
        EXIT("_iops_create iops is empty!");
    }

    if (pthread_create(&iops->tid, NULL, (start_f)_iops_run, iops)) {
        EXIT("pthread_create error r = %p", iops);
    }

    return iops;
}

//
// iops_end - 结束一个 iops 服务
// iops     : iops_run 返回的对象
// return   : void
//
inline void 
iops_end(iops_t iops) {
    if (iops && iops->irun) {
        iops->irun = false;
        pthread_join(iops->tid, NULL);
        iop_delete(iops->base);
        free(iops);
    }
}
