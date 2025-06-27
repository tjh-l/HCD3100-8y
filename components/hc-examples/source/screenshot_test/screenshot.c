#ifdef __linux__
#include <signal.h>
#include <termios.h>
#include "console.h"
#else
#include <kernel/lib/console.h>
#endif

#include <stdio.h>
#include <stdint.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <hcuapi/dis.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <assert.h>

#define TILE_ROW_SWIZZLE_MASK 3
#define OSD_DEV      "/dev/fb0"
#define DIS_DEV      "/dev/dis"
static unsigned char *osd_buf = NULL;
static unsigned char *bmp_buf = NULL;
static unsigned char *dis_buf = NULL;
static unsigned char *dis_buf2= NULL;
static unsigned char *osd_dis_buf=NULL;
static uint32_t osd_width=1280;   //osd层的宽为 1280
static uint32_t osd_height=720;   //osd层的高为  720
static uint32_t dis_width;
static uint32_t dis_height;

typedef struct {    
    char signature[2];
    uint32_t filesize;
    uint32_t reserved;
    uint32_t data_offset;
} __attribute__((packed)) bmp_header_t;

typedef struct {    
    uint32_t info_size;
    int32_t width;
    int32_t height;
    uint16_t planes;
    uint16_t bpp;
    uint32_t compression;
    uint32_t image_size;
    int32_t x_ppm;
    int32_t y_ppm;
    uint32_t colors;
    uint32_t important_colors;
} __attribute__((packed)) bmp_info_header_t;

struct dd_s
{
  int      infd;     
  int      outfd;     
  uint32_t nsectors;   
  uint32_t sector;    
  uint32_t skip;      
  bool     eof;       
  ssize_t  sectsize;  
  ssize_t  nbytes;   
  uint8_t *buffer;    
};

static int c_bt709_yuvL2rgbF[3][3] =
{ {298,0 ,  459},
  {298,-55 ,-136},
  {298,541,0} };

typedef struct {
    unsigned char a;
    unsigned char r;
    unsigned char g;
    unsigned char b;
} ARGBPixel;

static ARGBPixel bilinear_interpolation(ARGBPixel tl, ARGBPixel tr, ARGBPixel bl, ARGBPixel br, float x_ratio, float y_ratio) {
    ARGBPixel result;
    result.r = (unsigned char)((tl.r * (1 - x_ratio) * (1 - y_ratio)) + (tr.r * x_ratio * (1 - y_ratio)) + (bl.r * (1 - x_ratio) * y_ratio) + (br.r * x_ratio * y_ratio));
    result.g = (unsigned char)((tl.g * (1 - x_ratio) * (1 - y_ratio)) + (tr.g * x_ratio * (1 - y_ratio)) + (bl.g * (1 - x_ratio) * y_ratio) + (br.g * x_ratio * y_ratio));
    result.b = (unsigned char)((tl.b * (1 - x_ratio) * (1 - y_ratio)) + (tr.b * x_ratio * (1 - y_ratio)) + (bl.b * (1 - x_ratio) * y_ratio) + (br.b * x_ratio * y_ratio));
    result.a = (unsigned char)((tl.a * (1 - x_ratio) * (1 - y_ratio)) + (tr.a * x_ratio * (1 - y_ratio)) + (bl.a * (1 - x_ratio) * y_ratio) + (br.a * x_ratio * y_ratio));
    return result;
}

static int dis_get_display_info(struct dis_display_info *display_info )
{
    int fd = -1;

    fd = open("/dev/dis" , O_WRONLY);
    if(fd < 0)
    {
        return -1;
    }

    display_info->distype = DIS_TYPE_HD;
    ioctl(fd , DIS_GET_DISPLAY_INFO , display_info);
    close(fd);
    printf("w:%lu h:%lu\n", (long unsigned int)display_info->info.pic_width, (long unsigned int)display_info->info.pic_height);
    printf("y_buf:0x%lx size = 0x%lx\n" , (long unsigned int)display_info->info.y_buf , (long unsigned int)display_info->info.y_buf_size);
    printf("c_buf:0x%lx size = 0x%lx\n" , (long unsigned int)display_info->info.c_buf , (long unsigned int)display_info->info.c_buf_size);
    printf("display_info->info.de_map_mode :%d\n",display_info->info.de_map_mode);
    return 0;
}

