/*
app_config.h: the global config header file for application
 */
#ifndef __HCDEMO_CONFIG_H__
#define __HCDEMO_CONFIG_H__

#ifdef __linux__
#include <stdbool.h> //bool
#include <stdint.h> //uint32_t
#include <sys/types.h> //uint
#else
#include <generated/br2_autoconf.h>
#endif

#endif //end of __HCDEMO_CONFIG_H__

