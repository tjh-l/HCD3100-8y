#ifndef _TP2804_REG_H_
#define _TP2804_REG_H_

enum {
    VIN1 = 0 ,
    VIN2 = 1 ,
    VIN3 = 2 ,
    VIN4 = 3 ,
};
enum {
    STD_TVI ,
    STD_HDA , //AHD
};
enum {
    HD25 ,
    HD30 ,
    HD275 ,	//720p27.5
    FHD25 ,
    FHD30 ,
    FHD275 ,	//1080p27.5
    FHD28 ,
    UVGA25 ,  //1280x960p25
    UVGA30 ,  //1280x960p30
    FHD25_X3C ,
    F_UVGA30 ,  //FH 1280x960p30, 1800x1000
    HD50 ,
    HD60 ,
    FHD_X3C ,
};
////////////////////
void TP9951_sensor_init(unsigned char ch , unsigned char fmt , unsigned char std);
void tp2804_debug(void);
#endif