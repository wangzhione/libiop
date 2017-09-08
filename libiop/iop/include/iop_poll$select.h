#if !defined(_HAVE_EPOLL)

#include <time.h>
#include <iop_poll.h>

typedef struct selects {
#ifdef __GNUC__
	socket_t maxfd;
#endif
	fd_set rset;
	fd_set wset;
	fd_set orset;
	fd_set owset;
} * selects_t;

// 开始销毁函数
static inline void _selects_free(iopbase_t base) {
	selects_t s = base->mdata;
	if (s) {
		base->mdata = NULL;
		free(s);
	}
}

static int _selects_dispatch(iopbase_t base, uint32_t timeout) {
	iop_t iop;
	uint32_t revents;
	int n, num, curid, nextid;
	struct timeval tv = { 0 };
	selects_t mdata = base->mdata;
	if (timeout > 0) {
		tv.tv_sec = timeout / 1000;
		tv.tv_usec = (timeout % 1000) * 1000;
	}

	// 开始复制变化
	memcpy(&mdata->orset, &mdata->rset, sizeof(fd_set));
	memcpy(&mdata->owset, &mdata->wset, sizeof(fd_set));

	// 开始等待搞起了, window select 不需要最大 maxfd
#ifdef _MSC_VER
	// window select only listen socket 
	n = select(0, &mdata->orset, &mdata->owset, NULL, &tv);
	if (n < 0 && errno == EAGAIN) {
		// 当定时器时候等待
		n = 0;
		Sleep(tv.tv_sec * 1000 + tv.tv_usec / 1000);
	}
#else
	do {
		// 时间tv会改变, 时间总的而言不变化
		n = select(mdata->maxfd + 1, &mdata->orset, &mdata->owset, NULL, &tv);
	} while (n < SufBase && errno == SOCKET_EINTR);
#endif
	time(&base->curt);
	if (n <= 0)
		return n;

	num = 0;
	curid = base->iohead;
	while (curid != SOCKET_ERROR && num < n) {
		iop = base->iops + curid;
		nextid = iop->next;
		revents = 0;

		// 构建时间类型
		if (FD_ISSET(iop->s, &mdata->orset))
			revents |= EV_READ;
		if (FD_ISSET(iop->s, &mdata->owset))
			revents |= EV_WRITE;

		// 监测小时事件并处理
		if (revents) {
			++num;
			IOP_CB(base, iop, revents);
		}

		curid = nextid;
	}

	return n;
}

static int _selects_add(iopbase_t base, uint32_t id, socket_t s, uint32_t events) {
	selects_t mdata = base->mdata;
	if (events & EV_READ)
		FD_SET(s, &mdata->rset);
	if (events & EV_WRITE)
		FD_SET(s, &mdata->wset);

#ifndef _MSC_VER
	// 老子不想加宏了, 去你MB的C
	if (s > mdata->maxfd)
		mdata->maxfd = s;
#endif

	return SufBase;
}

// select 删除句柄
static int _selects_del(iopbase_t base, uint32_t id, socket_t s) {
	selects_t mdata = base->mdata;
	FD_CLR(s, &mdata->rset);
	FD_CLR(s, &mdata->wset);

#ifndef _MSC_VER
	// linux下需要提供maxfd的值
	if (s >= mdata->maxfd) {
		int curid = base->iohead;
		mdata->maxfd = SOCKET_ERROR;

		while (curid != SOCKET_ERROR) {
			iop_t iop = base->iops + curid;
			if (curid != id) {
				if (mdata->maxfd < iop->s)
					mdata->maxfd = iop->s;
			}
			curid = iop->next;
		}
	}
#endif

	return SufBase;
}

// 事件修改, 其实只处理了读写事件
static int _selects_mod(iopbase_t base, uint32_t id, socket_t s, uint32_t events) {
	selects_t mdata = base->mdata;
	if (events & EV_READ)
		FD_SET(s, &mdata->rset);
	else
		FD_CLR(s, &mdata->rset);

	if (events & EV_WRITE)
		FD_SET(s, &mdata->wset);
	else
		FD_CLR(s, &mdata->wset);

	return SufBase;
}

//
// iop_init_pool - 通信的底层接口
// base		: 总的iop对象管理器
// maxsz	: 开启的最大处理数
// return	: SufBase 表示成功
//
int
iop_init_pool(iopbase_t base, unsigned maxsz) {
	iopop_t op;
	selects_t mdata = calloc(1, sizeof(struct selects));
	if (NULL == mdata) {
		RETURN(ErrAlloc, "malloc sizeof(struct selects) is error!");
	}
	base->mdata = mdata;

	op = &base->op;
	op->ffree = _selects_free;
	op->fdispatch = _selects_dispatch;
	op->fadd = _selects_add;
	op->fdel = _selects_del;
	op->fmod = _selects_mod;

	return SufBase;
}

#endif