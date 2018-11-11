#include "tstr.h"

// INT_TSTR - 字符串构建的初始化大小
#define INT_TSTR  (1<<8)

//
// tstr_expand - 为当前字符串扩容, 属于低级api
// tsr      : 可变字符串
// len      : 扩容的长度
// return   : tsr->str + tsr->len 位置的串
//
char * 
tstr_expand(tstr_t tsr, size_t len) {
    size_t cap = tsr->cap;
    if ((len += tsr->len) > cap) {
        // 走 1.5 倍内存分配, '合理'降低内存占用
        cap = cap < INT_TSTR ? INT_TSTR : cap;
        while (cap < len) cap = cap * 3 / 2;
        tsr->str = realloc(tsr->str, cap);
        tsr->cap = cap;
    }
    return tsr->str + tsr->len;
}

//
// tstr_delete - tstr_t 释放函数
// tsr      : 待释放的串结构
// return   : void
//
inline void 
tstr_delete(tstr_t tsr) {
    free(tsr->str);
    free(tsr);
}

//
// tstr_t 创建函数, 根据 C 的 str 串创建 tstr_t 字符串
// str      : 待创建的字符串
// len      : 创建串的长度
// return   : 返回创建好的字符串,内存不足会打印日志退出程序
//
inline tstr_t 
tstr_create(const char * str, size_t len) {
    tstr_t tsr = calloc(1, sizeof(struct tstr));
    if (str && len) tstr_appendn(tsr, str, len);
    return tsr;
}

inline tstr_t 
tstr_creates(const char * str) {
    tstr_t tsr = calloc(1, sizeof(struct tstr));
    if (str) tstr_appends(tsr, str);
    return tsr;
}

//
// 向 tstr_t 串结构中添加字符等, 内存分配失败内部会自己处理
// c        : 单个添加的char
// str      : 添加的 C 串
// sz       : 添加串的长度
//
inline void 
tstr_appendc(tstr_t tsr, int c) {
    // 这类函数不做安全检查, 为了性能
    tstr_expand(tsr, 1);
    tsr->str[tsr->len++] = c;
}

inline void 
tstr_appends(tstr_t tsr, const char * str) {
    if (tsr && str) {
        unsigned sz = (unsigned)strlen(str);
        if (sz > 0)
            tstr_appendn(tsr, str, sz);
        tstr_cstr(tsr);
    }
}

inline void 
tstr_appendn(tstr_t tsr, const char * str, size_t sz) {
    tstr_expand(tsr, sz);
    memcpy(tsr->str + tsr->len, str, sz);
    tsr->len += sz;
}

//
// tstr_cstr - 通过 str_t 串得到一个 C 串以'\0'结尾
// tsr      : tstr_t 串
// return   : 返回构建 C 串, 内存地址 tsr->str
//
inline char * 
tstr_cstr(tstr_t tsr) {
    if (tsr->len < 1u || tsr->str[tsr->len - 1]) {
        tstr_expand(tsr, 1u);
        tsr->str[tsr->len] = '\0';
    }
    return tsr->str;
}

//
// tstr_dupstr - 得到 C 的串, 需要自行 free
// tsr      : tstr_t 串
// return   : 返回创建好的 C 串
//
inline char * 
tstr_dupstr(tstr_t tsr) {
    if (tsr && tsr->len >= 1) {
        // 构造内存, 返回最终结果
        size_t len = tsr->len + !!tsr->str[tsr->len - 1];
        char * str = malloc(len * sizeof(char));
        memcpy(str, tsr->str, len - 1);
        str[len - 1] = '\0';
        return str;
    }
    return NULL;
}

//
// tstr_popup - 字符串头弹出 len 长度字符
// tsr      : 可变字符串
// len      : 弹出的长度
// return   : void
//
inline void 
tstr_popup(tstr_t tsr, size_t len) {
    if (len >= tsr->len)
        tsr->len = 0;
    else {
        tsr->len -= len;
        memmove(tsr->str, tsr->str + len, tsr->len);
    }
}

// tstr_vprintf - BUFSIZ 以下内存处理
static int tstr_vprintf(tstr_t tsr, const char * fmt, va_list arg) {
    char buf[BUFSIZ];
    int len = vsnprintf(buf, sizeof buf, fmt, arg);
    if (len < sizeof buf) {
        // 合法直接构建内存返回
        if (len > 0)
            tstr_appendn(tsr, buf, len);
        tstr_cstr(tsr);
        va_end(arg);
    }
    return len;
}

//
// tstr_printf - 参照 sprintf 填充方式写入内容
// tsr      : tstr_t 串
// fmt      : 待格式化的串
// ...      : 等待进入的变量
// return   : 返回创建的 C 字符串内容
//
char * 
tstr_printf(tstr_t tsr, const char * fmt, ...) {
    int n, cap;
    va_list arg;
    va_start(arg, fmt);

    // 初步构建失败直接返回
    cap = tstr_vprintf(tsr, fmt, arg);
    if (cap < BUFSIZ)
        return tsr->str;
    
    // 开始详细构建内存
    do {
        char * ret = malloc(cap <<= 1);
        n = vsnprintf(ret, cap, fmt, arg);
        // 内存足够就开始填充, 以备结束
        if (n < cap && n > 0)
            tstr_appendn(tsr, ret, n);
        free(ret);
    } while (n < cap);

    va_end(arg);
    return tstr_cstr(tsr);
}
