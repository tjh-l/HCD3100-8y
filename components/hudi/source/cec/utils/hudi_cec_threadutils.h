/**
 * @file
 * @brief                hudi cec module interface.
 * @par Copyright(c):    Hichip Semiconductor (c) 2024
 */
#ifndef __HUDI_CEC_THREADUTILS_H__
#define __HUDI_CEC_THREADUTILS_H__

#include <pthread.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct
{
    void *thisobj;

    bool bstop;

    pthread_t      m_thread;
    pthread_attr_t m_attr;
    void *(*Process)(void *obj);
} hudi_cec_cthread_t;

typedef struct
{
    pthread_mutex_t mutex;
    pthread_cond_t  cond;
    uint32_t        msg_handle;
} hudi_cec_cthread_msg_t;

bool  hudi_cec_cthread_is_stopped(hudi_cec_cthread_t *thisptr);
bool  hudi_cec_cthread_is_running(hudi_cec_cthread_t *thisptr);
void *hudi_cec_cthread_handler(void *_thread);
bool  hudi_cec_cthread_create_thread(hudi_cec_cthread_t *thisptr,
                                     unsigned int stacksize, bool bwait);
bool hudi_cec_cthread_stop_thread(hudi_cec_cthread_t *thisptr, int iwaitms);
int hudi_cec_cthread_thread_init(hudi_cec_cthread_t *thisptr, void *thisobj,
                                 void *(*Process)(void *));

#ifdef __cplusplus
}
#endif

#endif