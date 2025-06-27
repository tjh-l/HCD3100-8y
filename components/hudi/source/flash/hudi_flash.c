/**
* @file
* @brief                hudi flash operation interface
* @par Copyright(c):    Hichip Semiconductor (c) 2024
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include <hudi_com.h>
#include <hudi_log.h>
#include <hudi_flash.h>
#include "hudi_flash_inter.h"

static hudi_flash_instance_t *g_hudi_flash_instance = NULL;
static pthread_mutex_t g_hudi_flash_mutex = PTHREAD_MUTEX_INITIALIZER;

void hudi_flash_lock(void)
{
    pthread_mutex_lock(&g_hudi_flash_mutex);
}

void hudi_flash_unlock(void)
{
    pthread_mutex_unlock(&g_hudi_flash_mutex);
}

int hudi_flash_open(hudi_handle *handle, hudi_flash_type_e type)
{
    hudi_flash_lock();

    if (!g_hudi_flash_instance)
    {
        g_hudi_flash_instance = malloc(sizeof(hudi_flash_instance_t));
        memset(g_hudi_flash_instance, 0, sizeof(hudi_flash_instance_t));
    }

    g_hudi_flash_instance->open_cnt ++;
    g_hudi_flash_instance->type = type;

    *handle = (hudi_handle)g_hudi_flash_instance;

    hudi_flash_unlock();

    return 0;
}

int hudi_flash_close(hudi_handle handle)
{
    hudi_flash_lock();

    if (handle != g_hudi_flash_instance)
    {
        hudi_log(HUDI_LL_ERROR, "Handle mismatch\n");
        hudi_flash_unlock();
        return -1;
    }

    if (!g_hudi_flash_instance)
    {
        hudi_log(HUDI_LL_ERROR, "No valid handle\n");
        hudi_flash_unlock();
        return -1;
    }

    g_hudi_flash_instance->open_cnt --;

    if (g_hudi_flash_instance->open_cnt <= 0)
    {
        free(g_hudi_flash_instance);
        g_hudi_flash_instance = NULL;
    }

    hudi_flash_unlock();

    return 0;
}
