#ifndef __OS_API_H__
#define __OS_API_H__

#ifdef __cplusplus
extern "C" {
#endif

#define INVALID_ID	0xFFFFFFFF

uint32_t api_message_create(int msg_count, int msg_length);
int api_message_create_by_key(uint32_t key);
int api_message_delete(uint32_t msg_id);
int api_message_send(uint32_t msg_id, void *msg, int length);
int api_message_receive(uint32_t msg_id, void *msg, int length);

int api_message_receive_tm(uint32_t msg_id, void *msg, int length, int ms);
int api_message_get_count(uint32_t msg_id);

#ifdef __cplusplus
}
#endif


#endif