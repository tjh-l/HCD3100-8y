/**
* @file
* @brief                hudi persistentmem interface.
* @par Copyright(c):    Hichip Semiconductor (c) 2024
*/

#ifndef __HUDI_PERSISTENTMEM_H__
#define __HUDI_PERSISTENTMEM_H__

#include <hcuapi/persistentmem.h>
#include "hudi_com.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
* @brief       Open an hudi persistentmem module instance
* @param[out]  handle  Output the handle of the instance
* @retval      0       Success
* @retval      other   Error
*/
int hudi_persistentmem_open(hudi_handle *handle);

/**
* @brief       Close an hudi persistentmem module instance
* @param[in]   handle  Handle of the instance to be closed
* @retval      0       Success
* @retval      other   Error
*/
int hudi_persistentmem_close(hudi_handle handle);

/**
* @brief       Create a persistentmem node
* @param[in]   handle  Handle of the instance
* @param[in]   node    Persistentmem create node parameters
* @retval      0       Success
* @retval      other   Error
*/
int hudi_persistentmem_node_create(hudi_handle handle, struct persistentmem_node_create *node);

/**
* @brief       Delete the persistentmem node
* @param[in]   handle   Handle of the instance
* @param[in]   node_id  The value of the persistentmem node id
* @retval      0        Success
* @retval      other    Error
*/
int hudi_persistentmem_node_delete(hudi_handle handle, unsigned short node_id);

/**
* @brief       Get the persistentmem node
* @param[in]   handle  Handle of the instance
* @param[in]   node    Persistentmem node parameters
* @retval      0       Success
* @retval      other   Error
*/
int hudi_persistentmem_node_get(hudi_handle handle, struct persistentmem_node *node);

/**
* @brief       Put the persistentmem node
* @param[in]   handle  Handle of the instance
* @param[in]   node    Persistentmem node parameters
* @retval      0       Success
* @retval      other   Error
*/
int hudi_persistentmem_node_put(hudi_handle handle, struct persistentmem_node *node);

#ifdef __cplusplus
}
#endif

#endif
