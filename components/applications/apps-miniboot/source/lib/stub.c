#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <assert.h>
#include <kernel/io.h>
#include <hcuapi/gpio.h>
#include <cpu_func.h>
#include "ctype.h"

void *malloc(size_t size);
void *memcpy(void *dst0, const void *src0, size_t len0);
size_t strlen (const char *str);

#define configISR_STACK_SIZE 0x1000
uint32_t xISRStack[configISR_STACK_SIZE] = { 0 };
const uint32_t *const xISRStackTop = &(xISRStack[configISR_STACK_SIZE - 7]);

uint32_t uxInterruptNesting = 0x0;

int gpio_config_irq(pinpad_e padctl, gpio_pinset_t pinset)
{
	return 0;
}

void _osGeneralExceptHdl(void)
{
}

void vTaskExitCritical( void )
{
}

void vTaskEnterCritical(void)
{
}

__attribute__((noinline)) void __assert_func(const char *file, int line, const char *func,
		   const char *failedexpr)
{
	while (1)
		;
}

void irq_save_all(void)
{
}

int usleep(unsigned long us)
{
	uint32_t ctime = sys_time_from_boot();
	us = (us + 999) / 1000;
	while (sys_time_from_boot() < (ctime + us))
		;
}

void __attribute__((__constructor__)) __stack_chk_init (void)
{
}

int strcmp(const char *s1, const char *s2)
{
	while (*s1 != '\0' && *s1 == *s2) {
		s1++;
		s2++;
	}

	return (*(unsigned char *)s1) - (*(unsigned char *)s2);
}

char *strdup(const char *str)
{
	size_t len = strlen(str) + 1;
	char *copy = malloc(len);
	if (copy) {
		memcpy(copy, str, len);
	}
	return copy;
}

void *memcpy(void *dst0, const void *src0, size_t len0)
{
	char *dst = (char *)dst0;
	char *src = (char *)src0;

	void *save = dst0;

	while (len0--) {
		*dst++ = *src++;
	}

	return save;
}

size_t strlen (const char *str)
{
	const char *start = str;
	while (*str)
		str++;
	return str - start;
}

int strncmp(const char *s1, const char *s2, size_t n)
{
	if (n == 0)
		return 0;

	while (n-- != 0 && *s1 == *s2) {
		if (n == 0 || *s1 == '\0')
			break;
		s1++;
		s2++;
	}

	return (*(unsigned char *)s1) - (*(unsigned char *)s2);
}

void *memset(void *m, int c, size_t n)
{
	char *s = (char *)m;
	while (n--)
		*s++ = (char)c;

	return m;
}

int memcmp(const void *m1, const void *m2, size_t n)
{
	unsigned char *s1 = (unsigned char *)m1;
	unsigned char *s2 = (unsigned char *)m2;

	while (n--) {
		if (*s1 != *s2) {
			return *s1 - *s2;
		}
		s1++;
		s2++;
	}
	return 0;
}

char *strncpy(char *dst0, const char *src0, size_t count)
{
	char *dscan;
	const char *sscan;

	dscan = dst0;
	sscan = src0;
	while (count > 0) {
		--count;
		if ((*dscan++ = *sscan++) == '\0')
			break;
	}
	while (count-- > 0)
		*dscan++ = '\0';

	return dst0;
}

void *memmove(void *dst_void, const void *src_void, size_t length)
{
	char *dst = dst_void;
	const char *src = src_void;

	if (src < dst && dst < src + length) {
		/* Have to copy backwards */
		src += length;
		dst += length;
		while (length--) {
			*--dst = *--src;
		}
	} else {
		while (length--) {
			*dst++ = *src++;
		}
	}

	return dst_void;
}

char *strchr(const char *s1, int i)
{
	const unsigned char *s = (const unsigned char *)s1;
	unsigned char c = i;

	while (*s && *s != c)
		s++;
	if (*s == c)
		return (char *)s;
	return NULL;
}

char *strrchr(const char *s, int i)
{
	const char *last = NULL;

	if (i) {
		while ((s = strchr(s, i))) {
			last = s;
			s++;
		}
	} else {
		last = strchr(s, i);
	}

	return (char *)last;
}

static const char *_parse_integer_fixup_radix(const char *s, unsigned int *base)
{
	if (*base == 0) {
		if (s[0] == '0') {
			if (tolower(s[1]) == 'x' && isxdigit(s[2]))
				*base = 16;
			else
				*base = 8;
		} else
			*base = 10;
	}
	if (*base == 16 && s[0] == '0' && tolower(s[1]) == 'x')
		s += 2;
	return s;
}

unsigned long strtoul(const char *cp, char **endp,
				unsigned int base)
{
	unsigned long result = 0;
	unsigned long value;

	cp = _parse_integer_fixup_radix(cp, &base);

	while (isxdigit(*cp) && (value = isdigit(*cp) ? *cp-'0' : (islower(*cp)
	    ? toupper(*cp) : *cp)-'A'+10) < base) {
		result = result*base + value;
		cp++;
	}

	if (endp)
		*endp = (char *)cp;

	return result;
}

void *memchr(const void *src_void, int c, size_t length)
{
	const unsigned char *src = (const unsigned char *)src_void;
	unsigned char d = c;

	while (length--) {
		if (*src == d)
			return (void *)src;
		src++;
	}

	return NULL;
}

extern ssize_t sci_write(const char *buffer, size_t buflen);
int puts(const char *s)
{
	while ((*s) != '\0') {
		sci_write(s, 1);
		s++;
	}
	return 0;
}