static inline int swizzle_tile_row(int dy)
{
    return (dy & ~TILE_ROW_SWIZZLE_MASK) | ((dy & 1) << 1) | ((dy & 2) >> 1);
}

static inline int calc_offset_mapping1(int stride_in_tile , int x , int y)
{
    int tile_x = x >> 4;
    const int tile_y = y >> 5;
    const int dx = x & 15;
    const int dy = y & 31;
    const int sdy = swizzle_tile_row(dy);
    stride_in_tile >>= 4;
    if(tile_x == stride_in_tile)
        tile_x--;
    return (stride_in_tile * tile_y + tile_x) * 512 + (sdy << 4) + dx;
}

static inline int calc_offset_mapping0(int stride_in_tile , int x , int y)
{
    return (y >> 4) * (stride_in_tile << 4) + (x >> 5) * 512 + (y & 15) * 32 + (x & 31);
}

static void get_yuv(unsigned char *p_buf_y ,
                    unsigned char *p_buf_c ,
                    int ori_width ,
                    int ori_height ,
                    int src_x ,
                    int src_y ,
                    unsigned char *y ,
                    unsigned char *u ,
                    unsigned char *v ,
                    int tile_mode)
{
    int width = 0;
    (void)ori_height;
    unsigned int src_offset = 0;

    if(tile_mode == 1)
    {
        width = (ori_width + 15) & 0xFFFFFFF0;

        src_offset = calc_offset_mapping1(width , src_x , src_y);
        *y = *(p_buf_y + src_offset);

        src_offset = calc_offset_mapping1(width , (src_x & 0xFFFFFFFE) , src_y >> 1);
        *u = *(p_buf_c + src_offset);
        *v = *(p_buf_c + src_offset + 1);
    }
    else
    {
        width = (ori_width + 31) & 0xFFFFFFE0;
        src_offset = calc_offset_mapping0(width , src_x , src_y);
        *y = *(p_buf_y + src_offset);
        src_offset = calc_offset_mapping0(width , (src_x & 0xFFFFFFFE) , src_y >> 1);
        *u = *(p_buf_c + src_offset);
        *v = *(p_buf_c + src_offset + 1);
    }
}

static int Clamp8(int v)
{
    if(v < 0)
    {
        return 0;
    }
    else if(v > 255)
    {
        return 255;
    }
    else
    {
        return v;
    }
}

static int Trunc8(int v)
{
    return v >> 8;
}

static void pixel_ycbcrL_to_argbF(unsigned char y , unsigned char cb , unsigned char cr ,
                                  unsigned char *r , unsigned char *g , unsigned char *b ,
                                  int c[3][3])  
{
    int  red = 0 , green = 0 , blue = 0;

    const int y1 = (y - 16) * c[0][0];
    const int pr = cr - 128;
    const int pb = cb - 128;
    red = Clamp8(Trunc8(y1 + (pr * c[0][2])));
    green = Clamp8(Trunc8(y1 + (pb * c[1][1] + pr * c[1][2])));
    blue = Clamp8(Trunc8(y1 + (pb * c[2][1])));

    *r = red;
    *g = green;
    *b = blue;
}

static void YUV420_RGB(unsigned char *p_buf_y ,
                unsigned  char *p_buf_c ,
                unsigned char *p_buf_out ,
                int ori_width ,
                int ori_height,
                int tile_mode)
{
    unsigned char *p_out = NULL;
    unsigned char r , g , b;
    unsigned char cur_y , cur_u , cur_v;

    int x = 0;
    int y = 0;

    p_out = p_buf_out;

    for(y = 0;y < ori_height;y++)
    {
        for(x = 0;x < ori_width;x++)
        {
            get_yuv(p_buf_y , p_buf_c ,
                    ori_width , ori_height ,
                    x , y ,
                    &cur_y , &cur_u , &cur_v ,
                    tile_mode);
            
            pixel_ycbcrL_to_argbF(cur_y , cur_u , cur_v , &r , &g , &b , c_bt709_yuvL2rgbF);
            *p_out++ = b;
            *p_out++ = g;
            *p_out++ = r;
            *p_out++ = 0xFF;
        }
    }
}

