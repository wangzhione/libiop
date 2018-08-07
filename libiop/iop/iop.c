#include <iop.h>

// iop_event - 默认 event 调度事件
inline static int iop_event(iopbase_t base, uint32_t id, uint32_t events, void * arg) {
    return SBase;
}

// 构建对象
static iopbase_t iopbase_new(uint32_t maxio) {
    uint32_t i = 0, prev = INVALID_SOCKET;
    iopbase_t base = calloc(1, sizeof(struct iopbase));
    base->iops = calloc(maxio, sizeof(struct iop));

    // 开始部署数据
    base->maxio = maxio;
    base->dispatch = INT_DISPATCH;
    base->last = time(&base->curt);
    base->keepalive = base->curt;
    base->iohead = INVALID_SOCKET;
    base->freetail = maxio - 1;
    base->fdel = iop_del;
    // 构建具体的处理
    while (i < maxio) {
        iop_t iop = base->iops + i;
        iop->id = i;
        iop->s = INVALID_SOCKET;
        iop->fevent = iop_event;

        iop->prev = prev;
        prev = i;
        iop->next = ++i;
    }

    return base;
}

//
// iop_create - 创建新的 iopbase_t 对象, io 调度对象
// return   : 失败返回 NULL
//
inline iopbase_t 
iop_create(void) {
    iopbase_t base = iopbase_new(INT_POLL);
    if (SBase > iop_poll(base)) {
        iop_delete(base);
        RETNUL("iop_poll_init base error!");
    }
    return base;
}

