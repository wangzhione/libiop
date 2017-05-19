#include <iop.h>
#include <iop_poll.h>

// 默认event 调度事件
static inline int _iop_default_event(iopbase_t base, uint32_t id, uint32_t events, void * arg) {
	return Success_Base;
}

static iop_t _iop_get_freehead(iopbase_t base) {
	iop_t iop;
	if (base->freehead == INVALID_SOCKET)
		return NULL;

	// 存在释放结点, 找出来处理
	iop = base->iops + base->freehead;
	base->freehead = iop->next;
	if (base->freehead == INVALID_SOCKET)
		base->freetail = INVALID_SOCKET;
	else
		base->iops[base->freehead].prev = INVALID_SOCKET;

	return iop;
}

// 构建对象
static iopbase_t _iopbase_new(uint32_t maxio) {
	uint32_t i = 0, prev = INVALID_SOCKET;
	iopbase_t base = calloc(1, sizeof(struct iopbase));
	if (NULL == base) {
		RETURN(NULL, "_iopbase_new calloc struct iopbase is error!");
	}

	base->iops = calloc(maxio, sizeof(struct iop));
	if (NULL == base->iops) {
		free(base);
		RETURN(NULL, "calloc struct iop is error, maxio = %u.", maxio);
	}

	// 开始部署数据
	base->maxio = maxio;
	base->dispatchval = _INT_DISPATCH;
	base->lastt = time(&base->curt);
	base->lastkeepalivet = base->curt;
	base->iohead = INVALID_SOCKET;
	base->freetail = maxio - 1;

	// 构建具体的处理
	while (i < maxio) {
		iop_t iop = base->iops + i;
		iop->id = i;
		iop->s = INVALID_SOCKET;
		iop->fevent = _iop_default_event;

		iop->prev = prev;
		prev = i;
		iop->next = ++i;
	}

	// 设置文件描述符次数
	SET_RLIMIT_NOFILE(_INT_POLL);
	return base;
}

//
// iop_create - 创建新的iopbase_t 对象, io模型集
// return	: iobase_t 模型,失败返回NULL
//
inline iopbase_t 
iop_create(void) {
	iopbase_t base = _iopbase_new(_INT_POLL);
	if (base) {
		if (Success_Base > iop_init_pool(base, _INT_POLL)) {
			iop_delete(base);
			RETURN(NULL, "iop_init_pool _INT_POLL = %d error!", _INT_POLL);
		}
	}
	return base;
}

//
// iop_delete - 销毁iopbase_t 对象
// base		: 待销毁的io基础对象
// return	: void
//
void 
iop_delete(iopbase_t base) {
	uint32_t i;
	ibuf_t ibuf;
	if (!base) return;
	if (base->iops) {
		while (base->iohead != INVALID_SOCKET)
			iop_del(base, base->iohead);

		for (i = 0; i < base->maxio; ++i) {
			if (!!(ibuf = base->iops[i].sbuf))
				ibuf_delete(ibuf);
			if (!!(ibuf = base->iops[i].rbuf))
				ibuf_delete(ibuf);
		}

		base->maxio = 0;
		free(base->iops);
		base->iops = NULL;
	}

	if (base->op.ffree)
		base->op.ffree(base);

	// 删除链表
	while (base->tplist) {
		ilist_t next = base->tplist->next;
		free(base->tplist->data);
		free(base->tplist);
		base->tplist = next;
	}

	free(base);
}

//
// iop_dispatch - 启动一次事件调度
// base		: io调度对象
// return	: 本次调度处理事件总数
//
int 
iop_dispatch(iopbase_t base) {
	int curid, nextid, r;
	iop_t iop;

	// 调度一次
	r = base->op.fdispatch(base, base->dispatchval);
	if (r < Success_Base)
		return r;

	// 判断时间信息
	if (base->curt > base->lastt) {

		// clear keepalive, 60 seconds per times
		if (base->curt > base->lastkeepalivet + _INT_KEEPALIVE) {
			base->lastkeepalivet = base->curt;
			curid = base->iohead;
			while (curid != INVALID_SOCKET) {
				iop = base->iops + curid;
				nextid = iop->next;
				if (iop->timeout > 0 && iop->lastt + iop->timeout < base->curt)
					IOP_CB(base, iop, EV_TIMEOUT);
				curid = nextid;
			}
		}

		base->lastt = base->curt;
	}

	return r;
}

