/**
* @file         
* @brief		        hudi list interface
* @par Copyright(c): 	Hichip Semiconductor (c) 2023
*/

#ifndef __HUDI_LIST_H__
#define __HUDI_LIST_H__

#ifdef __cplusplus
extern "C" {
#endif

typedef enum 
{
    HUDI_LIST_HEAD, 
    HUDI_LIST_TAIL,
} hudi_list_direction_t;

typedef struct hudi_list_node 
{
    struct hudi_list_node *prev;
    struct hudi_list_node *next;
    void *val;
} hudi_list_node_t;

typedef struct 
{
    hudi_list_node_t *head;
    hudi_list_node_t *tail;
    int len;
    void (*free)(void *val);
    int (*match)(void *a, void *b);
} hudi_list_t;

typedef struct 
{
    hudi_list_node_t *next;
    hudi_list_direction_t direction;
} hudi_list_iterator_t;

hudi_list_t *hudi_list_new(void);
hudi_list_node_t *hudi_list_node_new(void *val);
hudi_list_node_t *hudi_list_rpush(hudi_list_t *self, hudi_list_node_t *node);
hudi_list_node_t *hudi_list_lpush(hudi_list_t *self, hudi_list_node_t *node);
hudi_list_node_t *hudi_list_find(hudi_list_t *self, void *val);
hudi_list_node_t *hudi_list_at(hudi_list_t *self, int index);
hudi_list_node_t *hudi_list_rpop(hudi_list_t *self);
hudi_list_node_t *hudi_list_lpop(hudi_list_t *self);
void hudi_list_remove(hudi_list_t *self, hudi_list_node_t *node);
void hudi_list_destroy(hudi_list_t *self);

#ifdef __cplusplus
}
#endif

#endif
