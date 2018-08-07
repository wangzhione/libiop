#ifndef _H_IOP_POLL_LIBIOP

#include <time.h>
#include <iop_poll.h>

struct selecs {
    fd_set rset;
    fd_set wset;
    fd_set rsot;
    fd_set wsot;
#ifndef _MSC_VER
    socket_t maxfd;
#endif
};

// selecs_free - 开始销毁函数
inline static void selecs_free(iopbase_t base) {
    struct selecs * s = base->mata;
    if (s) {
        base->mata = NULL;
        free(s);
    }
}

static int selecs_dispatch(iopbase_t base, uint32_t timeout) {
    iop_t iop;
    uint16_t events;
    int n, num, curid, nextid;
    struct timeval v, * p = &v;
    struct selecs * mata = base->mata;
    if (timeout == (uint32_t)-1)
        p = NULL;
    else {
        v.tv_sec = timeout / 1000;
        v.tv_usec = (timeout % 1000) * 1000;
    }

    // 开始复制变化
    memcpy(&mata->rsot, &mata->rset, sizeof(fd_set));
    memcpy(&mata->wsot, &mata->wset, sizeof(fd_set));
#ifndef _MSC_VER
    do {
        // linux 时间 tv 会改变, winds 不会改变
        // linux 需要 maxfd + 1, winds 不需要
        n = select(mata->maxfd+1, &mata->rsot, &mata->wsot, NULL, p);
    } while (n < SBase && errno == EINTR);
#else
    // window select only listen socket 
    n = select(0, &mata->rsot, &mata->wsot, NULL, p);
#endif
    time(&base->curt);
    if (n <= 0) return n;

    num = 0;
    curid = base->iohead;
    while (curid != SOCKET_ERROR && num < n) {
        iop = base->iops + curid;
        nextid = iop->next;

        // 构建时间类型
        events = 0;
        if (FD_ISSET(iop->s, &mata->rsot))
            events |= EV_READ;
        if (FD_ISSET(iop->s, &mata->wsot))
            events |= EV_WRITE;
        // 监测小时事件并处理
        if (events) {
            ++num;
            iop_callback(base, iop, events);
        }

        curid = nextid;
    }

    return n;
}

// selecs_del 删除句柄
inline static int selecs_del(iopbase_t base, uint32_t id, socket_t s) {
    struct selecs * mata = base->mata;
    FD_CLR(s, &mata->rset);
    FD_CLR(s, &mata->wset);

#ifndef _MSC_VER
    // linux 下需要提供 maxfd 的值
    if (s >= mata->maxfd) {
        int curid = base->iohead;
        mata->maxfd = SOCKET_ERROR;
        while (curid != SOCKET_ERROR) {
            iop_t iop = base->iops + curid;
            // 找出 base->iops 中最大 socket_t fd
            if (mata->maxfd < iop->s) 
                mata->maxfd = iop->s;
            curid = iop->next;
        }
    }
#endif

    return SBase;
}

inline static int selecs_add(iopbase_t base, uint32_t id, socket_t s, uint32_t events) {
    struct selecs * mata = base->mata;
    if (events & EV_READ)
        FD_SET(s, &mata->rset);
    if (events & EV_WRITE)
        FD_SET(s, &mata->wset);
#ifndef _MSC_VER
    // 老子不想加宏了, 去你 MB 的 C
    if (s > mata->maxfd)
        mata->maxfd = s;
#endif
    return SBase;
}

// selecs_mod - 事件修改, 其实只处理了读写事件
inline static int selecs_mod(iopbase_t base, uint32_t id, socket_t s, uint32_t events) {
    struct selecs * mata = base->mata;
    if (events & EV_READ)
        FD_SET(s, &mata->rset);
    else
        FD_CLR(s, &mata->rset);

    if (events & EV_WRITE)
        FD_SET(s, &mata->wset);
    else
        FD_CLR(s, &mata->wset);

    return SBase;
}

//
// iop_poll - 为 iop base 对象注入 poll 处理行为
// base     : 总的 iop 对象基础管理器
// return   : SBase 表示成功
//
inline int
iop_poll(iopbase_t base) {
    base->mata = calloc(1, sizeof(struct selecs));
    base->op.ffree = selecs_free;
    base->op.fdispatch = selecs_dispatch;
    base->op.fadd = selecs_add;
    base->op.fdel = selecs_del;
    base->op.fmod = selecs_mod;
    return SBase;
}

#endif//_H_IOP_POLL_LIBIOP