/***************************************************
 * 函数原型：static int get_osd_layer_data(void)  
 * 函数功能：获取osd层数据
 * 形参：argc ：
 *      argv ：
 * 返回值:int
 * 作者： lxw--2024/7/27
 * *************************************************/
static int get_osd_layer_data(void)  
{    
    if(!osd_buf){
        printf("malloc osd_buf\n");
        osd_buf=malloc(osd_width * osd_height *4);
        memset(osd_buf, 0, osd_width * osd_height *4); 
    }

    int infd = open(OSD_DEV, O_RDONLY);
    if(infd <0){
        printf("open fail \n");
        return -1;
    }

    uint32_t sizenum = read(infd, osd_buf, osd_width * osd_height *4);
    if (sizenum != osd_width * osd_height *4) {
        printf("read sizenum =%ld\n",sizenum);
    }

    close(infd);
}

/***************************************************
 * 函数原型：static int get_dis_layer_data(void) 
 * 函数功能：获取视频层数据
 * 形参：argc ：
 *      argv ：
 * 返回值:int
 * 作者： lxw--2024/7/27
 * *************************************************/
static int get_dis_layer_data(void) 
{
  
    int buf_size = 0;
    void *buf = NULL;
    int ret = 0;
    unsigned char *p_buf_y = NULL;
    unsigned char *p_buf_c = NULL;
    unsigned char *buf_rgb = NULL;
    int fd = -1;

    struct dis_display_info display_info = { 0 };
    
    fd = open("/dev/dis" , O_RDWR);
    if(fd < 0)
    {
        return -1;
    }

    ret = dis_get_display_info(&display_info);
    if(ret <0)
    {
        return -1;
    }

    dis_width=display_info.info.pic_width;
    dis_height=display_info.info.pic_height;

    if(!dis_width || !dis_height){
        return -1;
    }

#ifdef __linux__
    uint32_t y_buf_offset = (unsigned long)display_info.info.y_buf % 4096;
    buf_size = display_info.info.y_buf_size *4;
    buf = mmap(0 , buf_size , PROT_READ | PROT_WRITE , MAP_SHARED , fd , 0);
    buf_rgb = malloc(buf_size);
    memset(buf_rgb, 0, buf_size); 

    p_buf_y = buf + y_buf_offset;
    p_buf_c = p_buf_y + (display_info.info.c_buf - display_info.info.y_buf);
    printf("buf =0x%x buf_rgb = 0x%x rgb_buf_size = 0x%x\n", (int)buf, (int)buf_rgb, buf_size);

    unsigned char *copy_p_buf_y = malloc(display_info.info.y_buf_size);
    memset(copy_p_buf_y,0,sizeof(display_info.info.y_buf_size));
    memcpy(copy_p_buf_y, (unsigned char *)p_buf_y, display_info.info.y_buf_size);

    unsigned char *copy_p_buf_c = malloc(display_info.info.c_buf_size);
    memset(copy_p_buf_c,0,sizeof(display_info.info.c_buf_size));
    memcpy(copy_p_buf_c, (unsigned char *)p_buf_c, display_info.info.c_buf_size);

#else
    buf_size = dis_width * dis_height *4;
    buf_rgb = malloc(buf_size);
    memset(buf_rgb, 0, buf_size); 
    printf("buf =0x%x buf_rgb = 0x%x rgb_buf_size = 0x%x\n", (int)buf, (int)buf_rgb, buf_size);

    unsigned char *copy_p_buf_y = malloc(display_info.info.y_buf_size);
    memset(copy_p_buf_y,0,sizeof(display_info.info.y_buf_size));
    memcpy(copy_p_buf_y, (unsigned char *)(display_info.info.y_buf), display_info.info.y_buf_size);

    unsigned char *copy_p_buf_c = malloc(display_info.info.c_buf_size);
    memset(copy_p_buf_c,0,sizeof(display_info.info.c_buf_size));
    memcpy(copy_p_buf_c, (unsigned char *)(display_info.info.c_buf), display_info.info.c_buf_size);
#endif
    YUV420_RGB(copy_p_buf_y,
        copy_p_buf_c,
        buf_rgb ,
        display_info.info.pic_width ,
        display_info.info.pic_height,
        display_info.info.de_map_mode);

    if(copy_p_buf_y){
        free(copy_p_buf_y);
        copy_p_buf_y=NULL;
    }

    if(copy_p_buf_c){
        free(copy_p_buf_c);
        copy_p_buf_c=NULL;
    }

#ifdef __linux__
    if (munmap(buf, buf_size) == -1) {
        printf("munmap fail\n");
    }
#endif

    if(!dis_buf){
        dis_buf=malloc(buf_size);
        memset(dis_buf, 0, buf_size);
        printf("malloc dis_buf \n");
    }

    if(buf_rgb != NULL){
        memcpy(dis_buf, buf_rgb, buf_size);
    }   
    close(fd);

    if(buf_rgb){
        free(buf_rgb);
        buf_rgb=NULL;
    }
    return 0;
}

