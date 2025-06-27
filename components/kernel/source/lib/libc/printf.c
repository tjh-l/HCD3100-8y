#include <reent.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdarg.h>
#include <linux/mutex.h>

int __wrap_printf (const char *__restrict fmt, ...)
{
	taskENTER_CRITICAL();
	int ret;
	va_list ap;
	struct _reent *ptr = _REENT;

	_REENT_SMALL_CHECK_INIT(ptr);
	va_start(ap, fmt);
	ret = _vfprintf_r(ptr, _stdout_r(ptr), fmt, ap);
	va_end(ap);
	taskEXIT_CRITICAL();
	return ret;
}
