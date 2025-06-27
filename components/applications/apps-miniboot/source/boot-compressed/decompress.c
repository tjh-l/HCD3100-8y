#include <generated/br2_autoconf.h>

unsigned long free_mem_ptr;
unsigned long free_mem_end_ptr;

/* The linker tells us where the image is. */
extern unsigned char __image_begin, __image_end;
void *memcpy(void *dest, const void *src, unsigned int n);
void cache_flush(void *src, unsigned long len, unsigned long cacheline);

void *__stack_chk_guard;
void __stack_chk_fail(void)
{
	while (1)
		;
}

void free(void *ptr)
{
	return;
}

void *malloc(unsigned int size)
{
	unsigned long *tmp = (unsigned long *)free_mem_ptr;
	free_mem_ptr += size;

	return (void *)tmp;
}

#define REG32_WRITE(_r, _v) ({ (*(volatile unsigned long *)(_r)) = (_v); })
#define REG16_READ(_r) ({ (*(volatile unsigned short *)(_r)); })

static void error(char *x)
{
}

int __decompress(unsigned char *buf, long in_len,
			      long (*fill)(void*, unsigned long),
			      long (*flush)(void*, unsigned long),
			      unsigned char *output, long out_len,
			      long *posp, unsigned long *pdecomp_len,
			      void (*error)(char *x));
void decompress_kernel(unsigned long boot_heap_start)
{
	unsigned long zimage_start, zimage_size;
	unsigned long cacheline = 16;
	unsigned long decom_zimage_len = 0;

	free_mem_ptr = boot_heap_start;
	free_mem_end_ptr = boot_heap_start + BOOT_HEAP_SIZE;

	zimage_start = (unsigned long)(&__image_begin);
	zimage_size = (unsigned long)(&__image_end) -
	    (unsigned long)(&__image_begin);

	__decompress((char *)zimage_start, zimage_size, 0, 0,
	           (void *)VMLINUX_LOAD_ADDRESS_ULL, 0, 0, &decom_zimage_len, error);

	if (((*(volatile unsigned long *)(0xb8800000)) & 0xffff0000) == 0x15120000)
		cacheline = 16;
	else if (((*(volatile unsigned long *)(0xb8800000)) & 0xffff0000) == 0x16000000)
		cacheline = 32;

	cache_flush((void *)VMLINUX_LOAD_ADDRESS_ULL, decom_zimage_len, cacheline);
}