//
// iop_run - 启动循环事件调度,直到退出
// base		: io事件集基础对象
// return	: void
//
void 
iop_run(iopbase_t base) {
	while (!base->flag) {
		iop_dispatch(base);
	}
	iop_delete(base);
}

static void * _run_thread(void * arg) {
	iop_run(arg);
	return arg;
}

//
// iop_run_pthread - 开启一个线程来跑这个轮询事件
// base		: io调度对象
// th		: 返回调度线程的id
// return	: >=0 表示成功, <0 表示失败
//
int 
iop_run_pthread(iopbase_t base, pthread_t * th) {
	int r;
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_setstacksize(&attr, _INT_STACK);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

	// 线程启动起来
	r = pthread_create(th, &attr, _run_thread, base);

	pthread_attr_destroy(&attr);
	return r;
}

//
// iop_stop - 退出循环事件调度
// base		: io事件集基础对象
// return	: void
//
inline void 
iop_stop(iopbase_t base) {
	base->flag = true;
}

//
// iop_add - 添加一个新的事件对象到iopbase 调度事件集中
// base		: io事件集基础对象
// s		: socket 处理句柄
// ets		: 处理事件类型 EV_XXX
// to		: 超时时间, '-1' 表示永不超时
// fev		: 事件回调函数
// arg		: 用户参数
// return	: 成功返回iop的id, 失败返回SOCKET_ERROR
//
uint32_t 
iop_add(iopbase_t base, socket_t s, uint32_t ets, uint32_t to, iop_event_f fev, void * arg) {
	int r = 0;
	iop_t iop = _iop_get_freehead(base);
	if (NULL == iop) {
		RETURN(Error_Base, "_iop_get_freehead is base error = %p.", base);
	}

	iop->s = s;
	iop->events = ets;
	iop->timeout = to;
	iop->fevent = fev;
	iop->lastt = base->curt;
	iop->arg = arg;

	if (s != INVALID_SOCKET) {
		iop->prev = INVALID_SOCKET;
		iop->next = base->iohead;
		base->iohead = iop->id;
		iop->type = IOP_IO;
		socket_set_nonblock(s);
		r = base->op.fadd(base, iop->id, s, ets);
		if (r < Success_Base) {
			iop_del(base, iop->id);
			return r;
		}
	}

	return iop->id;
}

// 
// iop_del - iop销毁事件
// base		: io事件集基础对象 
// id		: iop事件id
// return	: >=0 成功, <0 表示失败
//
int 
iop_del(iopbase_t base, uint32_t id) {
	iop_t iop = base->iops + id;
	switch (iop->type) {
	case IOP_IO:
		iop->fevent(base, id, EV_DELETE, iop->arg);
		if (iop->s != INVALID_SOCKET) {
			base->op.fdel(base, iop->id, iop->s);
			socket_close(iop->s);
			iop->s = INVALID_SOCKET;
		}

		if (iop->prev == INVALID_SOCKET) {
			base->iohead = iop->next;
			if (iop->next != INVALID_SOCKET)
				base->iops[base->iohead].prev = INVALID_SOCKET;
		}
		else {
			iop_t node = base->iops + iop->prev;
			node->next = iop->next;
			if (node->next != INVALID_SOCKET)
				base->iops[node->next].prev = node->id;
		}

		iop->prev = base->freetail;
		iop->next = INVALID_SOCKET;
		base->freetail = iop->id;
		if (base->freetail == INVALID_SOCKET)
			base->freehead = iop->id;

		break;

	default:
		CERR("iop->type = %u is error!", iop->type);
	}

	iop->type = IOP_FREE;

	return Success_Base;
}