/***************************************************
 * 函数原型：static int resize_with_bilinear_interpolation(...)
 * 函数功能：将任意分辨率的图像缩放到另一个分辨率
 * 形参：argc ：
 *      argv ：
 * 返回值:int
 * 作者： lxw--2024/7/27
 * 示例：resize_with_bilinear_interpolation(dis_buf,dis_buf2,dis_width,dis_height,osd_width,osd_height);
 * *************************************************/
static int resize_with_bilinear_interpolation(unsigned char *input_data, unsigned char *output_data,
                                       int input_width, int input_height,
                                       int output_width, int output_height) {
    float x_ratio = (float)input_width / output_width;
    float y_ratio = (float)input_height / output_height;
    for (int y = 0; y < output_height; ++y) {
        for (int x = 0; x < output_width; ++x) {
            float x_src = x * x_ratio;
            float y_src = y * y_ratio;
            int x_int = (int)x_src;
            int y_int = (int)y_src;
            float x_frac = x_src - x_int;
            float y_frac = y_src - y_int;
            
            // 修正边界情况，防止越界
            if (x_int >= input_width - 1) x_int = input_width - 2;
            if (y_int >= input_height - 1) y_int = input_height - 2;

            int idx_tl = (y_int * input_width + x_int) * sizeof(ARGBPixel);
            int idx_tr = (y_int * input_width + x_int + 1) * sizeof(ARGBPixel);
            int idx_bl = ((y_int + 1) * input_width + x_int) * sizeof(ARGBPixel);
            int idx_br = ((y_int + 1) * input_width + x_int + 1) * sizeof(ARGBPixel);
            
            ARGBPixel tl = *(ARGBPixel *)(input_data + idx_tl);
            ARGBPixel tr = *(ARGBPixel *)(input_data + idx_tr);
            ARGBPixel bl = *(ARGBPixel *)(input_data + idx_bl);
            ARGBPixel br = *(ARGBPixel *)(input_data + idx_br);
            ARGBPixel result = bilinear_interpolation(tl, tr, bl, br, x_frac, y_frac);
            int idx_out = (y * output_width + x) * sizeof(ARGBPixel);
            memcpy(output_data + idx_out, &result, sizeof(ARGBPixel));
        }
    }    
    return 0;
}

/***************************************************
 * 函数原型：static int merge_raw_files(int width,int height )
 * 函数功能：将2个相同大小的分辨率的图片，合并成一个
 * 形参：argc ：
 *      argv ：
 * 返回值:int
 * 作者： lxw--2024/7/27
 * *************************************************/
