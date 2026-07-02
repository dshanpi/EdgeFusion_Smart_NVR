#include "log.h"

#include <pthread.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>

static log_level_t g_level = LOG_INFO;
static FILE       *g_fp    = NULL;
static pthread_mutex_t g_lock = PTHREAD_MUTEX_INITIALIZER;

static const char *level_str(log_level_t l)
{
    switch (l) {
    case LOG_DEBUG: return "DEBUG";
    case LOG_INFO:  return "INFO ";
    case LOG_WARN:  return "WARN ";
    case LOG_ERROR: return "ERROR";
    }
    return "?    ";
}

void log_set_level(log_level_t lvl) { g_level = lvl; }

void log_set_file(const char *path)
{
    if (g_fp && g_fp != stdout && g_fp != stderr) fclose(g_fp);
    if (!path || !path[0]) { g_fp = NULL; return; }
    /* vfat 不支持某些权限/属主，普通 append 打开即可 */
    g_fp = fopen(path, "a");
    /* 失败不致命，回退到 stdout */
}

void log_write(log_level_t lvl, const char *file, int line, const char *fmt, ...)
{
    if (lvl < g_level) return;

    char timebuf[32];
    time_t t = time(NULL);
    struct tm tm;
    localtime_r(&t, &tm);
    strftime(timebuf, sizeof(timebuf), "%Y-%m-%d %H:%M:%S", &tm);

    /* 取文件名而非全路径 */
    const char *p = strrchr(file, '/');
    if (!p) p = file; else p++;

    FILE *out = g_fp ? g_fp : stdout;

    pthread_mutex_lock(&g_lock);
    fprintf(out, "[%s] [%s] [%s:%d] ", timebuf, level_str(lvl), p, line);

    va_list ap;
    va_start(ap, fmt);
    vfprintf(out, fmt, ap);
    va_end(ap);

    fputc('\n', out);
    fflush(out);
    pthread_mutex_unlock(&g_lock);
}
