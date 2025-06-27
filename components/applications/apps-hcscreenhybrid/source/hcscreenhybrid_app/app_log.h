#ifndef _APP_LOG_H_
#define _APP_LOG_H_

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

int app_log_level_get(void);
void app_log_level_set(int level);

extern int g_app_log_level;

#define app_log(level, fmt, ...) \
    do { \
        if (level <= g_app_log_level) printf("[app]"fmt"\n", ##__VA_ARGS__); \
    } while(0)

#endif
