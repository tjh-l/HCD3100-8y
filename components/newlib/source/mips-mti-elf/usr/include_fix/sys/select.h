#ifndef _ADAPT_SYS__SELECT_H
#define _ADAPT_SYS__SELECT_H

#ifdef __cplusplus
extern "C" {
#endif

#	define	FD_SETSIZE	1024

#include_next <sys/select.h>

#ifdef __cplusplus
}
#endif

#endif
