#ifdef _EPOLL

#include <iop_poll.h>
#include <sys/epoll.h>

struct epolls {
	int efd;                    // epoll 文件描述符与
	uint32_t ets;               // epoll 数组的个数
	struct epoll_event evs[];   // 事件数组
};

// 发送事件转换
static inline uint32_t _to_events(uint32_t what) {
	uint32_t events = 0;
	if (what & EV_READ)
		events |= EPOLLIN;
	if (what & EV_WRITE)
		events |= EPOLLOUT;
	return events;
}

// 事件宏转换
static inline uint32_t _to_what(uint32_t events) {
	uint32_t what = 0;
	if (events & (EPOLLHUP | EPOLLERR))
		what = EV_READ | EV_WRITE;
	else {
		if (events & EPOLLIN)
			what |= EV_READ;
		if (events & EPOLLOUT)
			what |= EV_WRITE;
	}
	return what;
}

// epoll 句柄释放
static void _epolls_free(iopbase_t base) {
	struct epolls * mdata = base->mdata;
    if (mdata) {
        base->mdata = NULL;
        if (mdata->efd >= 0)
            close(mdata->efd);
        mdata->efd = -1;
        free(mdata);
    }
}

// epoll 事件调度处理
static int _epolls_dispatch(iopbase_t base, uint32_t timeout) {
	int i, n = 0;
	iop_t iop;
    uint32_t what;
	struct epolls * mdata = base->mdata;

	do
		n = epoll_wait(mdata->efd, mdata->evs, mdata->ets, timeout);
	while (n < SufBase && errno == EINTR);

	// 得到当前时间
	time(&base->curt);
	for (i = 0; i < n; ++i) {
		struct epoll_event * ev = mdata->evs + i;
		uint32_t id = ev->data.u32;
		if (id >= 0 && id < base->maxio) {
			iop = base->iops + id;
			if (id < base->maxio) {
				iop = base->iops + id;
				what = _to_what(ev->events);
				IOP_CB(base, iop, what);
			}
		}
	}
	return n;
}

// epoll 添加处理事件
static inline int _epolls_add(iopbase_t base, uint32_t id, socket_t s, uint32_t events) {
	struct epolls * mdata = base->mdata;
	struct epoll_event ev;
	ev.data.u32 = id;
	ev.events = _to_events(events);
	return epoll_ctl(mdata->efd, EPOLL_CTL_ADD, s, &ev);
}

// epoll 删除监视操作
static inline int _epolls_del(iopbase_t base, uint32_t id, socket_t s) {
	struct epolls * mdata = base->mdata;
	struct epoll_event ev;
	ev.data.u32 = id;
	return epoll_ctl(mdata->efd, EPOLL_CTL_DEL, s, &ev);
}

// epoll 修改句柄注册
static inline int _epolls_mod(iopbase_t base, uint32_t id, socket_t s, uint32_t events) {
	struct epolls * mdata = base->mdata;
	struct epoll_event ev;
	ev.data.u32 = id;
	ev.events = _to_events(events);
	return epoll_ctl(mdata->efd, EPOLL_CTL_MOD, s, &ev);
}

//
// iop_poll_init - 通信的底层接口
// base		: 总的iop对象管理器
// maxsz	: 开启的最大处理数
// return	: SufBase 表示成功
//
int
iop_poll_init(iopbase_t base, unsigned maxsz) {
	struct epolls * mdata;
	struct iopop * op = &base->op;
	int epfd = epoll_create(_INT_POLL);
	if (epfd < SufBase) {
		RETURN(ErrBase, "epoll_create %d is error!", _INT_POLL);
	}

	mdata = malloc(sizeof(struct epolls) + maxsz * sizeof(struct epoll_event));
	if (NULL == mdata) {
		RETURN(ErrAlloc, "malloc error epolls maxsz = %u.", maxsz);
	}
	mdata->efd = epfd;
	mdata->ets = maxsz;
	base->mdata = mdata;

	op->ffree = _epolls_free;
	op->fdispatch = _epolls_dispatch;
	op->fadd = _epolls_add;
	op->fdel = _epolls_del;
	op->fmod = _epolls_mod;

	return SufBase;
}

#endif//_EPOLL
