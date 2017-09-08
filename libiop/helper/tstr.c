#include <tstr.h>

//-----------------------------------字符串相关的协助API -------------------------------

//
// Brian Kernighan与 Dennis Ritchie 简便快捷的 hash算法
// str		: 字符串内容
// return	: 返回计算后的hash值
//
unsigned 
tstr_hash(const char * str) {
	register unsigned h = 0;
	if (str) {
		register unsigned c;
		while ((c = *str++))
			h = h * 131 + c;
	}
	return h;
}

//
// 字符串不区分大小写比较函数
// ls		: 左串
// rs		: 右串
// return	: ls > rs 返回 > 0; ... < 0; ... =0
//
int 
tstr_icmp(const char * ls, const char * rs) {
	int l, r;
	if (!ls || !rs) 
		return (int)(ls - rs);

	do {
		if ((l = *ls++) >= 'a' && l <= 'z')
			l -= 'a' - 'A';
		if ((r = *rs++) >= 'a' && r <= 'z')
			r -= 'a' - 'A';
	} while (l && l == r);

	return l - r;
}

//
// 字符串拷贝函数, 需要自己free
// str		: 待拷贝的串
// return	: 返回拷贝后的串
//
char * 
tstr_dup(const char * str) {
	size_t len;
	char * nstr;
	if (NULL == str) 
		return NULL;

	len = strlen(str) + 1;
	nstr = malloc(sizeof(char) * len);
	//
	// 补充一下, 关于 malloc的写法, 说不尽道不完. 
	// 这里采用 日志 + exit, 这种未定义行为. 方便收集错误日志和监测大内存申请失败情况.
	//
	if (NULL == nstr)
		CERR_EXIT("malloc len = %zu is empty!", len);

	return memcpy(nstr, str, len);
}

//
// 简单的文件读取类,会读取完毕这个文件内容返回,失败返回NULL.
// path		: 文件路径
// return	: 创建好的字符串内容, 返回NULL表示读取失败
//
tstr_t 
tstr_freadend(const char * path) {
	int err;
	size_t rn;
	tstr_t tstr;
	char buf[BUFSIZ];
	FILE * txt = fopen(path, "rb");
	if (NULL == txt) {
		RETURN(NULL, "fopen r %s is error!", path);
	}

	// 分配内存
	tstr = tstr_creates(NULL);

	// 读取文件内容
	do {
		rn = fread(buf, sizeof(char), BUFSIZ, txt);
		if ((err = ferror(txt))) {
			fclose(txt);
			tstr_delete(tstr);
			RETURN(NULL, "fread err path = %d, %s.", err, path);
		}
		// 保存构建好的数据
		tstr_appendn(tstr, buf, rn);
	} while (rn == BUFSIZ);

	fclose(txt);

	// 继续构建数据, 最后一行补充一个\0
	tstr_cstr(tstr);

	return tstr;
}

static int _tstr_fwrite(const char * path, const char * str, const char * mode) {
	FILE * txt;
	if (!path || !*path || !str) {
		RETURN(ErrParam, "check !path || !*path || !str'!!!");
	}

	// 打开文件, 写入消息, 关闭文件
	if ((txt = fopen(path, mode)) == NULL) {
		RETURN(ErrFd, "fopen mode = '%s', path = '%s' error!", mode, path);
	}
	fputs(str, txt);
	fclose(txt);

	return SufBase;
}

//
// 将c串str覆盖写入到path路径的文件中
// path		: 文件路径
// str		: c的串内容
// return	: SufBase | ErrParam | ErrFd
//
inline int 
tstr_fwrites(const char * path, const char * str) {
	return _tstr_fwrite(path, str, "wb");
}

//
// 将c串str写入到path路径的文件中末尾
// path		: 文件路径
// str		: c的串内容
// return	: SufBase | ErrParam | ErrFd
//
inline int 
tstr_fappends(const char * path, const char * str) {
	return _tstr_fwrite(path, str, "ab");
}

//-----------------------------------字符串相关的核心API -------------------------------

