/**
* @file         
* @brief		        hudi list interface
* @par Copyright(c): 	Hichip Semiconductor (c) 2023
*/

#include <stdio.h>
#include <stdlib.h>
#include "hudi_list.h"

static hudi_list_iterator_t *_hudi_list_iterator_new_from_node(hudi_list_node_t *node, hudi_list_direction_t direction) 
{
    hudi_list_iterator_t *self;
    if (!(self = malloc(sizeof(hudi_list_iterator_t))))
        return NULL;
        
    self->next = node;
    self->direction = direction;
    
    return self;
}

static hudi_list_iterator_t *_hudi_list_iterator_new(hudi_list_t *list, hudi_list_direction_t direction) 
{
    hudi_list_node_t *node = direction == HUDI_LIST_HEAD ? list->head : list->tail;
    return _hudi_list_iterator_new_from_node(node, direction);
}

static hudi_list_node_t *_hudi_list_iterator_next(hudi_list_iterator_t *self) 
{
    hudi_list_node_t *curr = self->next;
    if (curr) 
    {
        self->next = self->direction == HUDI_LIST_HEAD ? curr->next : curr->prev;
    }
    
    return curr;
}

static void _hudi_list_iterator_destroy(hudi_list_iterator_t *self) 
{
    free(self);
    self = NULL;
}

hudi_list_t *hudi_list_new(void) 
{
    hudi_list_t *self;
    
    if (!(self = malloc(sizeof(hudi_list_t))))
        return NULL;
        
    self->head = NULL;
    self->tail = NULL;
    self->free = NULL;
    self->match = NULL;
    self->len = 0;
    
    return self;
}

hudi_list_node_t *hudi_list_node_new(void *val) 
{
    hudi_list_node_t *self;
    
    if (!(self = malloc(sizeof(hudi_list_node_t))))
        return NULL;
        
    self->prev = NULL;
    self->next = NULL;
    self->val = val;
    
    return self;
}

void hudi_list_destroy(hudi_list_t *self) 
{
    unsigned int len = self->len;
    hudi_list_node_t *next;
    hudi_list_node_t *curr = self->head;

    while (len--) 
    {
        next = curr->next;
        if (self->free) 
            self->free(curr->val);
            
        free(curr);
        curr = next;
    }

    free(self);
}

hudi_list_node_t *hudi_list_rpush(hudi_list_t *self, hudi_list_node_t *node) 
{
    if (!node) 
        return NULL;

    if (self->len) 
    {
        node->prev = self->tail;
        node->next = NULL;
        self->tail->next = node;
        self->tail = node;
    } 
    else 
    {
        self->head = self->tail = node;
        node->prev = node->next = NULL;
    }

    ++self->len;
    
    return node;
}

hudi_list_node_t *hudi_list_rpop(hudi_list_t *self) 
{
    if (!self->len) 
        return NULL;

    hudi_list_node_t *node = self->tail;

    if (--self->len) 
    {
        (self->tail = node->prev)->next = NULL;
    } 
    else 
    {
        self->tail = self->head = NULL;
    }

    node->next = node->prev = NULL;
    
    return node;
}

hudi_list_node_t *hudi_list_lpop(hudi_list_t *self) 
{
    if (!self->len) 
        return NULL;

    hudi_list_node_t *node = self->head;

    if (--self->len) 
    {
        (self->head = node->next)->prev = NULL;
    }
    else 
    {
        self->head = self->tail = NULL;
    }

    node->next = node->prev = NULL;
    
    return node;
}

hudi_list_node_t *hudi_list_lpush(hudi_list_t *self, hudi_list_node_t *node) 
{
    if (!node) 
    return NULL;

    if (self->len) 
    {
        node->next = self->head;
        node->prev = NULL;
        self->head->prev = node;
        self->head = node;
    } 
    else
    {
        self->head = self->tail = node;
        node->prev = node->next = NULL;
    }

    ++self->len;
    
    return node;
}

hudi_list_node_t *hudi_list_find(hudi_list_t *self, void *val) 
{
    hudi_list_iterator_t *it = _hudi_list_iterator_new(self, HUDI_LIST_HEAD);
    hudi_list_node_t *node;

    while ((node = _hudi_list_iterator_next(it))) 
    {
        if (self->match)
        {
            if (self->match(val, node->val)) 
            {
                _hudi_list_iterator_destroy(it);
                return node;
            }
        } 
        else 
        {
            if (val == node->val) 
            {
                _hudi_list_iterator_destroy(it);
                return node;
            }
        }
    }

    _hudi_list_iterator_destroy(it);
    
    return NULL;
}

hudi_list_node_t *hudi_list_at(hudi_list_t *self, int index) 
{
    hudi_list_direction_t direction = HUDI_LIST_HEAD;

    if (index < 0) 
    {
        direction = HUDI_LIST_TAIL;
        index = ~index;
    }

    if (index < self->len) 
    {
        hudi_list_iterator_t *it = _hudi_list_iterator_new(self, direction);
        hudi_list_node_t *node = _hudi_list_iterator_next(it);
        while (index--) 
            node = _hudi_list_iterator_next(it);
        _hudi_list_iterator_destroy(it);
        return node;
    }

    return NULL;
}

void hudi_list_remove(hudi_list_t *self, hudi_list_node_t *node) 
{
    node->prev ? (node->prev->next = node->next) : (self->head = node->next);
    node->next ? (node->next->prev = node->prev) : (self->tail = node->prev);

    if (self->free) 
        self->free(node->val);

    free(node);
    --self->len;
}
