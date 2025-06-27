/**
 * @file
 * @brief                hudi cec module interface.
 * @par Copyright(c):    Hichip Semiconductor (c) 2024
 */
#ifndef __HUDI_CEC_MESSAGE_H__
#define __HUDI_CEC_MESSAGE_H__
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C"
{
#endif

#define INVALID_ID 0xFFFFFFFF

uint32_t hudi_cec_api_message_create(int msg_count, int msg_length);

int hudi_cec_api_message_delete(uint32_t *msg_id);

int hudi_cec_api_message_send(uint32_t msg_id, void *msg, int length);

int hudi_cec_api_message_receive(uint32_t msg_id, void *msg, int length);

int hudi_cec_api_message_receive_tm(uint32_t msg_id, void *msg, int length,
                                    int ms);

int hudi_cec_api_message_recv(uint32_t msg_id, void *msg, int length,
                              bool nowait);

int hudi_cec_api_message_get_count(uint32_t msg_id);

#ifdef __cplusplus
}
#endif

#endif