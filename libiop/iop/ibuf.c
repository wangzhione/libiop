#include <ibuf.h>

//
// 内存阀值 -> 缩减后的内存
//
#define _UINT_CAPACITY	(64 * 1024U)
#define _UINT_SHRINK	(1024U)

ibuf_t 
ibuf_create(unsigned size) {
	ibuf_t buf = calloc(1, sizeof(struct ibuf));
	if (!buf) return NULL;
	if (size > 0) {
		buf->data = calloc(size, sizeof(char));
		if (!buf->data) {
			free(buf);
			return NULL;
		}
	}
	buf->capacity = size;
	return buf;
}

void 
ibuf_delete(ibuf_t buf) {
	if (buf) {
		if (buf->data && buf->capacity > 0)
			free(buf->data);
		free(buf);
	}
}

// 为buf扩充内存
int 
ibuf_expand(ibuf_t buf, unsigned len) {
	void * ndata;
	unsigned capc = buf->size + len;
	if (capc <= buf->capacity)
		return Success_Exist;

	ndata = realloc(buf->data, capc);
	if (ndata) {
		buf->data = ndata;
		buf->capacity = capc;
		return Success_Base;
	}
	return Error_Alloc;
}

int 
ibuf_pushback(ibuf_t buf, const void * data, unsigned len) {
	DEBUG_CODE({
		if (!buf || !data || len <= 0) {
			CERR("check '!buf || !data || len <= 0' error");
			return Error_Param;
		}
	});

	// 每次过来都需要判断是否要扩充一下内存
	if (Success_Base > ibuf_expand(buf, len))
		return Error_Alloc;

	memcpy(buf->data, data, len);
	buf->size += len;

	return Success_Base;
}

int 
ibuf_pushfront(ibuf_t buf, const void * data, unsigned len) {
	DEBUG_CODE({
		if (!buf || !data || len <= 0) {
			CERR("check '!buf || !data || len <= 0' error");
			return Error_Param;
		}
	});

	// 每次过来都需要判断是否要扩充一下内存
	if (Success_Base > ibuf_expand(buf, len))
		return Error_Alloc;

	// 先移动内存, 再填充
	memmove((char *)buf->data + len, buf->data, len);
	memcpy(buf->data, data, len);
	buf->size += len;

	return Success_Base;

}

// 内存缩水检查, 内存空间优化代码
static void _ibuf_shrink(ibuf_t buf) {
	if (buf->size == 0) {
		// 多于 64 k 缩减一下内存
		if (buf->capacity >= _UINT_CAPACITY) {
			// 大内存缩减为小内存的时候一定成功 1k
			void * ndata = realloc(buf->data, _UINT_SHRINK);
			if (ndata)
				buf->capacity = _UINT_SHRINK;
			else {
				buf->capacity = 0;
				free(buf->data);
				buf->data = NULL;
			}
		}
	}
}

int
ibuf_popback(ibuf_t buf, unsigned len) {
	DEBUG_CODE({
		if (!buf || buf->size <= 0 || len <= 0) {
			CERR("check '!buf || buf->size <= 0 || !data || len <= 0' error");
			return Error_Param;
		}
	});
	
	if (len > buf->size)
		len = buf->size;
	buf->size -= len;
	
	// 重新构建内存
	_ibuf_shrink(buf);

	return Success_Base;
}

int 
ibuf_popfront(ibuf_t buf, unsigned len) {
	DEBUG_CODE({
		if (!buf || buf->size <= 0 || len <= 0) {
			CERR("check '!buf || buf->size <= 0 || !data || len <= 0' error");
			return Error_Param;
		}
	});

	// 内存构建和移动
	if (len > buf->size)
		len = buf->size;
	else
		memmove(buf->data, (char *)buf->data + len, buf->size - len);

	buf->size -= len;

	return Success_Base;
}
