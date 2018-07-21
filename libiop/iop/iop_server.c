#include <pthread.h>
#include <iop_server.h>

struct iops {
    pthread_t tid;          // 奔跑线程
    iopbase_t base;         // iop 调度总对象
    uint32_t timeout;       // 超时时间
    volatile bool run;      // true 表示 iops 运行

    iop_parse_f fparser;
    iop_processor_f fprocessor;
    iop_f fconnect;
    iop_f fdestroy;
    iop_event_f ferror;
};

int iops_dispatch(iopbase_t base, uint32_t id, uint32_t events, void * arg) {
    int r, n;
    iop_t iop = base->iops + id;
    struct iops * srg = iop->srg;

    // 销毁事件
    if (events & EV_DELETE) {
        srg->fdestroy(base, id, arg);
        return SBase;
    }

    // 读事件
    if (events & EV_READ) {
        n = iop_recv(base, id);
        // 服务器关闭, 直接返回关闭操作
        if (n == EClose)
            return EClose;

        if (n < SBase) {
            return srg->ferror(base, id, EV_READ, arg);
        }

        for (;;) {
            // 读取链接关闭
            n = srg->fparser(iop->ruf->str, iop->ruf->len);
            if (n < SBase) {
                r = srg->ferror(base, id, EV_CREATE, arg);
                if (r < SBase)
                    return r;
                break;
            }
            if (n == SBase)
                break;

            r = srg->fprocessor(base, id, iop->ruf->str, n, arg);
            if (r >= SBase) {
                if (n == iop->ruf->len) {
                    iop->ruf->len = 0;
                    break;
                }
                tstr_popup(iop->ruf, n);
                continue;
            }
            return r;
        }
    }

    // 写事件
    if (events & EV_WRITE) {
        if (iop->suf->len <= 0)
            return iop_mod(base, id, events);

        n = socket_send(iop->s, iop->suf->str, iop->suf->len);
        if (n < SBase) {
            // EINTR : 进程还可以处理; EAGIN : 当前缓冲区已经写满可以继续写
            if (errno != EINTR && errno != EAGAIN) {
                r = srg->ferror(base, id, EV_WRITE, arg);
                if (r < SBase)
                    return r;
            }
            return SBase;
        }
        if (n == SBase)
            return SBase;

        // 截断已经发送内存
        tstr_popup(iop->suf, n);
    }

    // 超时时间处理
    if (events & EV_TIMEOUT) {
        r = srg->ferror(base, id, EV_TIMEOUT, arg);
        if (r < SBase)
            return r;
    }

    return SBase;
}

int iops_listen(iopbase_t base, uint32_t id, uint32_t events, void * arg) {
    if (events & EV_READ) {
        struct iops * srg = arg;
        iop_t iop = base->iops + id;
        socket_t s = socket_accept(iop->s, NULL);
        if (INVALID_SOCKET == s) {
            RETURN(EFd, "socket_accept is error id = %u", id);
        }

        int r = iop_add(base, s, EV_READ, srg->timeout, iops_dispatch, NULL);
        if (r < SBase) {
            socket_close(r);
            RETURN(r, "iop_add EV_READ timeout = %d, r = %u", srg->timeout, r);
        }

        iop = base->iops + r;
        iop->srg = srg;
        srg->fconnect(base, r, iop->arg);
    }
    return SBase;
}

void iops_run(struct iops * p) {
    while (p->run) {
        iop_dispatch(p->base);
    }
}

//
// iops_create - 创建 iop tcp server 对象并开始监听处理
// host        : 服务器地址 ip:port
// timeout     : 超时时间阀值
// fparser     : 协议解析器
// fprocessor  : 数据处理器
// fconnect    : 当连接创建时候回调
// fdestroy    : 退出时候的回调
// ferror      : 错误的时候回调
// return      : NULL is error, iops_delete 会采用同步方式结束
//
iops_t 
iops_create(const char * host, 
            uint32_t timeout, 
            iop_parse_f fparser, 
            iop_processor_f fprocessor, 
            iop_f fconnect, 
            iop_f fdestroy, 
            iop_event_f ferror) {
    // 构建 socket tcp 服务
    socket_t s = socket_tcp(host);
    if (INVALID_SOCKET == s) {
        RETNUL("socket_tcp host error is %s", host);
    }

    struct iops * p = malloc(sizeof(struct iops));
    // 如果创建最终 iopbase_t 对象失败, 直接返回
    if ((p->base = iop_create()) == NULL) {
        free(p); socket_close(s);
        RETNUL("iop_create is error");
    }

    p->run = true;
    p->timeout = timeout;
    p->fparser = fparser;
    p->fprocessor = fprocessor;
    p->fconnect = fconnect;
    p->fdestroy = fdestroy;
    p->ferror = ferror;

    // 添加主 iop 对象, 永不超时
    if (SOCKET_ERROR == iop_add(p->base, s, EV_READ, -1, iops_listen, p)) {
        iop_delete(p->base);
        free(p); socket_close(s);
        RETNUL("iop_add is read SOCKET_ERROR error");
    }

    // pthread create run func 
    if (pthread_create(&p->tid, NULL, (start_f)iops_run, p)) {
        iop_delete(p->base);
        free(p); socket_close(s);
        RETNUL("pthread_create error r = %p", p);
    }

    return p;
}

//
// iops_delete - 结束一个 iops 服务
// p           : iops_create 返回的对象
// return      : void
//
inline void 
iops_delete(iops_t p) {
    if (p && p->run) {
        p->run = false;
        pthread_join(p->tid, NULL);
        iop_delete(p->base);
        free(p);
    }
}
