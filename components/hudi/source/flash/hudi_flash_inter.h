/**
* @file         
* @brief		        hudi flash internal interface
* @par Copyright(c): 	Hichip Semiconductor (c) 2023
*/

#ifndef __HUDI_FLASH_INTER_H__
#define __HUDI_FLASH_INTER_H__

typedef struct
{
    int open_cnt;
    hudi_flash_type_e type;
} hudi_flash_instance_t;

#ifdef __cplusplus
extern "C" {
#endif

void hudi_flash_lock(void);
void hudi_flash_unlock(void);

#ifdef __cplusplus
}
#endif

#endif