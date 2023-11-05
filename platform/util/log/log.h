#ifndef __LOG_H__
#define __LOG_H__
#include <syslog.h>
#include "zlog.h"

extern zlog_category_t* c;
void log_to_mqtt(const char *fmt, ...);
#define log_info(format,...) log_to_mqtt( format, ##__VA_ARGS__);\
                             zlog_debug(c,format, ##__VA_ARGS__)
#define log_crit(format,...) zlog_notice(c,format, ##__VA_ARGS__)
//#define log_dbg(format,...) syslog(LOG_INFO,format, ##__VA_ARGS__)
//#define log_dbg(format,...) syslog(LOG_DEBUG,format, ##__VA_ARGS__)
#define log_dbg(format,...) zlog_debug(c,format, ##__VA_ARGS__)
#endif
