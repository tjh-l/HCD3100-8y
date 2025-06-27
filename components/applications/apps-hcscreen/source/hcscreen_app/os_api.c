/**
 * @file os_api.c
 * @author your name (you@domain.com)
 * @brief the common apis used for OS
 * @version 0.1
 * @date 2022-01-21
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#ifdef __linux__
#include <sys/msg.h>
#include <termios.h>
#include <poll.h>
#include <signal.h>

#else
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include <freertos/queue.h>
#include <kernel/lib/console.h>
#ifdef __linux__
#include <showlogo.h>
#endif
#endif

#include "com_api.h"
#include "os_api.h"

#ifdef __linux__
#define MAX_MSG_SIZE    128
typedef struct{
    long type;
    //char *buff;
    char buff[MAX_MSG_SIZE];
}msg_queue_t;    


uint32_t api_message_create(int msg_count, int msg_length)
{
    (void)msg_count;
    (void)msg_length;
    int msgid = -1;
    int count = 0;

    count = 0;
    do{
        msgid = msgget(IPC_PRIVATE, 0666 | IPC_CREAT);       
        if (msgid > 0)
            break;
        else if (msgid == 0)
            msgctl(msgid, IPC_RMID, NULL);
    }while(count++ < 10);

    if (msgid > 0){
        return (uint32_t)msgid;    
    } else {
        printf("%s(), line:%d. msgget failed\n", __func__, __LINE__);
        perror("msgget failed");
        return INVALID_ID;
    }
}

int api_message_create_by_key(uint32_t key)
{
    int msgid = -1;
    int count = 0;

    count = 0;
    do{
        msgid = msgget((key_t)key, 0666 | IPC_CREAT);       
        if (msgid > 0)
            break;
        else if (msgid == 0)
            msgctl(msgid, IPC_RMID, NULL);
    }while(count++ < 10);

    if (msgid > 0)
    {
        printf("%s(), line:%d. create msg id(%d): %d\n", __func__, __LINE__, count, msgid);
        return msgid;
    }
    else
    {
        printf("%s(), line:%d. msgget failed(%d)\n", __func__, __LINE__, count);
        perror("msgget failed");
        return -1;
    }    
}

int api_message_delete(uint32_t msg_id)
{
    int msgid = (int)msg_id;
    if (msgctl(msgid,IPC_RMID,NULL) == -1){
        //fprintf(stderr, "msg delete failed\n");
        printf("msg delete failed\n");
        return -1;
    }
    return 0;
}

int api_message_send(uint32_t msg_id, void *msg, int length)
{
    int msgid = (int)msg_id;

    if (length >= (MAX_MSG_SIZE-1)){
        printf("Message size too large!\n");
        return -1;
    }

    msg_queue_t msg_queue;
    msg_queue.type = 1;
    memcpy(msg_queue.buff, msg, length);


    if (msgsnd(msgid, (void *)&msg_queue, length, 0) == -1){
        //fprintf(stderr, "msgsnd failed\n");
        printf("msgsnd failed\n");
        return -1;
    }
    //printf("send msg id: %d, len:%d\n", msgid,length);
    return 0;
}

int api_message_receive(uint32_t msg_id, void *msg, int length)
{
    int msgid = (int)msg_id;
    msg_queue_t msg_queue;

    if (msgrcv(msgid, (void *)&msg_queue, length, 0, IPC_NOWAIT) == -1){
        //fprintf(stderr, "msgrcv failed width erro: %d", errno);
        return -1;
    }    
    memcpy(msg, msg_queue.buff, length);

    //printf("receive msg id: %d,len:%d\n", msgid,length);
    return 0;
}

int api_message_receive_tm(uint32_t msg_id, void *msg, int length, int ms)
{
    int i;
    for (i = 0; i < ms; i++)
    {
        if (0 == api_message_receive(msg_id, msg, length))
            return 0;
        usleep(1000);//sleep 1ms
    }
    return -1;
}

#else
uint32_t api_message_create(int msg_count, int msg_length)
{
    QueueHandle_t msgid = NULL;
    msgid = xQueueCreate(( UBaseType_t )msg_count, msg_length);
    if (!msgid) {
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
    if (xQueueSend(msgid, msg, 0) != pdTRUE){
        fprintf(stderr, "xQueueSend failed\n");
        return -1;
    }
    return 0;
}

int api_message_receive(uint32_t msg_id, void *msg, int length)
{
    (void)length;
    QueueHandle_t msgid = ((QueueHandle_t)msg_id);
    if (xQueueReceive((QueueHandle_t)msgid, (void *)msg, 0) != pdPASS){
        //fprintf(stderr, "xQueueReceive failed width erro: %d", errno);
        return -1;
    }
    return 0;
}

int api_message_receive_tm(uint32_t msg_id, void *msg, int length, int ms)
{
    (void)length;
    QueueHandle_t msgid = ((QueueHandle_t)msg_id);
    if (xQueueReceive((QueueHandle_t)msgid, (void *)msg, ms) != pdPASS){
        //fprintf(stderr, "xQueueReceive failed width erro: %d", errno);
        return -1;
    }
    return 0;
}

#endif
