#ifndef _H_LIBIOP_IBUF
#define _H_LIBIOP_IBUF

#include <chead.h>

typedef struct ibuf {		// 给你更多的权限, 需要自己搞
	void * data;			// 地址量
	unsigned size;			// 当前buf使用量
	unsigned capacity;		// 内存总的容量
} * ibuf_t;

extern ibuf_t ibuf_create(unsigned size);
extern void ibuf_delete(ibuf_t buf);
// 为buf扩充内存
int ibuf_expand(ibuf_t buf, unsigned len);
extern int ibuf_pushback(ibuf_t buf, const void * data, unsigned len);
extern int ibuf_pushfront(ibuf_t buf, const void * data, unsigned len);
extern int ibuf_popback(ibuf_t buf, unsigned len);
extern int ibuf_popfront(ibuf_t buf, unsigned len);

#endif // !_H_LIBIOP_IBUF
