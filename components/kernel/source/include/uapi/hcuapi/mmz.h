#ifndef _HCUAPI_MMZ_H_
#define _HCUAPI_MMZ_H_

#include <hcuapi/iocbase.h>

#define MMZ_NR_POOL_MMZ		(10)
#define MMZ_NR_POOL		(MMZ_NR_POOL_MMZ + 1)
#define MMZ_ID_SYSMEM		(MMZ_NR_POOL - 1)

struct mmz_blk {
	int id;			//<! mmz ID to request memory
	size_t alignment;	//<! alignment of memory to request
	int size;		//<! Size of memory to request
	uint32_t addr;		//<! Address to return or address to free
};

#define MMZ_TOTAL		_IOWR(MMZ_IOCBASE,  0, struct mmz_blk)
#define MMZ_MEMALIGN		_IOWR(MMZ_IOCBASE,  1, struct mmz_blk)
#define MMZ_FREE		_IOWR(MMZ_IOCBASE,  2, struct mmz_blk)
#define MMZ_FLUSH_CACHE		_IOWR(MMZ_IOCBASE,  3, struct mmz_blk)

size_t system_mem_total(void);
size_t system_mem_free(void);
size_t mem_total(void);
size_t mmz_total(int id);
void mmz_free(int id, void *ptr);
void *mmz_memalign(int id, size_t alignment, size_t size);

#ifdef __HCRTOS__
void *mmz_malloc(int id, size_t size);
void *mmz_zalloc(int id, size_t size);
void *mmz_calloc(int id, size_t nmemb, size_t size);
void *mmz_realloc(int id, void *ptr, size_t size);
int mmz_create(void *start, size_t size);
int mmz_delete(int id);
int mmz_name2id(const char *name);
size_t mmz_available(int id);
#endif

#endif /* _HCUAPI_MMZ_H_ */