static int merge_raw_files(int width,int height ) 
{
    printf("sizeof(ARGBPixel)*width * height =%ld\n",sizeof(ARGBPixel)*width * height);
    
    ARGBPixel *osd_data =(ARGBPixel *)osd_buf;
    ARGBPixel *dis_data=NULL;
    if(dis_buf){
        dis_data=(ARGBPixel *)dis_buf;
    }else if(dis_buf2){
        dis_data =(ARGBPixel *)dis_buf2;
    }else{
        printf(" dis buf and dis buf2 do not exist \n");
        return -1;
    }
    
    for (int i = 0; i < width * height; ++i) {
        if (osd_data[i].a == 0) {
            osd_data[i] = dis_data[i];
        }
    }

    if(dis_buf){
        free(dis_buf);
        printf("free dis_buf \n");
        dis_buf=NULL;
        dis_data=NULL;
    }else if(dis_buf2){
        free(dis_buf2);
        printf("free dis_buf2 \n");
        dis_buf2=NULL;
        dis_data=NULL;
    }

    if(!osd_dis_buf){
        osd_dis_buf=malloc(osd_width * osd_height *4);
        memset(osd_dis_buf, 0, osd_width * osd_height *4);
        printf("malloc osd_dis_buf \n");
    }
    if(osd_data !=NULL){
        memcpy(osd_dis_buf, osd_data, osd_width * osd_height *4);
    }
        
    if(osd_buf){
        free(osd_buf);
        printf("free osd_buf \n");
        osd_data=NULL;
        osd_buf=NULL;
    }
    printf("Image processed successfully.\n");
    return 0;
}

/***************************************************
 * 函数原型：static int capture_osd_to_bmp(int argc, char *argv[])
 * 函数功能：截osd层图，生成bmp图片
 * 形参：argc ：
 *      argv ：
 * 返回值:int
 * 作者： lxw--2024/7/27
 * 示例：capture_osd_to_bmp /media/sda/bmp_test.bmp
 * *************************************************/
static int capture_osd_to_bmp(int argc, char *argv[])
{
    if(argc !=2){
        printf("Enter: capture_osd_to_bmp /media/sda/bmp_test.bmp \n");
        return -1;
    }
    get_osd_layer_data();

    if(!osd_buf){
        printf("The osd layer cannot obtain data \n");
        return -1;
    }

    bmp_header_t header = {
        .signature="BM",
        .filesize = sizeof(bmp_header_t) + sizeof(bmp_info_header_t) + osd_width * osd_height * 4,
        .reserved = 0,
        .data_offset = sizeof(bmp_header_t) + sizeof(bmp_info_header_t)
    };

    bmp_info_header_t info_header = {
        .info_size = sizeof(bmp_info_header_t),
        .width = osd_width,
        .height = osd_height,
        .planes = 1,
        .bpp = 32,
        .compression = 0,
        .image_size = osd_width * osd_height * 4,
        .x_ppm = 0,
        .y_ppm = 0,
        .colors = 0,
        .important_colors = 0
    };

    if(!bmp_buf){
        bmp_buf=malloc((sizeof(header))+(sizeof(info_header))+(sizeof(uint32_t))*osd_width*osd_height);
        memset(bmp_buf, 0, (sizeof(header))+(sizeof(info_header))+(sizeof(uint32_t))*osd_width*osd_height); 
        printf("malloc bmp_buf \n");
    }

    memcpy(bmp_buf, &header, sizeof(header));
    memcpy(bmp_buf + sizeof(header), &info_header, sizeof(info_header));

    int bytes_per_row = sizeof(uint32_t) * osd_width;
    size_t copy_offset = sizeof(header) + sizeof(info_header);
    size_t max_copy_bytes = sizeof(header) + sizeof(info_header) + sizeof(uint32_t) * osd_width * osd_height;

    for(uint32_t y = 0; y < osd_height; y++) {
        if (copy_offset + (osd_height - 1 - y) * bytes_per_row + bytes_per_row > max_copy_bytes) {
            printf("Error: memcpy position out of bounds\n");
            return -1;
        }
        memcpy(bmp_buf + sizeof(header) + sizeof(info_header) + (osd_height - 1 - y) * bytes_per_row, (uint32_t *)osd_buf + y * osd_width, bytes_per_row);
    }

    if(osd_buf){
        printf("free osd_buf \n");
        free(osd_buf);
        osd_buf=NULL;
    }

    FILE* bmp_file = fopen(argv[1], "wb");
    if (!bmp_file) {
        printf("Error opening BMP file for reading.\n");
        if(bmp_buf){
            free(bmp_buf);
            bmp_buf=NULL;
            printf("free bmp_buf \n");
        }
        return -1;
    }
    fwrite(bmp_buf, (sizeof(header))+(sizeof(info_header))+(sizeof(uint32_t))*osd_width*osd_height, 1,bmp_file);

    fflush(bmp_file);

    if (bmp_file && fsync(fileno(bmp_file)) == -1) {
        perror("fsync");
    }

    fclose(bmp_file);

    if(bmp_buf){
        free(bmp_buf);
        bmp_buf=NULL;
        printf("free bmp_buf \n");
    }
}

