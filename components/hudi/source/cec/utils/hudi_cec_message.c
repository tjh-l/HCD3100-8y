
#include <stdint.h>
#ifdef __linux__
#include <poll.h>
#include <signal.h>
#include <sys/msg.h>
#include <termios.h>
#include <unistd.h>
#include <string.h>

#else
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>
#include <freertos/task.h>
#include <kernel/lib/console.h>
#endif

#include <pthread.h>
#include <stdio.h>
#include "hudi_cec_message.h"

static pthread_mutex_t m_msg_mutex = PTHREAD_MUTEX_INITIALIZER;

#ifdef __linux__
#define MAX_MSG_SIZE 128
typedef struct
{
    long type;
    char buff[MAX_MSG_SIZE];
} msg_queue_t;

uint32_t hudi_cec_api_message_create(int msg_count, int msg_length)
{
    (void)msg_count;
    (void)msg_length;
    int msgid = -1;

    pthread_mutex_lock(&m_msg_mutex);

    msgid = msgget(IPC_PRIVATE, 0666 | IPC_CREAT);
    if (msgid <= 0)
    {
        // make sure the msgid > 0, otherwise msgsnd would error!!!
        perror("msgget failed");
        msgid = msgget(IPC_PRIVATE, 0666 | IPC_CREAT);
        if (msgid <= 0)
        {
            perror("msgget failed again");
            pthread_mutex_unlock(&m_msg_mutex);
            return INVALID_ID;
        }
    }
    pthread_mutex_unlock(&m_msg_mutex);

    printf("create msg id: %d\n", msgid);
    return (uint32_t)msgid;
}

int hudi_cec_api_message_delete(uint32_t *msg_id)
{
    int msgid = (int) * msg_id;
    if (msgctl(msgid, IPC_RMID, NULL) == -1)
    {
        printf("msg delete failed\n");
        return -1;
    }
    *msg_id = 0;
    return 0;
}

int hudi_cec_api_message_send(uint32_t msg_id, void *msg, int length)
{
    int msgid = (int)msg_id;

    if (length >= (MAX_MSG_SIZE - 1))
    {
        printf("Message size too large!\n");
        return -1;
    }

    msg_queue_t msg_queue;
    msg_queue.type = 1;
    memcpy(msg_queue.buff, msg, length);

    if (msgsnd(msgid, (void *)&msg_queue, length, 0) == -1)
    {
        perror("msgsnd failed");
        return -1;
    }
    return 0;
}

int hudi_cec_api_message_receive(uint32_t msg_id, void *msg, int length)
{
    int         msgid = (int)msg_id;
    msg_queue_t msg_queue;

    if (msgrcv(msgid, (void *)&msg_queue, length, 0, IPC_NOWAIT) == -1)
    {
        return -1;
    }
    memcpy(msg, msg_queue.buff, length);

    return 0;
}

int hudi_cec_api_message_receive_tm(uint32_t msg_id, void *msg, int length, int ms)
{
    int i;
    for (i = 0; i < ms; i++)
    {
        if (0 == hudi_cec_api_message_receive(msg_id, msg, length))
            return 0;
        usleep(1000);  // sleep 1ms
    }
    return -1;
}

int hudi_cec_api_message_get_count(uint32_t msg_id)
{
    int             msgid = (int)msg_id;
    struct msqid_ds info;

    if (msgctl(msgid, IPC_STAT, &info) == -1)
    {
        printf("%s() error!\n", __func__);
        return -1;
    }
    return info.msg_qnum;
}

int hudi_cec_api_message_recv(uint32_t msg_id, void *msg, int length, bool nowait)
{
    int         msgid    = (int)msg_id;
    int         msg_flag = IPC_NOWAIT;
    msg_queue_t msg_queue;

    if (!nowait)
    {
        msg_flag = 0;
    }

    if (msgrcv(msgid, (void *)&msg_queue, length, 0, msg_flag) == -1)
    {
        return -1;
    }
    memcpy(msg, msg_queue.buff, length);

    return 0;
}

#else

uint32_t hudi_cec_api_message_create(int msg_count, int msg_length)
{
    QueueHandle_t msgid = NULL;
    msgid               = xQueueCreate((UBaseType_t)msg_count, msg_length);
    if (!msgid)
    {
        printf("create msg queue failed\n");
        return 0;
    }
    return (uint32_t)msgid;
}

int hudi_cec_api_message_delete(uint32_t *msg_id)
{
    QueueHandle_t msgid = ((QueueHandle_t) * msg_id);
    vQueueDelete(msgid);
    *msg_id = 0;
    return 0;
}

int hudi_cec_api_message_send(uint32_t msg_id, void *msg, int length)
{
    (void)length;
    QueueHandle_t msgid = ((QueueHandle_t)msg_id);
    if (xQueueSend(msgid, msg, 0) != pdTRUE)
    {
        fprintf(stderr, "xQueueSend failed\n");
        return -1;
    }
    return 0;
}

int hudi_cec_api_message_receive(uint32_t msg_id, void *msg, int length)
{
    (void)length;
    QueueHandle_t msgid = ((QueueHandle_t)msg_id);
    if (xQueueReceive((QueueHandle_t)msgid, (void *)msg, 0) != pdPASS)
    {
        return -1;
    }
    return 0;
}

int hudi_cec_api_message_receive_tm(uint32_t msg_id, void *msg, int length, int ms)
{
    (void)length;
    QueueHandle_t msgid = ((QueueHandle_t)msg_id);
    if (xQueueReceive((QueueHandle_t)msgid, (void *)msg, ms) != pdPASS)
    {
        return -1;
    }
    return 0;
}

int hudi_cec_api_message_get_count(uint32_t msg_id)
{
    QueueHandle_t msgid = ((QueueHandle_t)msg_id);
    return uxQueueMessagesWaiting(msgid);
}

int hudi_cec_api_message_recv(uint32_t msg_id, void *msg, int length, bool nowait)
{
    (void)length;

    QueueHandle_t msgid = ((QueueHandle_t)msg_id);
    int           ms    = 0;
    if (!nowait)
    {
        ms = portMAX_DELAY;
    }

    if (xQueueReceive((QueueHandle_t)msgid, (void *)msg, ms) != pdPASS)
    {
        return -1;
    }
    return 0;
}

#endif
