#include "hudi_cec_threadutils.h"
int hudi_cec_cthread_thread_init(hudi_cec_cthread_t *thisptr, void *thisobj,
                                 void *(*Process)(void *))
{
    thisptr->thisobj = thisobj;
    thisptr->Process = Process;
    return 0;
}
bool hudi_cec_cthread_is_stopped(hudi_cec_cthread_t *thisptr)
{
    return thisptr->bstop;
}

void *hudi_cec_cthread_handler(void *_thread)
{
    hudi_cec_cthread_t *thread = _thread;
    void *              retVal = NULL;

    if (thread->Process)
        retVal = thread->Process(thread->thisobj);

    return retVal;
}
bool hudi_cec_cthread_create_thread(hudi_cec_cthread_t *thisptr,
                                    unsigned int stacksize, bool bwait)
{
    (void)bwait;
    bool ret = false;

    pthread_attr_init(&thisptr->m_attr);
    pthread_attr_setstacksize(&thisptr->m_attr, stacksize);
    thisptr->bstop = false;

    if (pthread_create(&thisptr->m_thread, &thisptr->m_attr,
                       hudi_cec_cthread_handler, ((void *)thisptr)) == 0)
    {
        ret = true;
    }

    return ret;
}

bool hudi_cec_cthread_stop_thread(hudi_cec_cthread_t *thisptr, int iwaitms)
{
    (void)iwaitms;
    bool ret = true;

    thisptr->bstop = true;
    pthread_join(thisptr->m_thread, NULL);

    return ret;
}