
/**
 * @file nds03_dev.h
 * @author tongsheng.tang
 * @brief NDS03 device setting functions
 * @version 2.x.x
 * @date 2024-04
 * 
 * @copyright Copyright (c) 2024, Nephotonics Information Technology (Hefei) Co., Ltd.
 * 
 */
#ifndef __NDS03_STDINT_H__
#define __NDS03_STDINT_H__


#define PLATFORM_C51            0
#define PLATFORM_NOT_C51        1
#define PLATFORM_LINUX_DRIVER   2
#ifndef NDS03_PLATFORM
#define NDS03_PLATFORM          PLATFORM_NOT_C51    
#endif

#if NDS03_PLATFORM == PLATFORM_NOT_C51
    #include <stdint.h>

#elif NDS03_PLATFORM == PLATFORM_C51
    /* exact-width signed integer types */
    typedef   signed        char    int8_t;
    typedef   signed        int     int16_t;
    typedef   signed        long    int32_t;

        /* exact-width unsigned integer types */
    typedef unsigned        char    uint8_t;
    typedef unsigned        int     uint16_t;
    typedef unsigned        long    uint32_t;

    #define __func__          __FILE__


    /* minimum values of exact-width signed integer types */
    #define INT32_MIN               (~0x7fffffff)  /* -2147483648 is unsigned */

    /* maximum values of exact-width signed integer types */
    #define INT32_MAX                 2147483647

    /* maximum values of exact-width unsigned integer types */
    #define UINT32_MAX               4294967295u

    /* 7.18.2.2 */

    /* minimum values of minimum-width signed integer types */
    #define INT_LEAST32_MIN          (~0x7fffffff)

    /* maximum values of minimum-width signed integer types */
    #define INT_LEAST32_MAX            2147483647

    /* maximum values of minimum-width unsigned integer types */
    #define UINT_LEAST32_MAX           4294967295u

    /* 7.18.2.3 */

    /* minimum values of fastest minimum-width signed integer types */
    #define INT_FAST32_MIN           (~0x7fffffff)

    /* maximum values of fastest minimum-width signed integer types */
    #define INT_FAST32_MAX             2147483647

    /* maximum values of fastest minimum-width unsigned integer types */
    #define UINT_FAST32_MAX            4294967295u
#elif NDS03_PLATFORM == PLATFORM_LINUX_DRIVER
  /* exact-width signed integer types */
    typedef   signed        char    int8_t;
    typedef   signed        short   int16_t;
    typedef   signed        int     int32_t;

        /* exact-width unsigned integer types */
    typedef unsigned        char    uint8_t;
    typedef unsigned        short   uint16_t;
    typedef unsigned        int     uint32_t;

    #define __func__          __FILE__


    /* minimum values of exact-width signed integer types */
    #define INT32_MIN               (~0x7fffffff)  /* -2147483648 is unsigned */

    /* maximum values of exact-width signed integer types */
    #define INT32_MAX                 2147483647

    /* maximum values of exact-width unsigned integer types */
    #define UINT32_MAX               4294967295u
#endif 

#endif  /** __NDS03_STDINT_H__ */