//
// iop_delete - 销毁 iopbase_t 对象
// base     : 待销毁的 io 调度对象
// return   : void
//
void 
iop_delete(iopbase_t base) {
    if (!base) return;
    if (base->iops) {
        while (base->iohead != INVALID_SOCKET)
            iop_del(base, base->iohead);

        for (uint32_t i = 0; i < base->maxio; ++i) {
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
// base     : io 调度对象
// return   : 本次调度处理事件总数
//
int 
iop_dispatch(iopbase_t base) {
    int r = base->op.fdispatch(base, base->dispatch);
    // 调度一次结果监测
    if (r < SBase)
        return r;

    // 判断时间信息
    if (base->curt > base->last) {
        // clear keepalive, 60 seconds per times
        if (base->curt > base->keepalive + INT_KEEPALIVE) {
            int curid = base->iohead;
            base->keepalive = base->curt;
            while (curid != INVALID_SOCKET) {
                iop_t iop = base->iops + curid;
                int nextid = iop->next;
                if (iop->timeout > 0 && iop->last + iop->timeout < base->curt)
                    iop_callback(base, iop, EV_TIMEOUT);
                curid = nextid;
            }
        }

        base->last = base->curt;
    }

    return r;
}

// iop_get - 得到一个可用 iop 对象
inline iop_t iop_get(iopbase_t base) {
    iop_t iop = NULL;
    if (base->freehead != INVALID_SOCKET) {
        // 存在释放结点, 找出来处理
        iop = base->iops + base->freehead;
        base->freehead = iop->next;
        if (base->freehead == INVALID_SOCKET)
            base->freetail = INVALID_SOCKET;
        else
            base->iops[base->freehead].prev = INVALID_SOCKET;
    }
    return iop;
}

//
// iop_add - 添加一个新的事件对象到 iopbase 调度对象集中
// base     : io 调度对象
// s        : socket 处理句柄
// events   : 处理事件类型 EV_XXX
// to       : 超时时间, '-1' 表示永不超时
// fevent   : 事件回调函数
// arg      : 用户参数
// return   : 成功返回 iop 的 id, 失败返回 SOCKET_ERROR
//
uint32_t 
iop_add(iopbase_t base,
    socket_t s, uint32_t events, uint32_t to, iop_event_f fevent, void * arg) {
    int r;
    iop_t iop = iop_get(base);
    if (NULL == iop) {
        RETURN(EBase, "iop_get base is error = %p", base);
    }

    iop->s = s;
    iop->events = events;
    iop->timeout = to;
    iop->fevent = fevent;
    iop->last = base->curt;
    iop->arg = arg;

    if (s != INVALID_SOCKET) {
        iop->prev = INVALID_SOCKET;
        iop->next = base->iohead;
        base->iohead = iop->id;
        iop->type = IOP_IO;
        socket_set_nonblock(s);
        r = base->op.fadd(base, iop->id, s, events);
        if (r < SBase) {
            iop_del(base, iop->id);
            return SOCKET_ERROR;
        }
    }

    return iop->id;
}

//
// iop_del - iop 销毁事件
// base     : io 调度对象
// id       : iop 事件 id
// return   : >=0 成功, <0 表示失败
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

        // INVALID_SOCKET 充当链表空结点
        if (iop->prev == INVALID_SOCKET) {
            base->iohead = iop->next;
            if (base->iohead != INVALID_SOCKET)
                base->iops[base->iohead].prev = INVALID_SOCKET;
        } else {
            iop_t node = base->iops + iop->prev;
            node->next = iop->next;
            if (node->next != INVALID_SOCKET)
                base->iops[node->next].prev = node->id;
        }

        iop->prev = base->freetail;
        iop->next = INVALID_SOCKET;
        base->freetail = iop->id;
        if (base->freetail == INVALID_SOCKET)
            base->freehead = INVALID_SOCKET;
        break;
    default:
        CERR("iop->type = %u is error", iop->type);
    }

    iop->type = IOP_FREE;
    return SBase;
}

//
// iop_mod - 修改iop事件订阅事件
// base     : io事件集基础对象 
// id       : iop事件id
// events   : 新的 events 事件
// return   : >= SBase 成功. 否则 表示失败
//
int 
iop_mod(iopbase_t base, uint32_t id, uint32_t events) {
    iop_t iop = base->iops + id;
    if (iop->type != IOP_IO) {
        RETURN(EBase, "iop error type = %u, %u", iop->type, id);
    }
    if (iop->s == INVALID_SOCKET) {
        RETURN(EBase, "iop socket error is %"PRIu64", %u", (int64_t)iop->s, id);
    }

    return base->op.fmod(base, iop->id, iop->s, events);
}

// iop_xxxx 轮询事件的发送接收操作, 发送没变化, 接收放在接收缓冲区
int
iop_send(iopbase_t base, uint32_t id, const void * data, uint32_t len) {
    iop_t iop = base->iops + id;
    const char * str = data;
    tstr_t buf = iop->suf;
    int n = 0;

    // 当前上一个发送缓存发送完毕, 才会继续发送
    if (buf->len <= 0) {
        n = socket_send(iop->s, data, len);
        if (n >= 0 && n >= (int)len)
            return SBase;
        if (n < 0) {
            if (errno != EINTR && errno != EAGAIN) {
                RETURN(EBase, "socket_send error r = %d", n);
            }
            n = 0;
        }
        str += n;
    }

    // 剩余的发送部分, 下次再发. 并且简单检查是否超过发送缓冲区
    if (buf->len > INT_SEND) {
        RETURN(EAlloc, "iop->sbuf->capacity error too length = %zu", buf->len);
    }

    // 开始填充内存
    tstr_appendn(buf, str, len - n);
    if (iop->events & EV_WRITE)
        return SBase;

    return iop_mod(base, id, iop->events | EV_WRITE);;
}

int
iop_recv(iopbase_t base, uint32_t id) {
    iop_t iop = base->iops + id;
    tstr_t buf = iop->ruf;
    int n;

    if (buf->len > INT_RECV) {
        RETURN(EAlloc, "iop->rbuf->capacity error too length = %zu", buf->len);
    }

    // once init recv buf
    if (buf->cap < INT_RECV)
        tstr_expand(buf, INT_RECV);

    // 开始接收数据
    n = socket_recv(iop->s, buf->str + buf->len, buf->cap - buf->len);
    if (n < 0) {
        // 信号打断, 或者重试继续读取操作
        if (errno == EINTR || errno == EAGAIN)
            return SBase;

        CERR("socket_recv error = %d", n);
        // 读取失败, 在 base 中清除 iop id 对象
        iop_del(base, id);
        return EBase;
    }

    // 返回最终结果
    if (n == 0)
        return EClose;

    buf->len += n;
    return SBase;
}
