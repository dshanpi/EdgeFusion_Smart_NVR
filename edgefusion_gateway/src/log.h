#ifndef EDGEFUSION_LOG_H
#define EDGEFUSION_LOG_H

#include <stdio.h>

typedef enum {
    LOG_DEBUG = 0,
    LOG_INFO,
    LOG_WARN,
    LOG_ERROR,
} log_level_t;

/* 全局日志级别，由 conf.c 设置 */
void log_set_level(log_level_t lvl);
void log_set_file(const char *path);

void log_write(log_level_t lvl, const char *file, int line, const char *fmt, ...);

#define LOG_DBG(fmt, ...) log_write(LOG_DEBUG, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define LOG_INF(fmt, ...) log_write(LOG_INFO,  __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define LOG_WRN(fmt, ...) log_write(LOG_WARN,  __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define LOG_ERR(fmt, ...) log_write(LOG_ERROR, __FILE__, __LINE__, fmt, ##__VA_ARGS__)

#endif
