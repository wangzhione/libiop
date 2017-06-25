#ifndef _H_SIMPLEC_TSTR
#define _H_SIMPLEC_TSTR

#include <struct.h>

#ifndef _STRUCT_TSTR

struct tstr {
	char * str;			// 字符串实际保存的内容
	size_t len;			// 当前字符串长度
	size_t cap;			// 字符池大小
};

// 定义的字符串类型
typedef struct tstr * tstr_t;

#define _STRUCT_TSTR
#endif

//文本串栈上创建内容,不想用那些技巧了,就这样吧
#define TSTR_CREATE(var) \
	struct tstr var[1] = { NULL }
#define TSTR_DELETE(var) \
	free((var)->str)

//-----------------------------------字符串相关的协助API -------------------------------

//
// 采用jshash算法,计算字符串返回后的hash值,碰撞率为80%
// str		: 字符串内容
// return	: 返回计算后的hash值
//
extern unsigned tstr_hash(const char * str);

//
// 字符串不区分大小写比较函数
// ls		: 左串
// rs		: 右串
// return	: ls > rs 返回 > 0; ... < 0; ... =0
//
extern int tstr_icmp(const char * ls, const char * rs);

//
// 字符串拷贝函数, 需要自己free
// str		: 待拷贝的串
// return	: 返回拷贝后的串
//
extern char * tstr_dup(const char * str);

//
// 简单的文件读取类,会读取完毕这个文件内容返回,失败返回NULL.
// path		: 文件路径
// return	: 创建好的字符串内容, 返回NULL表示读取失败
//
extern tstr_t tstr_freadend(const char * path);

//
// 将c串str覆盖写入到path路径的文件中
// path		: 文件路径
// str		: c的串内容
// return	: Success_Base | Error_Param | Error_Fd
//
extern flag_e tstr_fwrites(const char * path, const char * str);

//
// 将c串str写入到path路径的文件中末尾
// path		: 文件路径
// str		: c的串内容
// return	: Success_Base | Error_Param | Error_Fd
//
extern flag_e tstr_fappends(const char * path, const char * str);

//-----------------------------------字符串相关的核心API -------------------------------

//
// tstr_t 创建函数, 会根据c的tstr串创建一个 tstr_t结构的字符串
// str		: 待创建的字符串
// len		: 创建串的长度
// return	: 返回创建好的字符串,内存不足会打印日志退出程序
//
extern tstr_t tstr_create(const char * str, size_t len);
extern tstr_t tstr_creates(const char * str);

//
// tstr_t 释放函数
// tstr		: 待释放的串结构
//
extern void tstr_delete(tstr_t tstr);

//
// tstr_expand - 为当前字符串扩容, 属于低级api
// tstr		: 可变字符串
// len		: 扩容的长度
// return	: void
//
void tstr_expand(tstr_t tstr, size_t len);

//
// 向tstr_t串结构中添加字符等, 内存分配失败内部会自己处理
// c		: 单个添加的char
// str		: 添加的c串
// sz		: 添加串的长度
//
extern void tstr_appendc(tstr_t tstr, int c);
extern void tstr_appends(tstr_t tstr, const char * str);
extern void tstr_appendn(tstr_t tstr, const char * str, size_t sz);

//
// tstr_popup - 从字符串头弹出len长度字符
// tstr		: 可变字符串
// len		: 弹出的长度
// return	: void
//
extern void tstr_popup(tstr_t tstr, size_t len);

//
// 得到一个精简的c的串, 需要自己事后free
// tstr		: tstr_t 串
// return	: 返回创建好的c串
//
extern char * tstr_dupstr(tstr_t tstr);

//
// 通过cstr_t串得到一个c的串以'\0'结尾
// tstr		: tstr_t 串
// return	: 返回构建好的c的串, 内存地址tstr->str
//
extern char * tstr_cstr(tstr_t tstr);

#endif // !_H_SIMPLEC_TSTR