//
// tstr_t 创建函数, 会根据c的tstr串创建一个 tstr_t结构的字符串
// str		: 待创建的字符串
// len		: 创建串的长度
// return	: 返回创建好的字符串,内存不足会打印日志退出程序
//
tstr_t 
tstr_create(const char * str, size_t len) {
	tstr_t tstr = calloc(1, sizeof(struct tstr));
	if (NULL == tstr)
		CERR_EXIT("malloc sizeof struct tstr is error!");
	if (str && len > 0)
		tstr_appendn(tstr, str, len);
	return tstr;
}

tstr_t
tstr_creates(const char * str) {
	tstr_t tstr = calloc(1, sizeof(struct tstr));
	if (NULL == tstr)
		CERR_EXIT("malloc sizeof struct tstr is error!");
	if(str)
		tstr_appends(tstr, str);
	return tstr;
}

//
// tstr_t 释放函数, 不处理多释放问题
// tstr		: 待释放的串结构
//
inline void 
tstr_delete(tstr_t tstr) {
	free(tstr->str);
	free(tstr);
}

// 文本字符串创建的初始化大小
#define _UINT_TSTR		(32u)

//
// tstr_expand - 为当前字符串扩容, 属于低级api
// tstr		: 可变字符串
// len		: 扩容的长度
// return	: tstr->str + tstr->len 位置的串
//
char *
tstr_expand(tstr_t tstr, size_t len) {
	size_t cap = tstr->cap;
	if ((len += tstr->len) >= cap) {
		char * nstr;
		for (cap = cap < _UINT_TSTR ? _UINT_TSTR : cap; cap < len; cap <<= 1)
			;
		// 开始分配内存
		if ((nstr = realloc(tstr->str, cap)) == NULL) {
			tstr_delete(tstr);
			CERR_EXIT("realloc cap = %zu empty!!!", cap);
		}

		// 重新内联内存
		tstr->str = nstr;
		tstr->cap = cap;
	}

	return tstr->str + tstr->len;
}

//
// 向tstr_t串结构中添加字符等, 内存分配失败内部会自己处理
// c		: 单个添加的char
// str		: 添加的c串
// sz		: 添加串的长度
//
inline void 
tstr_appendc(tstr_t tstr, int c) {
	// 这类函数不做安全检查, 为了性能
	tstr_expand(tstr, 1);
	tstr->str[tstr->len++] = c;
}

void 
tstr_appends(tstr_t tstr, const char * str) {
	size_t len;
	if (!tstr || !str) {
		RETURN(NIL, "check '!tstr || !str' param is error!");
	}

	len = strlen(str);
	if(len > 0)
		tstr_appendn(tstr, str, len);
	tstr_cstr(tstr);
}

inline void 
tstr_appendn(tstr_t tstr, const char * str, size_t sz) {
	tstr_expand(tstr, sz);
	memcpy(tstr->str + tstr->len, str, sz);
	tstr->len += sz;
}

//
// tstr_popup - 从字符串头弹出len长度字符
// tstr		: 可变字符串
// len		: 弹出的长度
// return	: void
//
inline void 
tstr_popup(tstr_t tstr, size_t len) {
	if (len > tstr->len)
		tstr->len = 0;
	else {
		tstr->len -= len;
		memmove(tstr->str, tstr->str + len, tstr->len);
	}
}

//
// 得到一个精简的c的串, 需要自己事后free
// tstr		: tstr_t 串
// return	: 返回创建好的c串
//
char * 
tstr_dupstr(tstr_t tstr) {
	size_t len;
	char * str;
	if (!tstr || tstr->len < 1)
		return NULL;

	// 构造内存, 最终返回结果
	len = tstr->len + !!tstr->str[tstr->len - 1];
	str = malloc(len * sizeof(char));
	if (NULL == str)
		CERR_EXIT("malloc len = %zu is error!", len);
	
	memcpy(str, tstr->str, len - 1);
	str[len - 1] = '\0';

	return str;
}

//
// 通过cstr_t串得到一个c的串以'\0'结尾
// tstr		: tstr_t 串
// return	: 返回构建好的c的串, 内存地址tstr->str
//
inline char * 
tstr_cstr(tstr_t tstr) {
	// 本质是检查最后一个字符是否为 '\0'
	if (tstr->len < 1 || tstr->str[tstr->len - 1]) {
		tstr_expand(tstr, 1);
		tstr->str[tstr->len] = '\0';
	}

	return tstr->str;
}