//
// iop_mod - 修改iop事件订阅事件
// base		: io事件集基础对象 
// id		: iop事件id
// events	: 新的events事件
// return	: >=0 成功, <0 表示失败
//
int 
iop_mod(iopbase_t base, uint32_t id, uint32_t events) {
	iop_t iop = base->iops + id;
	if (iop->type != IOP_IO) {
		RETURN(Error_Base, "iop type is error = [%u, %u].", iop->type, id);
	}
	if (iop->s == INVALID_SOCKET) {
		RETURN(Error_Base, "iop socket is error = [%u, %u].", iop->s, id);
	}

	return base->op.fmod(base, iop->id, iop->s, events);
}

// iop 轮询事件的发送接收操作, 发送没变化, 接收放在接收缓冲区
int 
iop_send(iopbase_t base, uint32_t id, const void * data, uint32_t len) {
	int r = Success_Base;
	const char * csts = data;
	iop_t iop = base->iops + id;
	if (NULL == iop->sbuf) {
		iop->sbuf = ibuf_create(len);
		if (NULL == iop->sbuf) {
			RETURN(Error_Alloc, "ibuf_create iop->sbuf alloc error len = %u.", len);
		}
	}

	if (iop->sbuf->size <= 0) {
		r = socket_send(iop->s, data, len);
		if (r >= 0 && (uint32_t)r >= len)
			return Success_Base;
		if (r < 0) {
			if (socket_errno != SOCKET_EINPROGRESS && socket_errno != SOCKET_EWOULDBOCK) {
				RETURN(Error_Base, "socket_send socker_errno = %u, r = %d.", socket_errno, r);
			}
			r = 0;
		}
		csts += r;
	}

	// 剩余的发送部分, 下次再发
	if (iop->sbuf->capacity > _INT_MAXBUF) {
		RETURN(Error_Alloc, "iop->sbuf->capacity error too length = %u.", iop->sbuf->capacity);
	}

	// 开始填充内存
	r = ibuf_pushback(iop->sbuf, csts, len - r);
	if (r < Success_Base) {
		RETURN(Error_Alloc, "ibuf_pushback error div = %d.", len - r);
	}

	if (iop->events & EV_WRITE)
		return Success_Base;
	
	return iop_mod(base, id, iop->events | EV_WRITE);;
}


int 
iop_recv(iopbase_t base, uint32_t id) {
	int r;
	iop_t iop = base->iops + id;
	ibuf_t buf = iop->rbuf;
	if (NULL == buf) {
		buf = ibuf_create(_INT_RECV);
		if (NULL == buf) {
			RETURN(Error_Alloc, "ibuf_create iop->rbuf alloc error len = %u.", _INT_RECV);
		}
		iop->rbuf = buf;
	}
	else {
		if (buf->capacity > _INT_MAXBUF || buf->capacity <= buf->size) {
			RETURN(Error_Alloc, "iop->rbuf->capacity error too length = %u.", iop->sbuf->capacity);
		}
		if (Success_Base > ibuf_expand(buf, _INT_RECV)) {
			RETURN(Error_Alloc, "ibuf_expand error _INT_RECV = %d!!", _INT_RECV);
		}
	}

	// 开始接收数据
	r = socket_recv(iop->s, (char *)buf->data + buf->size, buf->capacity - buf->size);
	if (r < 0) {
		if (socket_errno != SOCKET_EINPROGRESS && socket_errno != SOCKET_EWOULDBOCK) {
			RETURN(Error_Base, "socket_read socker_errno = %u, r = %d.", socket_errno, r);
		}
		return r;
	}

	// 返回最终结果
	if (r == 0)
		return Success_Close;

	buf->size += r;
	return Success_Base;
}