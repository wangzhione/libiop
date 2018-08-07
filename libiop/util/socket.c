#include <socket.h>

#ifdef _MSC_VER

//
// gettimeofday - Linux sys/time.h 中得到微秒时间实现
// tv       : 返回结果包含秒数和微秒数
// tz       : 包含的时区, winds 上这个变量没有作用
// return   : 默认返回 0
//
int 
gettimeofday(struct timeval * tv, void * tz) {
    struct tm m;
    SYSTEMTIME se;

    GetLocalTime(&se);
    m.tm_year = se.wYear - 1900;
    m.tm_mon = se.wMonth - 1;
    m.tm_mday = se.wDay;
    m.tm_hour = se.wHour;
    m.tm_min = se.wMinute;
    m.tm_sec = se.wSecond;
    m.tm_isdst = -1; // 不考虑夏令时

    tv->tv_sec = (long)mktime(&m);
    tv->tv_usec = se.wMilliseconds * 1000;

    return 0;
}

#endif

//
// socket_recvn     - socket 接受 sz 个字节
// socket_sendn     - socket 发送 sz 个字节
//

int 
socket_recvn(socket_t s, void * buf, int sz) {
    int r, n = sz;
    while (n > 0) {
        r = recv(s, buf, n, 0);
        if (r == 0) break;
        if (r == SOCKET_ERROR) {
            if (errno == EINTR)
                continue;
            return SOCKET_ERROR;
        }
        n -= r;
        buf = (char *)buf + r;
    }
    return sz - n;
}

int 
socket_sendn(socket_t s, const void * buf, int sz) {
    int r, n = sz;
    while (n > 0) {
        r = send(s, buf, n, 0);
        if (r == 0) break;
        if (r == SOCKET_ERROR) {
            if (errno == EINTR)
                continue;
            return SOCKET_ERROR;
        }
        n -= r;
        buf = (char *)buf + r;
    }
    return sz - n;
}

//
// socket_addr - 通过 ip, port 构造 ipv4 结构
//
int 
socket_addr(const char * ip, uint16_t port, sockaddr_t addr) {
    addr->sin_family = AF_INET;
    addr->sin_port = htons(port);
    addr->sin_addr.s_addr = inet_addr(ip);
    if (addr->sin_addr.s_addr == INADDR_NONE) {
        struct hostent * host = gethostbyname(ip);
        if (!host || !host->h_addr)
            return EParam;

        // 尝试一种, 默认 ipv4
        memcpy(&addr->sin_addr, host->h_addr, host->h_length);
    }
    memset(addr->sin_zero, 0, sizeof addr->sin_zero);

    return SBase;
}

//
// socket_binds     - 端口绑定返回绑定好的 socket fd, 返回 INVALID_SOCKET or PF_INET PF_INET6
// socket_listens   - 端口监听返回监听好的 socket fd.
//
socket_t 
socket_binds(const char * ip, uint16_t port, uint8_t protocol, int * family) {
    socket_t fd;
    char ports[sizeof "65535"];
    struct addrinfo hint = { 0 };
    struct addrinfo * addr = NULL;
    if (NULL == ip || *ip == '\0')
        ip = "0.0.0.0"; // INADDR_ANY

    sprintf(ports, "%hu", port);
    hint.ai_family = AF_UNSPEC;
    if (protocol == IPPROTO_TCP)
        hint.ai_socktype = SOCK_STREAM;
    else {
        assert(protocol == IPPROTO_UDP);
        hint.ai_socktype = SOCK_DGRAM;
    }
    hint.ai_protocol = protocol;

    if (getaddrinfo(ip, ports, &hint, &addr))
        return INVALID_SOCKET;

    fd = socket(addr->ai_family, addr->ai_socktype, 0);
    if (fd == INVALID_SOCKET)
        goto _fail;
    if (socket_set_reuseaddr(fd))
        goto _faed;
    if (bind(fd, addr->ai_addr, (int)addr->ai_addrlen))
        goto _faed;

    // Success return ip family
    if (family)
        *family = addr->ai_family;
    freeaddrinfo(addr);
    return fd;

_faed:
    socket_close(fd);
_fail:
    freeaddrinfo(addr);
    return INVALID_SOCKET;
}

socket_t 
socket_listens(const char * ip, uint16_t port, int backlog) {
    socket_t fd = socket_binds(ip, port, IPPROTO_TCP, NULL);
    if (INVALID_SOCKET != fd && listen(fd, backlog)) {
        socket_close(fd);
        return INVALID_SOCKET;
    }
    return fd;
}

// host_parse - 解析 host 内容
static int host_parse(const char * host, char ip[BUFSIZ], uint16_t * pprt) {
    int port = 0;
    char * st = ip;

    if (!host || !*host || *host == ':')
        strcpy(ip, "0.0.0.0");
    else {
        char c;
        // 简单检查字符串是否合法
        size_t n = strlen(host);
        if (n >= BUFSIZ)
            RETURN(EParam, "host err %s", host);

        // 寻找分号
        while ((c = *host++) != ':' && c)
            *ip++ = c;
        *ip = '\0';
        if (c == ':') {
            if (n > ip - st + sizeof "65535")
                RETURN(EParam, "host port err %s", host);
            port = atoi(host);
            // 有些常识数字, 不一定是魔法 ... :)
            if (port <= 1024 || port > 65535)
                RETURN(EParam, "host port err %s, %d", host, port);
        }
    }
    *pprt = port;
    return SBase;
}

