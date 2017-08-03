#ifndef _H_SIMPLEC_SCSOCKET
#define _H_SIMPLEC_SCSOCKET

#include <struct.h>

//
//	- 跨平台的丑陋从这里开始, 封装一些共用实现
//  __GNUC		=> linux 平台特殊操作		-> gcc
//  __MSC_VER	=> winds 平台特殊操作		-> vs
//
#ifdef __GNUC__

#include <fcntl.h>
#include <netdb.h>
#include <unistd.h>
#include <signal.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <sys/un.h>
#include <sys/uio.h>
#include <sys/select.h>
#include <sys/resource.h>

//
// This is used instead of -1, since the. by WinSock
//
#define INVALID_SOCKET			(~0)
#define SOCKET_ERROR            (-1)

#define socket_errno			errno
#define SOCKET_EINTR			EINTR
#define SOCKET_EAGAIN			EAGAIN
#define SOCKET_EINVAL			EINVAL
#define SOCKET_EINPROGRESS		EINPROGRESS
#define SOCKET_EMFILE			EMFILE
#define SOCKET_ENFILE			ENFILE

// 链接还在进行中, linux这里显示 EINPROGRESS，winds应该是 WSAEWOULDBLOCK
#define SOCKET_CONNECTED		EINPROGRESS

#if defined(EWOULDBOCK)
#define SOCKET_EWOULDBOCK		EWOULDBOCK
#else
#define SOCKET_EWOULDBOCK		EAGAIN
#endif

#define IGNORE_SIGNAL(sig)			signal(sig, SIG_IGN)

#define SET_RLIMIT_NOFILE(num) \
	do {\
		struct rlimit $r = { num, num }; \
		setrlimit(RLIMIT_NOFILE, &$r);\
	} while(0)

typedef int socket_t;

#elif _MSC_VER

#undef	FD_SETSIZE
#define FD_SETSIZE				(1024)
#include <ws2tcpip.h>

#define IGNORE_SIGNAL(sig)
#define SET_RLIMIT_NOFILE(num)	

#define socket_errno			WSAGetLastError()
#define SOCKET_EINTR			WSAEINTR
#define SOCKET_EAGAIN			WSAEWOULDBLOCK
#define SOCKET_EINVAL			WSAEINVAL
#define SOCKET_EWOULDBOCK		WSAEWOULDBLOCK
#define SOCKET_EINPROGRESS		WSAEINPROGRESS
#define SOCKET_EMFILE			WSAEMFILE
#define SOCKET_ENFILE			WSAETOOMANYREFS

#define SOCKET_CONNECTED		WSAEWOULDBLOCK

typedef int socklen_t;
typedef SOCKET socket_t;

//
// gettimeofday - Linux sys/time.h 中得到微秒的一种实现
// tv		:	返回结果包含秒数和微秒数
// tz		:	包含的时区,在window上这个变量没有用不返回
// return	:   默认返回0
//
extern int gettimeofday(struct timeval * tv, void * tz);

#else
#	error "error : Currently only supports the Best New CL and GCC!"
#endif

