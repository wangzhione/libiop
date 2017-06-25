#ifndef _H_SIMPLEC_STRUCT
#define _H_SIMPLEC_STRUCT

//
// 数据结构从这个头文件集成开始
// 构建统一的标识文件, 构建统一的注册函数, 构建统一基础头文件
//
// author : wz
//

#include <ctype.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <assert.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdarg.h>
#include <stdbool.h>
#include <inttypes.h>

// 
// 控制台输出完整的消息提示信息, 其中fmt必须是 "" 包裹的字符串
// CERR			-> 简单的消息打印
// CERR_EXIT	-> 输出错误信息, 并推出当前进程
// CERR_IF		-> if语句检查, 如果符合标准错误直接退出
// 
#ifndef _H_CERR
#define _H_CERR

#define CERR(fmt, ...) \
	fprintf(stderr, "[%s:%s:%d][errno %d:%s]" fmt "\n",\
		__FILE__, __func__, __LINE__, errno, strerror(errno), ##__VA_ARGS__)

#define CERR_EXIT(fmt,...) \
	CERR(fmt, ##__VA_ARGS__), exit(EXIT_FAILURE)

#define CERR_IF(code) \
	if((code) < 0) \
		CERR_EXIT(#code)

//
// RETURN - 打印错误信息, 并return 返回指定结果
// val		: return的东西, 当需要 return void; 时候填 ',' 就过 or NIL
// fmt		: 双引号包裹的格式化字符串
// ...		: fmt中对应的参数
// return	: val
// 
#define NIL
#define RETURN(val, fmt, ...) \
	do {\
		CERR(fmt, ##__VA_ARGS__);\
		return val;\
	} while(0)


#endif

/*
 * 这里是一个 在 DEBUG 模式下的测试宏
 *
 * 用法 :
 * DEBUG_CODE({
 *		puts("debug start...");
 * });
 */
#ifndef DEBUG_CODE
# ifdef _DEBUG
#	define DEBUG_CODE(code) \
		do code while(0)
# else
#	define DEBUG_CODE(code) 
# endif	//	! _DEBUG
#endif	//	! DEBUG_CODE

#ifndef _ENUM_FLAG
//
// flag_e - 全局操作基本行为返回的枚举, 用于判断返回值状态的状态码
// >= 0 标识 Success状态, < 0 标识 Error状态
//
typedef enum {
	Success_Exist	= +2,			//希望存在,设置之前已经存在了.
	Success_Close	= +1,			//文件描述符读取关闭, 读取完毕也会返回这个
	Success_Base	= +0,			//结果正确的返回宏

	Error_Base		= -1,			//错误基类型, 所有错误都可用它, 在不清楚的情况下
	Error_Param		= -2,			//调用的参数错误
	Error_Alloc		= -3,			//内存分配错误
	Error_Fd		= -4,			//文件打开失败
	Error_TOUT		= -5,			//超时错误
} flag_e;

//
// 定义一些通用的函数指针帮助, 主要用于基库的封装.
// 有构造函数, 析构函数, 比较函数, 轮询函数 ... 
// cmp_f	- int cmp(const void * ln, const void * rn); 标准结构
// each_f	- flag_e <=> int, echh循环操作, arg 外部参数, node 内部节点
// start_f	- pthread 线程启动的辅助函数宏, 方便优化
//
typedef int (* cmp_f)();
typedef void * (* new_f)();
typedef void (* die_f)(void * node);
typedef flag_e (* each_f)(void * node, void * arg);
typedef void * (* start_f) (void * arg);

#define _ENUM_FLAG
#endif // !_ENUM_FLAG

#endif // !_H_SIMPLEC_STRUCT