//
// socket_host - 通过 ip:port 串得到 socket addr 结构
// host     : ip:port 串
// addr     : 返回最终生成的地址
// return   : >= EBase 表示成功
//
int 
socket_host(const char * host, sockaddr_t addr) {
    uint16_t port; char ip[BUFSIZ];
    if (host_parse(host, ip, &port) < SBase)
        return EParam;

    // 开始构造 addr
    if (NULL == addr) {
        sockaddr_t nddr;
        return socket_addr(ip, port, nddr);
    }
    return socket_addr(ip, port, addr);
}

//
// socket_tcp - 创建 TCP 详细套接字
// host     : ip:port 串  
// return   : 返回监听后套接字
//
socket_t 
socket_tcp(const char * host) {
    uint16_t port; char ip[BUFSIZ];
    if (host_parse(host, ip, &port) < SBase)
        return EParam;
    return socket_listens(ip, port, SOMAXCONN);
}

//
// socket_udp - 创建 UDP 详细套接字
// host     : ip:port 串  
// return   : 返回绑定后套接字
//
socket_t 
socket_udp(const char * host) {
    uint16_t port; char ip[BUFSIZ];
    if (host_parse(host, ip, &port) < SBase)
        return EParam;
    return socket_binds(ip, port, IPPROTO_UDP, NULL);
}

//
// socket_connects - 返回链接后的阻塞套接字
// host     : ip:port 串  
// return   : 返回链接后阻塞套接字
//
socket_t 
socket_connects(const char * host) {
    sockaddr_t addr;
    socket_t s = socket_stream();
    if (INVALID_SOCKET == s) {
        RETURN(s, "socket_stream is error");
    }

    // 解析配置成功后尝试链接
    if (socket_host(host, addr) >= SBase)
        if (socket_connect(s, addr) >= SBase)
            return s;

    socket_close(s);
    RETURN(INVALID_SOCKET, "socket_connects %s", host);
}

//
// socket_connecto      - connect 超时链接, 返回非阻塞 socket
//
int socket_connecto(socket_t s, const sockaddr_t addr, int ms) {
    int n, r;
    struct timeval to;
    fd_set rset, wset, eset;

    // 还是阻塞的connect
    if (ms < 0) return socket_connect(s, addr);

    // 非阻塞登录, 先设置非阻塞模式
    r = socket_set_nonblock(s);
    if (r < SBase) return r;

    // 尝试连接, connect 返回 -1 并且 errno == EINPROGRESS 表示正在建立链接
    r = socket_connect(s, addr);
    // connect 链接中, linux 是 EINPROGRESS，winds 是 WSAEWOULDBLOCK
    if (r >= SBase || errno != EINPROGRESS) {
        socket_set_block(s);
        return r;
    }

    // 超时 timeout, 直接返回结果 EBase = -1 错误
    if (ms == 0) {
        socket_set_block(s);
        return EBase;
    }

    FD_ZERO(&rset); FD_SET(s, &rset);
    FD_ZERO(&wset); FD_SET(s, &wset);
    FD_ZERO(&eset); FD_SET(s, &eset);
    to.tv_sec = ms / 1000;
    to.tv_usec = (ms % 1000) * 1000;
    n = select((int)s + 1, &rset, &wset, &eset, &to);
    // 超时直接滚
    if (n <= 0) {
        socket_set_block(s);
        return EBase;
    }

    // 当连接成功时候,描述符会变成可写
    if (n == 1 && FD_ISSET(s, &wset)){
        socket_set_block(s);
        return SBase;
    }

    // 当连接建立遇到错误时候, 描述符变为即可读又可写
    if (FD_ISSET(s, &eset) || n == 2) {
        socklen_t len = sizeof n;
        // 只要最后没有 error 那就 链接成功
        if (!getsockopt(s, SOL_SOCKET, SO_ERROR, (char *)&n, &len) && !n)
            r = SBase;
    }
    socket_set_block(s);
    return r;
}

//
// socket_connectos - 返回链接后的非阻塞套接字
// host     : ip:port 串  
// ms       : 链接过程中毫秒数
// return   : 返回链接后非阻塞套接字
//
socket_t 
socket_connectos(const char * host, int ms) {
    sockaddr_t addr;
    socket_t s = socket_stream();
    if (INVALID_SOCKET == s) {
        RETURN(s, "socket_stream is error");
    }

    // 解析配置成功后尝试链接
    if (socket_host(host, addr) >= SBase)
        if (socket_connecto(s, addr, ms) >= SBase)
            return s;

    socket_close(s);
    RETURN(INVALID_SOCKET, "socket_connectos %s", host);
}
