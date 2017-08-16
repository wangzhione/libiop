#ifndef _H_SIMPLEC_VLIST
#define _H_SIMPLEC_VLIST

#include <struct.h> 

struct vlist {
	struct vlist * next;
	void * data;
};

typedef struct vlist * vlist_t;

//
// vlist_add - 添加一个新的结点, 返回头结点
// list		: 链表对象
// data		: 待插入数据
// return	: 返回插入后的头结点
//
extern vlist_t vlist_add(vlist_t list, void * data);

//
// vlist_delete_ - 销毁vlist对象
// list		: 对象链表对象
// die		: 销毁结点, NULL 表示不销毁
// free		: 默认的销毁函数
// return	: void
//
extern void vlist_delete_(vlist_t list, node_f die);
#define vlist_delete(list) \
	do {\
		vlist_delete_(list, free);\
		list = NULL;\
	} while(0)

//
// vlist_each - 遍历其中每个对象并处理
// list		: 对象链表对象
// echo		: 轮询执行的函数结点
// return	: void
//
extern void vlist_each(vlist_t list, node_f echo);

#endif // !_H_SIMPLEC_VLIST