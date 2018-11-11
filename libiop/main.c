#include "iop_server.h"

#define STR_HOST        "127.0.0.1:8088"
#define INT_SLEEP       (5000)
#define INT_TIMEOUT     (60)

//
// echo_server - 测试回显服务器
// echo_client - 测试回显客户端
//
void echo_server(void);
void echo_client(int i);

//
// 简单测试 libiop 运行情况
//
int main(int argc, char * argv[]) {
    // 启动 * 装载 socket 库
    socket_init();

    // 客户端和服务器雌雄同体
    if (argc > 1 && !strcmp("client", argv[1])) {
        for (int i = 0; i < INT_IOP * 2; ++i)
            pthread_async(echo_client, i);
        // sleep ... 稍微等等 ...
        msleep(INT_SLEEP);
    } else {
        // 测试基础的服务器启动
        echo_server();
    }

    return EXIT_SUCCESS;
}

// echo_parser - 数据检查
inline static int echo_parser(const char * buf, uint32_t len) {
    return (int)len;
}

// echo_processor - 数据处理
static int echo_processor(iopbase_t base, uint32_t id, char * data, uint32_t len, void * arg) {
    char * buf = malloc(len + 1);
    memcpy(buf, data, len);
    buf[len] = '\0';

    printf("recv id = %u, data = %s, len = %u\n", id, buf, len);

    // 开始发送数据
    int r = iop_send(base, id, buf, len);
    free(buf);
    if (r < SBase) {
        RETURN(r, "iop_send error id : %u, len = %u, r = %d", id, len, r);
    }
    return SBase;
}

// 连接处理
inline static void echo_connect(iopbase_t base, uint32_t id, void * arg) {
    printf("echo_connect base = %p, id = %u, arg = %p\n", base, id, arg);
}

// 最终销毁处理
inline static void echo_destroy(iopbase_t base, uint32_t id, void * arg) {
    printf("echo_destroy base = %p, id = %u, arg = %p\n", base, id, arg);
}

// 错误处理
static int echo_error(iopbase_t base, uint32_t id, uint32_t events, void * arg) {
    switch (events) {
    case EV_READ:
        CERR("EV_READ    base = %p, id = %u, arg = %p", base, id, arg);
        break;
    case EV_WRITE:
        CERR("EV_WRITE   base = %p, id = %u, arg = %p", base, id, arg);
        break;
    case EV_CREATE:
        CERR("EV_CREATE  base = %p, id = %u, arg = %p", base, id, arg);
        break;
    case EV_TIMEOUT:
        CERR("EV_TIMEOUT base = %p, id = %u, arg = %p", base, id, arg);
        break;
    default:
        CERR("EV_DEFAULT base = %p, id = %u, err = %u, arg = %p", base, id, events, arg);
    }
    return SBase;
}

//
// 测试回显服务器
//
void 
echo_server(void) {
    iops_t base = iops_create(STR_HOST, INT_TIMEOUT,
        echo_parser, echo_processor, echo_connect, echo_destroy, echo_error);

    printf("create a new tcp server host %s\n", STR_HOST);
    puts("start iop run loop.");

    // 这里阻塞一会等待消息, 否则异步线程直接退出了
    for (int i = 0; i < INT_TIMEOUT; ++i) {
        puts("echo server run, but my is wait you ... ");
        msleep(INT_SLEEP);
    }

    iops_delete(base);
}

// INT_CLIENT - 客户端发送次数
#define INT_CLIENT      (9)

//
// 会显服务器客户端
//
void 
echo_client(int i) {
    char str[] = "Hello, 世界!";

    // 连接到服务器
    printf("echo_client [%d] connect [%s] ...\n", i, STR_HOST);
    socket_t s = socket_connectos(STR_HOST, INT_TIMEOUT);
    if (INVALID_SOCKET == s) {
        EXIT("socket_connectos [%d] error [%s:%d]", i, STR_HOST, INT_TIMEOUT);
    }

    // CPU 切换调度的切片
    msleep(0);

    for (int i = 0; i < INT_CLIENT; ++i) {
        // 发送一个数据接收一个数据
        int r = socket_sendn(s, str, sizeof str);
        printf("socket_sendn [%d] sz = %zu, str = %s, r = %d\n", i, sizeof str, str, r);
        if (r == SOCKET_ERROR) {
            CERR("socket_sendn [%d] r = %d", i, r);
            socket_close(s); exit(EXIT_FAILURE);
        }
        r = socket_recvn(s, str, sizeof str);
        if (r == SOCKET_ERROR) {
            CERR("socket_recvn [%d] r = %d", i, r);
            socket_close(s); exit(EXIT_FAILURE);
        }
        printf("socket_recvn [%d] sz = %zu, str = %s, r = %d\n", i, sizeof str, str, r);
    }

    shutdown(s, SHUT_WR);
    socket_close(s);
    printf("echo_client [%d] test end ...\n", i);
}
