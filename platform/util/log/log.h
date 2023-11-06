#ifndef __LOG_H__
#define __LOG_H__
#include "zlog.h"
#include <syslog.h>

extern zlog_category_t *zlog_c;
void log_to_mqtt(const char *fmt, ...);
int log_init();
// #ifndef DEBUG
#define log_info(format, ...) zlog_debug(zlog_c, format, ##__VA_ARGS__)
#define log_crit(format, ...) zlog_notice(zlog_c, format, ##__VA_ARGS__)
#define log_dbg(format, ...)            \
    log_to_mqtt(format, ##__VA_ARGS__); \
    zlog_debug(zlog_c, format, ##__VA_ARGS__)
#define log_err(format, ...) zlog_error(zlog_c, format, ##__VA_ARGS__)
// #else
// #define log_info(format, ...)
// #define log_crit(format, ...)
// #define log_dbg(format, ...)
// #define log_err(format, ...)
// #endif

#endif
