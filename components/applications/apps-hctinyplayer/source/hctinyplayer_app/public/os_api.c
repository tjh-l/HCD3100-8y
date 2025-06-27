#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include <freertos/queue.h>
#include <kernel/lib/console.h>
#include <pthread.h>
#include "com_api.h"
#include "os_api.h"

static pthread_mutex_t m_msg_mutex = PTHREAD_MUTEX_INITIALIZER;

uint32_t api_message_create(int msg_count, int msg_length)
{
    QueueHandle_t msgid = NULL;
    msgid = xQueueCreate(( UBaseType_t )msg_count, msg_length);
    if (!msgid)
    {
        printf ("create msg queue failed\n");
        return 0;
    }
    return (uint32_t)msgid;
}

int api_message_delete(uint32_t msg_id)
{
    QueueHandle_t msgid = ((QueueHandle_t)msg_id);
    vQueueDelete(msgid);
    return 0;
}

int api_message_send(uint32_t msg_id, void *msg, int length)
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

int api_message_receive(uint32_t msg_id, void *msg, int length)
{
    (void)length;
    QueueHandle_t msgid = ((QueueHandle_t)msg_id);
    if (xQueueReceive((QueueHandle_t)msgid, (void *)msg, 0) != pdPASS)
    {
        //fprintf(stderr, "xQueueReceive failed width erro: %d", errno);
        return -1;
    }
    return 0;
}

int api_message_receive_tm(uint32_t msg_id, void *msg, int length, int ms)
{
    (void)length;
    QueueHandle_t msgid = ((QueueHandle_t)msg_id);
    if (xQueueReceive((QueueHandle_t)msgid, (void *)msg, ms) != pdPASS)
    {
        //fprintf(stderr, "xQueueReceive failed width erro: %d", errno);
        return -1;
    }
    return 0;
}

int api_message_get_count(uint32_t msg_id)
{
    QueueHandle_t msgid = ((QueueHandle_t)msg_id);
    return uxQueueMessagesWaiting(msgid);
}

