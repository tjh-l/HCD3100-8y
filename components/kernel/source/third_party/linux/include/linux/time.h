#ifndef _LINUX_TIME_H
#define _LINUX_TIME_H

# include <linux/math64.h>
# include <linux/time64.h>

extern time64_t mktime64(const unsigned int year, const unsigned int mon,
                        const unsigned int day, const unsigned int hour,
                        const unsigned int min, const unsigned int sec);

#endif
