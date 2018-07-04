#include <iop.h>
#include <iop_poll.h>

// 默认event 调度事件
static inline int _iop_devent(iopbase_t base, uint32_t id, uint32_t events, void * arg) {
	return SBase;
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
	base->dispatch = INT_DISPATCH;
	base->last = time(&base->curt);
	base->keepalive = base->curt;
	base->iohead = -1;
	base->freetail = maxio - 1;

	// 构建具体的处理
	while (i < maxio) {
		iop_t iop = base->iops + i;
		iop->id = i;
		iop->s = INVALID_SOCKET;
		iop->fevent = _iop_devent;

		iop->prev = prev;
		prev = i;
		iop->next = ++i;
	}

	return base;
}

//
// iop_create - 创建新的iopbase_t 对象, io模型集
// return	: iobase_t 模型,失败返回NULL
//
inline iopbase_t
iop_create(void) {
	iopbase_t base = _iopbase_new(INT_POLL);
	if (base) {
		if (SBase > iop_poll_init(base)) {
			iop_delete(base);
			RETURN(NULL, "iop_poll_init base error!");
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
	if (!base) return;
	if (base->iops) {
		while (base->iohead != INVALID_SOCKET)
			iop_del(base, base->iohead);

		for (i = 0; i < base->maxio; ++i) {
			iop_t iop = base->iops + i;
			TSTR_DELETE(iop->suf);
			TSTR_DELETE(iop->ruf);
		}

		base->maxio = 0;
		free(base->iops);
		base->iops = NULL;
	}

	if (base->op.ffree)
		base->op.ffree(base);

	free(base);
}

//
// iop_dispatch - 启动一次事件调度
// base		: io调度对象
// return	: 本次调度处理事件总数
//
int 
iop_dispatch(iopbase_t base) {
	iop_t iop;
	int curid, nextid;
    int r = base->op.fdispatch(base, base->dispatch);
	// 调度一次结果监测
	if (r < SBase)
		return r;

	// 判断时间信息
	if (base->curt > base->last) {
		// clear keepalive, 60 seconds per times
		if (base->curt > base->keepalive + INT_KEEPALIVE) {
			base->keepalive = base->curt;
			curid = base->iohead;
			while (curid != INVALID_SOCKET) {
				iop = base->iops + curid;
				nextid = iop->next;
				if (iop->timeout > 0 && iop->last + iop->timeout < base->curt)
					iop_callback(base, iop, EV_TIMEOUT);
				curid = nextid;
			}
		}

		base->last = base->curt;
	}

	return r;
}

static iop_t _iop_get(iopbase_t base) {
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
	iop_t iop = _iop_get(base);
	if (NULL == iop) {
		RETURN(EBase, "_iop_get is base error = %p.", base);
	}

	iop->s = s;
	iop->events = ets;
	iop->timeout = to;
	iop->fevent = fev;
	iop->last = base->curt;
	iop->arg = arg;

	if (s != INVALID_SOCKET) {
		iop->prev = INVALID_SOCKET;
		iop->next = base->iohead;
		base->iohead = iop->id;
		iop->type = IOP_IO;
		socket_set_nonblock(s);
		r = base->op.fadd(base, iop->id, s, ets);
		if (r < SBase) {
			iop_del(base, iop->id);
			return SOCKET_ERROR;
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

	return SBase;
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
		RETURN(EBase, "iop type is error = [%u, %u].", iop->type, id);
	}
	if (iop->s == INVALID_SOCKET) {
		RETURN(EBase, "iop socket is error = [%"PRIu64", %u].", (int64_t)iop->s, id);
	}

	return base->op.fmod(base, iop->id, iop->s, events);
}

// iop 轮询事件的发送接收操作, 发送没变化, 接收放在接收缓冲区
int
iop_send(iopbase_t base, uint32_t id, const void * data, uint32_t len) {
	int r = SBase;
	const char * csts = data;
	iop_t iop = base->iops + id;
	tstr_t buf = iop->suf;

	if (buf->len <= 0) {
		r = socket_send(iop->s, data, len);
		if (r >= 0 && (uint32_t)r >= len)
			return SBase;
		if (r < 0) {
			if (errno != EINTR && errno != EAGAIN) {
				RETURN(EBase, "socket_send error r = %d.", r);
			}
			r = 0;
		}
		csts += r;
	}

	// 剩余的发送部分, 下次再发
	if (buf->cap > INT_SEND) {
		RETURN(EAlloc, "iop->sbuf->capacity error too length = %zu.", buf->cap);
	}

	// 开始填充内存
	tstr_appendn(buf, csts, len - r);

	if (iop->events & EV_WRITE)
		return SBase;

	return iop_mod(base, id, iop->events | EV_WRITE);;
}


int
iop_recv(iopbase_t base, uint32_t id) {
	int r;
	iop_t iop = base->iops + id;
	tstr_t buf = iop->ruf;

	if (buf->cap > INT_SEND) {
		RETURN(EAlloc, "iop->rbuf->capacity error too length = %zu.", buf->cap);
	}
	tstr_expand(buf, INT_RECV);

	// 开始接收数据
	r = socket_recv(iop->s, buf->str + buf->len, buf->cap - buf->len);
	if (r < 0) {
		if (errno != EINTR && errno != EAGAIN) {
			RETURN(EBase, "socket_recv error r = %d.", r);
		}
		return r;
	}

	// 返回最终结果
	if (r == 0)
		return EClose;

	buf->len += r;
	return SBase;
}