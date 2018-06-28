#ifndef _H_TSTR
#define _H_TSTR

#include <struct.h>

#ifndef TSTR_CREATE

struct tstr {
    size_t len;   // 长度
    size_t cap;   // 容量
    char * str;   // 字符池
};

typedef struct tstr * tstr_t;

//
// TSTR_CREATE - 栈上创建 tstr_t 结构
// TSTR_DELETE - 释放栈上 tstr_t 结构
// var  : 变量名
//
#define TSTR_CREATE(var)                                    \
struct tstr var[1] = { { 0, 0, NULL } }

#define TSTR_DELETE(var)                                    \
free((var)->str)

#endif//TSTR_CREATE

//
// tstr_delete - tstr_t 释放函数
// tsr      : 待释放的串结构
// return   : void
//
extern void tstr_delete(tstr_t tsr);

//
// tstr_expand - 为当前字符串扩容, 属于低级api
// tsr      : 可变字符串
// len      : 扩容的长度
// return   : tsr->str + tsr->len 位置的串
//
char * tstr_expand(tstr_t tsr, size_t len);

//
// tstr_t 创建函数, 会根据c的tstr串创建一个tstr_t结构的字符串
// str      : 待创建的字符串
// len      : 创建串的长度
// return   : 返回创建好的字符串,内存不足会打印日志退出程序
//
extern tstr_t tstr_create(const char * str, size_t len);
extern tstr_t tstr_creates(const char * str);

//
// 向 tstr_t 串结构中添加字符等, 内存分配失败内部会自己处理
// c        : 单个添加的char
// str      : 添加的c串
// sz       : 添加串的长度
//
extern void tstr_appendc(tstr_t tsr, int c);
extern void tstr_appends(tstr_t tsr, const char * str);
extern void tstr_appendn(tstr_t tsr, const char * str, size_t sz);

//
// tstr_cstr - 通过cstr_t串得到一个c的串以'\0'结尾
// tsr      : tstr_t 串
// return   : 返回构建好的c的串, 内存地址 tsr->str
//
extern char * tstr_cstr(tstr_t tsr);

//
// tstr_dupstr - 得到 c 的串, 需要自行 free
// tsr      : tstr_t 串
// return   : 返回创建好的c串
//
extern char * tstr_dupstr(tstr_t tsr);

//
// tstr_popup - 字符串头弹出 len 长度字符
// tsr      : 可变字符串
// len      : 弹出的长度
// return   : void
//
extern void tstr_popup(tstr_t tsr, size_t len);

//
// tstr_printf - 参照 sprintf 填充方式写入内容
// tsr      : tstr_t 串
// fmt      : 待格式化的串
// ...      : 等待进入的变量
// return   : 返回创建好的c字符串内容
//
extern char * tstr_printf(tstr_t tsr, const char * fmt, ...);

#endif//_H_TSTR
