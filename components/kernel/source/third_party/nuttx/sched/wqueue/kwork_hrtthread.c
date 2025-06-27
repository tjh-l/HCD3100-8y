/****************************************************************************
 * sched/wqueue/kwork_hrtthread.c
 *
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.  The
 * ASF licenses this file to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance with the
 * License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
 * License for the specific language governing permissions and limitations
 * under the License.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/
#define ELOG_OUTPUT_LVL ELOG_LVL_ERROR

#include <nuttx/config.h>

#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <nuttx/queue.h>

#include <nuttx/wqueue.h>

#include "wqueue.h"

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <kernel/vfs.h>
#include <kernel/module.h>

#define LOG_TAG "WQUEUE"
#include <kernel/elog.h>
#include <kernel/ld.h>
#include <kernel/soc/soc_common.h>

#ifdef CONFIG_SCHED_HRTWORK

timer2_reg_t *gHRTimer = (timer2_reg_t *)&TIMER20;

/****************************************************************************
 * Public Data
 ****************************************************************************/

/* The state of the kernel mode, high priority work queue(s). */

struct hrt_wqueue_s g_hrtwork;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: work_hrtthread
 *
 * Description:
 *   These are the worker threads that performs the actions placed on the
 *   high priority work queue.
 *
 *   These, along with the lower priority worker thread(s) are the kernel
 *   mode work queues (also build in the flat build).
 *
 *   All kernel mode worker threads are started by the OS during normal
 *   bring up.  This entry point is referenced by OS internally and should
 *   not be accessed by application logic.
 *
 * Input Parameters:
 *   argc, argv (not used)
 *
 * Returned Value:
 *   Does not return
 *
 ****************************************************************************/

static void work_hrtthread(void *arg)
{
  int wndx = 0;
#if CONFIG_SCHED_HRTNTHREADS > 1
  TaskHandle_t me = xTaskGetCurrentTaskHandle();
  int i;

  /* Find out thread index by search the workers in g_hrtwork */

  for (wndx = 0, i = 0; i < CONFIG_SCHED_HRTNTHREADS; i++)
    {
      if (g_hrtwork.worker[i].pid == me)
        {
          wndx = i;
          break;
        }
    }

  DEBUGASSERT(i < CONFIG_SCHED_HRTNTHREADS);
#endif

  /* Loop forever */

  for (; ; )
    {
      /* Then process queued work.  work_process will not return until: (1)
       * there is no further work in the work queue, and (2) signal is
       * triggered, or delayed work expires.
       */

      work_process((FAR struct kwork_wqueue_s *)&g_hrtwork, wndx, HRTWORK);
    }

  vTaskDelete(NULL);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/
static void PortSysHRTimerIntHandler(uint32_t param)
{
  BaseType_t xHigherPriorityTaskWoken = pdFALSE, xResult;
  (void) param;

  if (gHRTimer->ctrl.overflow)
    {
      gHRTimer->cnt.val = 0;
      gHRTimer->ctrl.overflow = 1;
      gHRTimer->ctrl.en = 0;
      gHRTimer->ctrl.int_en = 0;
      xResult = xEventGroupSetBitsFromISR(g_work_event, HRTWORK, &xHigherPriorityTaskWoken);
      if (xResult == pdPASS)
        {
          portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
        }
    }
}
 
static void work_hrtimer_init(void)
{
  gHRTimer->cnt.val = 0;
  gHRTimer->ctrl.val = 0;
  gHRTimer->ctrl.overflow = 1;
  xPortInterruptInstallISR(SB_TIMER_IRQ, PortSysHRTimerIntHandler, 0);
}

#define usec2tick(usec) ((usec)*27 / 32)
#define tick2usec(tick) ((tick)*32 / 27)
void work_hrtimer_start(clock_t delay)
{
  if(delay < 2)
      delay = 2;
  gHRTimer->cnt.val = 0 - usec2tick(delay);
  /* enable timer and its interrupt */
  gHRTimer->ctrl.en = 1;
  gHRTimer->ctrl.int_en = 1;
}

/****************************************************************************
 * Name: work_start_hrtimer
 *
 * Description:
 *   Start the high-priority, kernel-mode worker thread(s)
 *
 * Input Parameters:
 *   None
 *
 * Returned Value:
 *   The task ID of the worker thread is returned on success.  A negated
 *   errno value is returned on failure.
 *
 ****************************************************************************/

static int work_start_hrtimer(void)
{
  TaskHandle_t pid = NULL;
  int wndx;
  char name[32] = { 0 };

  /* Don't permit any of the threads to run until we have fully initialized
   * g_hrtwork.
   */

  taskENTER_CRITICAL();

  /* Start the high-priority, kernel mode worker thread(s) */

  log_i("Starting High Resolution Timer kernel worker thread(s)\n");

  for (wndx = 0; wndx < CONFIG_SCHED_HRTNTHREADS; wndx++)
    {
      snprintf(name, sizeof(name), "%s%d", HRTWORKNAME, wndx);
      xTaskCreate( work_hrtthread, name, CONFIG_SCHED_HRTWORKSTACKSIZE,
                  NULL, CONFIG_SCHED_HRTWORKPRIORITY, &pid );

      if (pid == NULL)
        {
          log_e("ERROR: kthread_create %d failed: %p\n", wndx, pid);
          taskEXIT_CRITICAL();
          return (int)pid;
        }

      g_hrtwork.worker[wndx].pid  = pid;
      g_hrtwork.worker[wndx].busy = true;
      init_completion(&(g_hrtwork.worker[wndx].completion));
    }

  work_hrtimer_init();

  taskEXIT_CRITICAL();
  return 0;
}

module_system(hrtwork, work_start_hrtimer, NULL, 0)

#endif /* CONFIG_SCHED_HRTWORK */
