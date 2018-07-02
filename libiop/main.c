#include <iop_server.h>

#define STR_HOST        "127.0.0.1:8088"
#define INT_TIMEOUT     (60)
#define INT_SLEEP       (5000)

//
// echo_server - 测试回显服务器
// echo_client - 测试回显客户端
//
void echo_server(void);
void echo_client(void);

//
// 简单测试 libiop 运行情况
//
int main(int argc, char * argv[]) {
    // 启动 * 装载 socket 库
    socket_init();

    // 客户端和服务器雌雄同体
    if (argc > 1 && !strcmp("client", argv[1])) {
        echo_client();
    } else {
        // 测试基础的服务器启动
        echo_server();
    }

    return EXIT_SUCCESS;
}

// 数据检查
inline static int echo_parser(const char * buf, uint32_t len) {
    return (int)len;
}

// 数据处理
inline static int echo_processor(iopbase_t base, uint32_t id, char * data, uint32_t len, void * arg) {
    char buf[BUFSIZ];

    if (len >= BUFSIZ)
        len = BUFSIZ - 1;
    buf[len] = '\0';
    memcpy(buf, data, len);

    printf("recv data len = %u, data = %s.\n", len, buf);

    // 开始发送数据
    int r = iop_send(base, id, data, len);
    if (r < 0)
        CERR("iop_send error id = %u, len = %u.\n", id, len);
    return r;
}

// 连接处理
inline static void echo_connect(iopbase_t base, uint32_t id, void * arg) {
    printf("echo_connect base = %p, id : %u, arg = %p.\n", base, id, arg);
}

// 最终销毁处理
inline static void echo_destroy(iopbase_t base, uint32_t id, void * arg) {
    printf("echo_destroy base = %p, id : %u, arg = %p.\n", base, id, arg);
}

// 错误处理
inline static int echo_error(iopbase_t base, uint32_t id, uint32_t err, void * arg) {
    switch (err) {
    case EV_READ:
        CERR("EV_READ base = %p, id = %u, err = %u, arg = %p.", base, id, err, arg);
        break;
    case EV_WRITE:
        CERR("EV_WRITE base = %p, id = %u, err = %u, arg = %p.", base, id, err, arg);
        break;
    case EV_CREATE:
        CERR("EV_CREATE base = %p, id = %u, err = %u, arg = %p.", base, id, err, arg);
        break;
    case EV_TIMEOUT:
        CERR("EV_TIMEOUT base = %p, id = %u, err = %u, arg = %p.", base, id, err, arg);
        break;
    default:
        CERR("default id = %u, err = %u.", id, err);
    }
    return SBase;
}

//
// 测试回显服务器
//
void 
echo_server(void) {
    int r = -1;
    iops_t base = iops_run(STR_HOST, INT_TIMEOUT,
        echo_parser, echo_processor, echo_connect, echo_destroy, echo_error);

    printf("create a new tcp server host %s\n", STR_HOST);
    puts("start iop run loop.");

    // 这里阻塞一会等待消息, 否则异步线程直接退出了
    while (++r < INT_TIMEOUT) {
        puts("echo server run, but my is wait you ... ");
        msleep(INT_SLEEP);
    }

    iops_end(base);
}

//
// 会显服务器客户端
//
void 
echo_client(void) {
    int r;
    char str[] = "Hi - 你好!";

    // 连接到服务器
    printf("echo_client connect [%s] ...\n", STR_HOST);
    socket_t s = socket_connectos(STR_HOST, INT_TIMEOUT);
    if (INVALID_SOCKET == s) {
        EXIT("socket_connectos error [%s:%d]", STR_HOST, INT_TIMEOUT);
    }

    for (int i = 0; i < 9; ++i) {
        // 发送一个数据接收一个数据
        printf("socket_sendn len = %zu, str = [%s].\n", sizeof str, str);
        r = socket_sendn(s, str, sizeof str);
        if (r == SOCKET_ERROR) {
            socket_close(s);
            EXIT("socket_sendn r = %d.", r);
        }
        r = socket_recvn(s, str, sizeof str);
        if (r == SOCKET_ERROR) {
            socket_close(s);
            EXIT("socket_recvn r = %d.", r);
        }
        printf("socket_recvn len = %zu, str = [%s].\n", sizeof str, str);
    }

    socket_close(s);
    puts("echo_client test end...");
}
