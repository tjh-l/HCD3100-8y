#ifndef _TWILL_H_
#define _TWILL_H_

#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>

typedef struct{
    int order; //detect bar order: 4, 5, 6, 3, 2, 1
    bool horlines;
    bool twills;
} swtwill_result_t;

int swtwill_detect(int w, int h, const uint8_t *src_y, swtwill_result_t *res);

#endif