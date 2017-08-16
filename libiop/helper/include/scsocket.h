#ifndef _H_SIMPLEC_SCSOCKET
#define _H_SIMPLEC_SCSOCKET

#include <time.h>
#include <struct.h>
#include <signal.h>

extern const char * strerr(int error);

//
// IGNORE_SIGPIPE - 管道破裂,忽略SIGPIPE信号
//
#define IGNORE_SIGNAL(sig)	signal(sig, SIG_IGN)

#ifdef __GNUC__

#include <fcntl.h>
#include <netdb.h>
#include <unistd.h>
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
#define SOCKET_ERROR			(-1)

#if !defined(EWOULDBOCK)
#define EWOULDBOCK				EAGAIN
#endif

// connect链接还在进行中, linux显示 EINPROGRESS，winds是 WSAEWOULDBLOCK
#define ECONNECTED				EINPROGRESS

#define SET_RLIMIT_NOFILE(num)				\
	do {									\
		struct rlimit $r = { num, num };	\
		setrlimit(RLIMIT_NOFILE, &$r);		\
	} while(0)

typedef int socket_t;

#elif _MSC_VER

#undef	FD_SETSIZE
#define FD_SETSIZE				(1024)
#include <ws2tcpip.h>

#define SET_RLIMIT_NOFILE(num)	

#undef	errno
#define	errno					WSAGetLastError()
#undef	strerror
#define strerror				(char *)strerr

#undef  EINTR
#define EINTR					WSAEINTR
#undef	EAGAIN
#define EAGAIN					WSAEWOULDBLOCK
#undef	EINVAL
#define EINVAL					WSAEINVAL
#undef	EWOULDBOCK
#define EWOULDBOCK				WSAEINPROGRESS
#undef	EINPROGRESS
#define EINPROGRESS				WSAEINPROGRESS
#undef	EMFILE
#define EMFILE					WSAEMFILE
#undef	ENFILE
#define ENFILE					WSAETOOMANYREFS

// connect链接还在进行中, linux显示 EINPROGRESS，winds是 WSAEWOULDBLOCK
#define ECONNECTED				WSAEWOULDBLOCK

/*
 * WinSock 2 extension -- manifest constants for shutdown()
 */
#define SHUT_RD					SD_RECEIVE
#define SHUT_WR					SD_SEND
#define SHUT_RDWR				SD_BOTH

typedef int socklen_t;
typedef SOCKET socket_t;

//
// gettimeofday - Linux sys/time.h 中得到微秒的一种实现
// tv		:	返回结果包含秒数和微秒数
// tz		:	包含的时区,在window上这个变量没有用不返回
// return	:   默认返回0
//
extern int gettimeofday(struct timeval * tv, void * tz);

//
// strerr - linux 上面替代 strerror, winds 替代 FormatMessage 
// error	: linux 是 errno, winds 可以是 WSAGetLastError() ... 
// return	: system os 拔下来的提示字符串常量
//
extern const char * strerr(int error);

#else
#	error "error : Currently only supports the Best New CL and GCC!"
#endif

// EAGAIN and EWOULDBLOCK may be not the same value.
#if (EAGAIN != EWOULDBOCK)
#define EAGAIN_WOULDBOCK EAGAIN : case EWOULDBOCK
#else
#define EAGAIN_WOULDBOCK EAGAIN
#endif

// 目前通用的tcp udp v4地址
typedef struct sockaddr_in sockaddr_t;

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