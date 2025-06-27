/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _LINUX_TIME64_H
#define _LINUX_TIME64_H

#include <linux/types.h>
typedef __s64 time64_t;

/*
 * This wants to go into uapi/linux/time.h once we agreed about the
 * userspace interfaces.
 */
#if __BITS_PER_LONG == 64
#define timespec64 timespec
#define itimerspec64 itimerspec
#else
struct timespec64 {
	time64_t 	tv_sec; /* seconds */
	long 		tv_nsec; /* nanoseconds */
};

struct itimerspec64 {
	struct timespec64 it_interval;
	struct timespec64 it_value;
};
#endif

/* Located here for timespec[64]_valid_strict */
#define TIME64_MAX			((s64)~((u64)1 << 63))
#define TIME64_MIN			(-TIME64_MAX - 1)

#define KTIME_MAX			((s64)~((u64)1 << 63))
#define KTIME_SEC_MAX			(KTIME_MAX / NSEC_PER_SEC)

#define ns_to_timespec64               ns_to_timespec
/*
 * Limits for settimeofday():
 *
 * To prevent setting the time close to the wraparound point time setting
 * is limited so a reasonable uptime can be accomodated. Uptime of 30 years
 * should be really sufficient, which means the cutoff is 2232. At that
 * point the cutoff is just a small part of the larger problem.
 */
#define TIME_UPTIME_SEC_MAX		(30LL * 365 * 24 *3600)
#define TIME_SETTOD_SEC_MAX		(KTIME_SEC_MAX - TIME_UPTIME_SEC_MAX)

#endif /* _LINUX_TIME64_H */
