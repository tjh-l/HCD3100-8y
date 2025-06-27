#ifndef _ADAPT_SYS__STAT_H
#define _ADAPT_SYS__STAT_H

#ifdef __cplusplus
extern "C" {
#endif

int chattr(const char *path, char *attr);
int lsattr(const char *path, int *attr);

#include_next <sys/stat.h>

#ifdef __cplusplus
}
#endif

#endif