/***************************************************
 * 函数原型：static int capture_dis_to_bmp(int argc, char *argv[])
 * 函数功能：截视频层图，生成bmp图片
 * 形参：argc ：
 *      argv ：
 * 返回值:int
 * 作者： lxw--2024/7/27
 * 示例：capture_dis_to_bmp /media/sda/bmp_test.bmp
 * *************************************************/
static int capture_dis_to_bmp(int argc, char *argv[])
{
    if(argc !=2){
        printf("Enter: capture_dis_to_bmp /media/sda/bmp_test.bmp \n");
        return -1;
    }

    get_dis_layer_data();

    if(!dis_buf){
        printf("The video layer cannot obtain data \n");
        return -1;
    }

    int width=dis_width;
    int height=dis_height;

    bmp_header_t header = {
        .signature="BM",
        .filesize = sizeof(bmp_header_t) + sizeof(bmp_info_header_t) + width * height * 4,
        .reserved = 0,
        .data_offset = sizeof(bmp_header_t) + sizeof(bmp_info_header_t)
    };

    bmp_info_header_t info_header = {
        .info_size = sizeof(bmp_info_header_t),
        .width = width,
        .height = height,
        .planes = 1,
        .bpp = 32,
        .compression = 0,
        .image_size = width * height * 4,
        .x_ppm = 0,
        .y_ppm = 0,
        .colors = 0,
        .important_colors = 0
    };

    if(!bmp_buf){
        bmp_buf=malloc((sizeof(header))+(sizeof(info_header))+(sizeof(uint32_t))*width*height);
        memset(bmp_buf, 0, (sizeof(header))+(sizeof(info_header))+(sizeof(uint32_t))*width*height); 
        printf("malloc bmp_buf \n");
    }

    memcpy(bmp_buf, &header, sizeof(header));
    memcpy(bmp_buf + sizeof(header), &info_header, sizeof(info_header));

    int bytes_per_row = sizeof(uint32_t) * width;
    size_t copy_offset = sizeof(header) + sizeof(info_header);
    size_t max_copy_bytes = sizeof(header) + sizeof(info_header) + sizeof(uint32_t) * width * height;

    for(int y = 0; y < height; y++) {
        if (copy_offset + (height - 1 - y) * bytes_per_row + bytes_per_row > max_copy_bytes) {
            printf("Error: memcpy position out of bounds\n");
            return -1;
        }
        memcpy(bmp_buf + sizeof(header) + sizeof(info_header) + (height - 1 - y) * bytes_per_row, (uint32_t *)dis_buf + y * width, bytes_per_row);
    }

    if(dis_buf){
        printf("free dis_buf \n");
        free(dis_buf);
        dis_buf=NULL;
    }

    FILE* bmp_file = fopen(argv[1], "wb");
    if (!bmp_file) {
        printf("Error opening BMP file for reading.\n");
        if(bmp_buf){
            free(bmp_buf);
            bmp_buf=NULL;
            printf("free bmp_buf \n");
        }
        return -1;
    }
    fwrite(bmp_buf, (sizeof(header))+(sizeof(info_header))+(sizeof(uint32_t))*width*height, 1,bmp_file);

    fflush(bmp_file);

    if (bmp_file && fsync(fileno(bmp_file)) == -1) {
        perror("fsync");
    }

    fclose(bmp_file);

    if(bmp_buf){
        free(bmp_buf);
        bmp_buf=NULL;
        printf("free bmp_buf \n");
    }
}

