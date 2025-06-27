#ifndef _SWISP_H
#define _SWISP_H

#include <stdint.h>
#include <stdbool.h>


/*
enum {
    SWISP_CTRL_GAMMA,
    SWISP_CTRL_CCM,
    SWISP_CTRL_AWB,
};

#define SWISP_CTRL_GAMMA_MASK (1 << SWISP_CTRL_GAMMA)
*/
typedef enum {
    SWISP_GC1054,
    SWISP_BF3A03,
} swisp_sensor_t;

typedef enum {
    SWISP_RGGB = 0,
    SWISP_GRBG,
    SWISP_GBRG,
    SWISP_BGGR
} swisp_raw_layout_t;

typedef struct {
    uint32_t pipeline_ctrl;
    swisp_raw_layout_t raw_layout;
    swisp_sensor_t sensor;

    /* the parameters of correct luma*/
    uint64_t y_sum;
    int y_mean;
    int y_count;

    /* the parameters of AWB*/
    uint64_t r_sum;
    uint64_t g_sum;
    uint64_t b_sum;
    int r_mean;
    int g_mean;
    int b_mean;
    int count;    // the pixel count of one pic

    bool is_first_frame; // judge current frame whether the first frame
} swisp_raw_ctx_t;

typedef struct{
    /* skin luma parameters */
    int skin_y_mean;
    int skin_y_sum;
    int skin_y_count;
    int cy_times;   // !> the times of modify luma

    /* img parameters */
    int w;
    int h;
    int stride;

    /* ctrl parameters*/
    bool is_first_frame;
} swisp_cy_ctx_t;

typedef struct{
    int w;      // !>img width
    int h;      // !>img height
    int dx_s;   // !>the start x of target area
    int dy_s;   // !>the start y of target area
    int dx_e;   // !>the end x of target area
    int dy_e;   // !>the end y of target area
    int stride;

    int target_y; 
    int constract;
    int mean_y; // !> area mean value of y
    int area_sum; 
    bool is_first_frame;
} swisp_ry_ctx_t;

// when input raw data------------------------------------
void swisp_raw_init_ctx(swisp_raw_ctx_t *ctx, uint32_t ctrl, 
                    swisp_sensor_t sensor, 
                    swisp_raw_layout_t raw_layout);

// when crop an input RAW image, (x, y), width, and height must be multiple of 2
void swisp_raw_to_yuv420_32x16(swisp_raw_ctx_t *ctx, 
                    const uint8_t *src_y, int src_y_stride, 
                    const uint8_t *src_c, int src_c_stride,
                    int w, int h, 
                    uint8_t *output_y, int dst_y_stride,
                    uint8_t *output_c, int dst_c_stride);


// when input yuv422 img-----------------------------------
void swisp_cy_init_ctx(swisp_cy_ctx_t *ctx, int w, int h);
void swisp_yuv_correct_y(swisp_cy_ctx_t *ctx, uint8_t *src_y, const uint8_t *src_c);


// for input single y data---------------------------------
/*
target_y: for the input y component, values range [1, 255]
constract : for the input constract component, values range [1, 100]
*/
void swisp_ry_init_ctx(swisp_ry_ctx_t *ctx, int w, int h,
                int dx_s, int dy_s, int dx_e,
                int dy_e, int target_y, int constract);

void swisp_red_y(swisp_ry_ctx_t *ctx, uint8_t *src_y);

void swisp_printf_ry_ctx(swisp_ry_ctx_t *ctx);


/*
swisp_ctx_t swisp_ctx;
swisp_init_ctx(&swisp_ctx, SWISP_CTRL_GAMMA_MASK | SWISP_CTRL_AWB_MASK, );

foreach (frame)
    swisp_pipeline(,...);;

swisp_destroy_ctx(&swisp_ctx);
*/

#endif
