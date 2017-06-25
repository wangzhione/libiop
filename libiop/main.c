#include <iop_server.h>

//
// 测试回显服务器
//
void test_echo_server(void);

//
// 会显服务器客户端
//
void test_echo_client(void);

//
// 简单测试 libiop 运行情况
//
int main(int argc, char * argv[]) {

	// 启动 * 装载 socket 库
	socket_start();

	// 客户端和服务器雌雄同体
	if (argc > 1 && !strcmp("client", argv[1])) {
		test_echo_client();
	}
	else {
		// 测试基础的服务器启动
		test_echo_server();
	}

	return EXIT_SUCCESS;
}

// 数据检查
static int _echo_parser(const char * buf, uint32_t len) {
	return (int)len;
}

// 数据处理
static int _echo_processor(iopbase_t base, uint32_t id, char * data, uint32_t len, void * arg) {
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
static void _echo_connect(iopbase_t base, uint32_t id, void * arg) {
	printf("_echo_connect base = %p, id : %u, arg = %p.\n", base, id, arg);
}

// 最终销毁处理
static void _echo_destroy(iopbase_t base, uint32_t id, void * arg) {
	printf("_echo_destroy base = %p, id : %u, arg = %p.\n", base, id, arg);
}

// 错误处理
static int _echo_error(iopbase_t base, uint32_t id, uint32_t err, void * arg) {
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
	return Success_Base;
}

#define _STR_IP			"127.0.0.1"
#define _SHORT_PORT		(8088)
#define _UINT_TIMEOUT	(60)

//
// 测试回显服务器
//
void 
test_echo_server(void) {
	int r;
	iopbase_t base = iop_create();
	printf("create a new iopbase_t = %p.\n", base);
	r = iop_add_ioptcp(base, _STR_IP, _SHORT_PORT, _UINT_TIMEOUT,
		_echo_parser, _echo_processor, _echo_connect, _echo_destroy, _echo_error);
	if (r < Success_Base)
		CERR_EXIT("iop_add_ioptcp is error base, r = %p, %d.", base, r);

	printf("create a new tcp server on ip %s, port %d.", _STR_IP, _SHORT_PORT);
	puts("start iop run loop.");
	iop_run(base);
}

#define _INT_LOOP	(10)

//
// 会显服务器客户端
//
void 
test_echo_client(void) {
	int r, i = -1;
	char str[] = "王志 - 你好!";

	printf("test_echo_client connect [%s:%d]...\n", _STR_IP, _SHORT_PORT);

	// 连接到服务器
	socket_t s = socket_connectos(_STR_IP, _SHORT_PORT, _UINT_TIMEOUT);
	if (s == INVALID_SOCKET) {
		CERR_EXIT("socket_connectos error [%s:%u:%d]", _STR_IP, _SHORT_PORT, _UINT_TIMEOUT);
	}

	while (++i < _INT_LOOP) {
		// 发送一个数据接收一个数据
		printf("socket_sendn len = %zd, str = [%s].\n", sizeof str, str);
		r = socket_sendn(s, str, sizeof str);
		if (r == SOCKET_ERROR) {
			socket_close(s);
			CERR_EXIT("socket_sendn r = %d.", r);
		}
		r = socket_recvn(s, str, sizeof str);
		if (r == SOCKET_ERROR) {
			socket_close(s);
			CERR_EXIT("socket_recvn r = %d.", r);
		}
		printf("socket_recvn len = %zd, str = [%s].\n", sizeof str, str);
	}

	socket_close(s);

	puts("test_echo_client test end...");
}