/***************************************************
 * 函数原型：static int capture_osd_dis_to_bmp(int argc, char *argv[])
 * 函数功能：截osd+视频层图，生成bmp图片
 * 形参：argc ：
 *      argv ：
 * 返回值:int
 * 作者： lxw--2024/7/27
 * 示例：capture_osd_dis_to_bmp /media/sda/osd_dis.bmp
 * *************************************************/
static int capture_osd_dis_to_bmp(int argc, char *argv[])
{

    if(argc !=2){
        printf("Enter: capture_osd_dis_to_bmp /media/sda/osd_dis.bmp \n");
        return -1;
    }

    get_osd_layer_data();
    if(!osd_buf){
        printf("The osd layer cannot obtain data \n");
        return -1;
    }

    get_dis_layer_data();
    if(!dis_buf){
        printf("The video layer cannot obtain data \n");
        return -1;
    }

    if(dis_width != osd_width || dis_height != osd_height){   //如果osd层分辨率 不等于 视频层分辨率
        printf("If the osd layer resolution is not equal to the video layer resolution\n");
        if(!dis_buf2){
            dis_buf2=malloc(osd_width*osd_height*4);
            memset(dis_buf2, 0, osd_width*osd_height*4); 
            printf("malloc dis_buf2 \n");
        }
        resize_with_bilinear_interpolation(dis_buf,dis_buf2,dis_width,dis_height,osd_width,osd_height);
        if(dis_buf){
            free(dis_buf);
            dis_buf=NULL;
            printf("free dis_buf \n");
        }
        merge_raw_files(osd_width,osd_height);
    }else{
        printf("If the osd layer resolution is equal to the video layer resolution\n");
        merge_raw_files(osd_width,osd_height);
    }

    int width=osd_width;
    int height=osd_height;

    bmp_header_t header = {
        .signature="BM",
        .filesize = sizeof(bmp_header_t) + sizeof(bmp_info_header_t) + width * height * 4,
        .reserved = 0,
        .data_offset = sizeof(bmp_header_t) + sizeof(bmp_info_header_t)
    };

    bmp_info_header_t info_header = {
        .info_size = sizeof(bmp_info_header_t),
        .width = width,
        .height = height,
        .planes = 1,
        .bpp = 32,
        .compression = 0,
        .image_size = width * height * 4,
        .x_ppm = 0,
        .y_ppm = 0,
        .colors = 0,
        .important_colors = 0
    };

    if(!bmp_buf){
        bmp_buf=malloc((sizeof(header))+(sizeof(info_header))+(sizeof(uint32_t))*width*height);
        memset(bmp_buf, 0, (sizeof(header))+(sizeof(info_header))+(sizeof(uint32_t))*width*height); 
        printf("malloc bmp_buf \n");
    }

    memcpy(bmp_buf, &header, sizeof(header));
    memcpy(bmp_buf + sizeof(header), &info_header, sizeof(info_header));

    int bytes_per_row = sizeof(uint32_t) * width;
    size_t copy_offset = sizeof(header) + sizeof(info_header);
    size_t max_copy_bytes = sizeof(header) + sizeof(info_header) + sizeof(uint32_t) * width * height;

    for(int y = 0; y < height; y++) {
        if (copy_offset + (height - 1 - y) * bytes_per_row + bytes_per_row > max_copy_bytes) {
            printf("Error: memcpy position out of bounds\n");
            return -1;
        }
        memcpy(bmp_buf + sizeof(header) + sizeof(info_header) + (height - 1 - y) * bytes_per_row, (uint32_t *)osd_dis_buf + y * width, bytes_per_row);
    }

    if(osd_dis_buf){
        printf("free osd_dis_buf \n");
        free(osd_dis_buf);
        osd_dis_buf=NULL;
    }

    FILE* bmp_file = fopen(argv[1], "wb");
    if (!bmp_file) {
        printf("Error opening BMP file for reading.\n");
        if(bmp_buf){
            free(bmp_buf);
            bmp_buf=NULL;
            printf("free bmp_buf \n");
        }
        return -1;
    }
    fwrite(bmp_buf, (sizeof(header))+(sizeof(info_header))+(sizeof(uint32_t))*width*height, 1,bmp_file);

    fflush(bmp_file);

    if (bmp_file && fsync(fileno(bmp_file)) == -1) {
        perror("fsync");
    }

    fclose(bmp_file);

    if(bmp_buf){
        free(bmp_buf);
        bmp_buf=NULL;
        printf("free bmp_buf \n");
    }

}