#undef	CERR
extern const char * sh_strerr(int error);
#define CERR(fmt, ...) \
	fprintf(stderr, "[%s:%s:%d][errno %d:%s]" fmt "\n", __FILE__, __func__, __LINE__, socket_errno, sh_strerr(socket_errno), ##__VA_ARGS__)

// EAGAIN and EWOULDBLOCK may be not the same value.
#if (SOCKET_EAGAIN != SOCKET_EWOULDBOCK)
#define SOCKET_WAGAIN SOCKET_EAGAIN : case SOCKET_EWOULDBOCK
#else
#define SOCKET_WAGAIN SOCKET_EAGAIN
#endif

//
// IGNORE_SIGPIPE - 管道破裂,忽略SIGPIPE信号
//
#define IGNORE_SIGPIPE()		IGNORE_SIGNAL(SIGPIPE)

// 目前通用的tcp udp v4地址
typedef struct sockaddr_in sockaddr_t;

//
// MAKE_TIMEVAL - 毫秒数转成 timeval 变量
// tv		: struct timeval 变量
// msec		: 毫秒数
//
#define MAKE_TIMEVAL(tv, msec) \
	do {\
		if((msec) > 0) {\
			(tv).tv_sec = (msec) / 1000;\
			(tv).tv_usec = ((msec) % 1000) * 1000;\
		}\
		else {\
			(tv).tv_sec = 0;\
			(tv).tv_usec = 0;\
		}\
	} while(0)

//
// socket_start	- 单例启动socket库的初始化方法
// socket_addr	- 通过ip, port 得到 ipv4 地址信息
// 
extern void socket_start(void);
extern int socket_addr(const char * ip, uint16_t port, sockaddr_t * addr);

//
// socket_dgram		- 创建UDP socket
// socket_stream	- 创建TCP socket
// socket_close		- 关闭上面创建后的句柄
// socket_read		- 读取数据
// socket_write		- 写入数据
//
extern socket_t socket_dgram(void);
extern socket_t socket_stream(void);
extern int socket_close(socket_t s);
extern int socket_read(socket_t s, void * data, int sz);
extern int socket_write(socket_t s, const void * data, int sz);

//
// socket_set_block		- 设置套接字是阻塞
// socket_set_nonblock	- 设置套接字是非阻塞
// socket_set_reuseaddr	- 开启地址复用
// socket_set_keepalive - 开启心跳包检测, 默认2h 5次
// socket_set_recvtimeo	- 设置接收数据毫秒超时时间
// socket_set_sendtimeo	- 设置发送数据毫秒超时时间
//
extern int socket_set_block(socket_t s);
extern int socket_set_nonblock(socket_t s);
extern int socket_set_reuseaddr(socket_t s);
extern int socket_set_keepalive(socket_t s);
extern int socket_set_recvtimeo(socket_t s, int ms);
extern int socket_set_sendtimeo(socket_t s, int ms);

//
// socket_get_error - 得到当前socket error值, 0 表示正确, 其它都是错误
//
extern int socket_get_error(socket_t s);

//
// socket_recv		- socket接受信息
// socket_recvn		- socket接受len个字节进来
// socket_send		- socket发送消息
// socket_sendn		- socket发送len个字节出去
// socket_recvfrom	- recvfrom接受函数
// socket_sendto	- udp发送函数, 通过socket_udp 搞的完全可以 socket_send发送
//
extern int socket_recv(socket_t s, void * buf, int len);
extern int socket_recvn(socket_t s, void * buf, int len);
extern int socket_send(socket_t s, const void * buf, int len);
extern int socket_sendn(socket_t s, const void * buf, int len);
extern int socket_recvfrom(socket_t s, void * buf, int len, int flags, sockaddr_t * in, socklen_t * inlen);
extern int socket_sendto(socket_t s, const void * buf, int len, int flags, const sockaddr_t * to, socklen_t tolen);

//
// socket_bind		- 端口绑定返回绑定好的 socket fd, 失败返回 INVALID_SOCKET or PF_INET PF_INET6
// socket_listen	- 端口监听返回监听好的 socket fd.
//
extern socket_t socket_bind(const char * host, uint16_t port, uint8_t protocol, int * family);
extern socket_t socket_listen(const char * host, uint16_t port);

//
// socket_tcp			- 创建TCP详细的套接字
// socket_udp			- 创建UDP详细套接字
// socket_connect		- connect操作
// socket_connects		- 通过socket地址连接
// socket_connecto		- connect连接超时判断
// socket_connectos		- connect连接客户端然后返回socket_t句柄
// socket_accept		- accept 链接函数
//
extern socket_t socket_tcp(const char * host, uint16_t port);
extern socket_t socket_udp(const char * host, uint16_t port);
extern int socket_connect(socket_t s, const sockaddr_t * addr);
extern int socket_connects(socket_t s, const char * ip, uint16_t port);
extern int socket_connecto(socket_t s, const sockaddr_t * addr, int ms);
extern socket_t socket_connectos(const char * host, uint16_t port, int ms);
extern socket_t socket_accept(socket_t s, sockaddr_t * addr, socklen_t * len);

#endif // !_H_SIMPLEC_SCSOCKET