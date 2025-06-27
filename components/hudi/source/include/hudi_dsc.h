/**
* @file
* @brief                hudi dsc interface.
* @par Copyright(c):    Hichip Semiconductor (c) 2024
*/

#ifndef __HUDI_DSC_H__
#define __HUDI_DSC_H__

#include <hcuapi/dsc.h>

#include "hudi_com.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    dsc_algo_type_e algo_type;
    dsc_crypto_mode_e crypto_mode;
    dsc_chaining_mode_e chaining_mode;
    dsc_residue_mode_e residue_mode;
    unsigned int dsc_buf_len;
} hudi_dsc_config_t;

/**
* @brief       Open a hudi dsc module instance
* @param[out]  handle  Output the handle of the instance
* @param[in]   config  Dsc parameters
* @retval      0       Success
* @retval      other   Error
*/
int hudi_dsc_open(hudi_handle *handle, hudi_dsc_config_t *config);

/**
* @brief       Close a hudi dsc module instance
* @param[in]   handle  Handle of the instance to be closed
* @retval      0       Success
* @retval      other   Error
*/
int hudi_dsc_close(hudi_handle handle);

/**
* @brief       Set encryption/decryption key
* @param[in]   handle  Handle of the instance
* @param[in]   key     Encryption/Decryption key
* @param[in]   key_len Length of key data
* @param[in]   iv      Initialization vector
* @param[in]   iv_len  Length of iv data
* @retval      0       Success
* @retval      other   Error
*/
int hudi_dsc_key_set(hudi_handle handle, unsigned char *key, unsigned int key_len, unsigned char* iv, unsigned iv_len);

/**
* @brief       Encrypt/Decrypt data
* @param[in]   handle  Handle of the instance
* @param[in]   input   Buffer for holding the input data
* @param[in]   output  Buffer for holding the output data
* @param[in]   len     Length of the input data
* @retval      0       Success
* @retval      other   Error
*/
int hudi_dsc_crypt(hudi_handle *handle, unsigned char *input, unsigned char *output, unsigned int len);

#ifdef __cplusplus
}
#endif

#endif
