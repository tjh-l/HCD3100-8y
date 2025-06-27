/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Generic RTC interface.
 * This version contains the part of the user interface to the Real Time Clock
 * service. It is used with both the legacy mc146818 and also  EFI
 * Struct rtc_time and first 12 ioctl by Paul Gortmaker, 1996 - separated out
 * from <linux/mc146818rtc.h> to this file for 2.4 kernels.
 *
 * Copyright (C) 1999 Hewlett-Packard Co.
 * Copyright (C) 1999 Stephane Eranian <eranian@hpl.hp.com>
 */
#ifndef _LINUX_RTC_H_
#define _LINUX_RTC_H_

#include <linux/types.h>
#include <linux/interrupt.h>
#include <uapi/linux/rtc.h>
#include <linux/timerqueue.h>

extern int rtc_month_days(unsigned int month, unsigned int year);
extern int rtc_year_days(unsigned int day, unsigned int month, unsigned int year);
extern int rtc_valid_tm(struct rtc_time *tm);
extern time64_t rtc_tm_to_time64(struct rtc_time *tm);
extern void rtc_time64_to_tm(time64_t time, struct rtc_time *tm);
ktime_t rtc_tm_to_ktime(struct rtc_time tm);
struct rtc_time rtc_ktime_to_tm(ktime_t kt);
static inline bool is_leap_year(unsigned int year)
{
	return (!(year % 4) && (year % 100)) || !(year % 400);
}

#endif /* _LINUX_RTC_H_ */