/***************************************************
 * 函数原型：static int enter_screenshot_test(int argc , char *argv[])
 * 函数功能：进入screenshot
 * 形参：argc ：
 *      argv ：
 * 返回值:int
 * 作者： lxw--2024/7/27
 * *************************************************/
static int enter_screenshot_test(int argc , char *argv[])
{
    (void)argc;
    (void)argv;
    return 0;
}

#ifdef __linux__
static struct termios stored_settings;
static void exit_console(int signo)
{
    (void)signo;
    tcsetattr(0 , TCSANOW , &stored_settings);
    exit(0);
}

int main (int argc, char *argv[])
{
    struct termios new_settings;

	tcgetattr(0, &stored_settings);
	new_settings = stored_settings;
	new_settings.c_lflag &= ~(ICANON | ECHO | ISIG);
	new_settings.c_cc[VTIME] = 0;
	new_settings.c_cc[VMIN] = 1;
	tcsetattr(0, TCSANOW, &new_settings);

	signal(SIGTERM, exit_console);
	signal(SIGINT, exit_console);
	signal(SIGSEGV, exit_console);
	signal(SIGBUS, exit_console);

    console_init("screenshot:");
	// console_register_cmd(NULL, "enter_screenshot_test", enter_screenshot_test, CONSOLE_CMD_MODE_SELF, "enter screenshot test");
	console_register_cmd(NULL, "capture_osd_to_bmp", capture_osd_to_bmp, CONSOLE_CMD_MODE_SELF, "Cut the osd layer diagram and generate bmp images");
	console_register_cmd(NULL, "capture_dis_to_bmp", capture_dis_to_bmp, CONSOLE_CMD_MODE_SELF, "Cut the dis layer diagram and generate bmp images");
	console_register_cmd(NULL, "capture_to_bmp", capture_osd_dis_to_bmp, CONSOLE_CMD_MODE_SELF, "Cut the osd+dis layer diagram and generate bmp images");

    console_start();
    exit_console(0);
    (void)argc;
    (void)argv;
}
#else
CONSOLE_CMD(screenshot , NULL , enter_screenshot_test , CONSOLE_CMD_MODE_SELF , "enter screenshot test")
CONSOLE_CMD(capture_osd_to_bmp, "screenshot" ,  capture_osd_to_bmp, CONSOLE_CMD_MODE_SELF, "Cut the osd layer diagram and generate bmp images")
CONSOLE_CMD(capture_dis_to_bmp, "screenshot",  capture_dis_to_bmp, CONSOLE_CMD_MODE_SELF, "Cut the dis layer diagram and generate bmp images")
CONSOLE_CMD(capture_to_bmp, "screenshot",  capture_osd_dis_to_bmp, CONSOLE_CMD_MODE_SELF, "Cut the osd+dis layer diagram and generate bmp images")
#endif
