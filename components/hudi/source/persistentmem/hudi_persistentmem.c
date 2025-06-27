/**
 * @file
 * @brief                hudi persistentmem interface
 * @par Copyright(c):    Hichip Semiconductor (c) 2024
 */

#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include <hcuapi/persistentmem.h>

#include <hudi_com.h>
#include <hudi_log.h>
#include <hudi_persistentmem.h>

#include "hudi_persistentmem_inter.h"

static pthread_mutex_t g_hudi_persistentmem_mutex = PTHREAD_MUTEX_INITIALIZER;

static int _hudi_persistentmem_mutex_lock()
{
    pthread_mutex_lock(&g_hudi_persistentmem_mutex);

    return 0;
}

static int _hudi_persistentmem_mutex_unlock()
{
    pthread_mutex_unlock(&g_hudi_persistentmem_mutex);

    return 0;
}

int hudi_persistentmem_node_get(hudi_handle handle, struct persistentmem_node *node)
{
    hudi_persistentmem_instance_t *inst = (hudi_persistentmem_instance_t *)handle;

    if (!inst || !inst->inited)
    {
        hudi_log(HUDI_LL_ERROR, "Persistentmem not open\n");
        return -1;
    }

    _hudi_persistentmem_mutex_lock();

    if (ioctl(inst->fd, PERSISTENTMEM_IOCTL_NODE_GET, node) < 0)
    {
        hudi_log(HUDI_LL_ERROR, "Persistentmem node get fail\n");
        _hudi_persistentmem_mutex_unlock();
        return -1;
    }

    _hudi_persistentmem_mutex_unlock();

    return 0;
}

int hudi_persistentmem_node_put(hudi_handle handle, struct persistentmem_node *node)
{
    hudi_persistentmem_instance_t *inst = (hudi_persistentmem_instance_t *)handle;

    if (!inst || !inst->inited)
    {
        hudi_log(HUDI_LL_ERROR, "Persistentmem not open\n");
        return -1;
    }

    _hudi_persistentmem_mutex_lock();

    if (ioctl(inst->fd, PERSISTENTMEM_IOCTL_NODE_PUT, node) < 0)
    {
        hudi_log(HUDI_LL_ERROR, "Persistentmem node put fail\n");
        _hudi_persistentmem_mutex_unlock();
        return -1;
    }

    _hudi_persistentmem_mutex_unlock();

    return 0;
}

int hudi_persistentmem_node_create(hudi_handle handle, struct persistentmem_node_create *node)
{
    hudi_persistentmem_instance_t *inst = (hudi_persistentmem_instance_t *)handle;

    if (!inst || !inst->inited)
    {
        hudi_log(HUDI_LL_ERROR, "Persistentmem not open\n");
        return -1;
    }

    _hudi_persistentmem_mutex_lock();

    if (ioctl(inst->fd, PERSISTENTMEM_IOCTL_NODE_CREATE, node) < 0)
    {
        hudi_log(HUDI_LL_ERROR, "Persistentmem node create fail\n");
        _hudi_persistentmem_mutex_unlock();
        return -1;
    }

    _hudi_persistentmem_mutex_unlock();

    return 0;
}

int hudi_persistentmem_node_delete(hudi_handle handle, unsigned short node_id)
{
    hudi_persistentmem_instance_t *inst = (hudi_persistentmem_instance_t *)handle;

    if (!inst || !inst->inited)
    {
        hudi_log(HUDI_LL_ERROR, "Persistentmem not open\n");
        return -1;
    }

    _hudi_persistentmem_mutex_lock();

    if (ioctl(inst->fd, PERSISTENTMEM_IOCTL_NODE_DELETE, node_id) < 0)
    {
        hudi_log(HUDI_LL_ERROR, "Persistentmem node delete fail\n");
        _hudi_persistentmem_mutex_unlock();
        return -1;
    }

    _hudi_persistentmem_mutex_unlock();

    return 0;
}

int hudi_persistentmem_open(hudi_handle *handle)
{
    hudi_persistentmem_instance_t *inst = NULL;

    if (!handle)
    {
        hudi_log(HUDI_LL_ERROR, "Invalid persistentmem parameters\n");
        return -1;
    }

    _hudi_persistentmem_mutex_lock();

    inst = (hudi_persistentmem_instance_t *)malloc(sizeof(hudi_persistentmem_instance_t));
    memset(inst, 0, sizeof(hudi_persistentmem_instance_t));

    inst->fd = open(HUDI_PERSISTENTMEM_DEV, O_SYNC | O_RDWR);
    if (inst->fd < 0)
    {
        hudi_log(HUDI_LL_ERROR, "Open persistentmem fail\n");
        free(inst);
        _hudi_persistentmem_mutex_unlock();
        return -1;
    }

    inst->inited = 1;
    *handle = inst;

    _hudi_persistentmem_mutex_unlock();

    return 0;
}

int hudi_persistentmem_close(hudi_handle handle)
{
    hudi_persistentmem_instance_t *inst = (hudi_persistentmem_instance_t *)handle;

    if (!inst || !inst->inited)
    {
        hudi_log(HUDI_LL_ERROR, "Persistentmem not open\n");
        return -1;
    }

    _hudi_persistentmem_mutex_lock();

    close(inst->fd);

    memset(inst, 0, sizeof(hudi_persistentmem_instance_t));
    free(inst);

    _hudi_persistentmem_mutex_unlock();

    return 0;
}