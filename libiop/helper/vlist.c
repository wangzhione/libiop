#include <vlist.h>

//
// vlist_add - 添加一个新的结点, 返回头结点
// list		: 链表对象
// arg		: 待插入数据
// return	: 返回插入后的头结点
//
vlist_t 
vlist_add(vlist_t list, void * data) {
	struct vlist * node = malloc(sizeof(struct vlist));
	if (NULL == node) {
		RETURN(list, "malloc struct vlist is error");
	}
	node->data = data;
	node->next = list;

	return node;
}

//
// vlist_delete_ - 销毁vlist对象
// list		: 对象链表对象
// die		: 销毁结点, NULL 表示不销毁
// free		: 默认的销毁函数
// return	: void
//
void 
vlist_delete_(vlist_t list, die_f die) {
	while (list) {
		vlist_t next = list->next;
		if (die)
			die(list->data);
		free(list);
		list = next;
	}
}

//
// vlist_each - 遍历其中每个对象并处理
// list		: 对象链表对象
// echo		: 轮询执行的函数结点
// return	: void
//
void 
vlist_each(vlist_t list, die_f echo) {
	while (list) {
		echo(list->data);
		list = list->next;
	}
}