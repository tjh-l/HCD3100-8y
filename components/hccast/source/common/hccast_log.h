#ifndef _HCCAST_LOG_H_
#define _HCCAST_LOG_H_

enum loglevel {
	LL_FATAL = 0,
	LL_ERROR,
	LL_WARNING,
	LL_NOTICE,
	LL_INFO,
	LL_DEBUG,
	LL_SPEW,
	LL_FLOOD,
};

void hccast_log_level_set(int level);

extern int g_hccast_log_level;

#define hccast_log(level, fmt, ...) \
    do { \
        if (level <= g_hccast_log_level) printf("[hccast]"fmt, ##__VA_ARGS__); \
    } while(0)


#endif
