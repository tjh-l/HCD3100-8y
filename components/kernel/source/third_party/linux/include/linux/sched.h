/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _LINUX_SCHED_H
#define _LINUX_SCHED_H

#include <sched.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <linux/kthread.h>
#include <assert.h>

#define TASK_RUNNING		0
#define TASK_INTERRUPTIBLE	1
#define TASK_UNINTERRUPTIBLE	2

void schedule(void);
int cond_resched(void);
#define yield() taskYIELD()
#define	MAX_SCHEDULE_TIMEOUT	portMAX_DELAY

/* sched_setscheduler not implemented */
#define sched_setscheduler(p,policy,param)                                                \
	kthread_sched_setscheduler(p,policy,param,true)
	
/*
 * Scheduling policies
 */
#define SCHED_NORMAL		0
#define SCHED_FIFO		1
#define SCHED_RR		2
#define SCHED_BATCH		3
		/* SCHED_ISO: reserved but not implemented yet */
#define SCHED_IDLE		5
#define SCHED_DEADLINE		6

void set_current_state(long state_value);

#define __set_current_state(state_value) set_current_state(state_value)

/* Signal mechanism not implemented */
#define signal_pending(x) 0
#define flush_signals(x) do {} while (0)
#define allow_signal(x) do {} while (0)

signed long schedule_timeout(signed long timeout);

extern int wake_up_process(struct task_struct *tsk);
extern int kthread_sched_setscheduler(struct task_struct *p, int policy,
			  const struct sched_param *param, bool check);

static int find_vpid(int nr)
{
	return nr;
}

static inline int kill_pid(int pid, int sig, int priv)
{
	/* TODO, check how this API is called */
	assert(0);
}

#endif
