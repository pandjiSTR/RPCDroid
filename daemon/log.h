#pragma once
#include <stdio.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

#define LOG_LEVEL_DEBUG 0
#define LOG_LEVEL_INFO  1
#define LOG_LEVEL_WARN  2
#define LOG_LEVEL_ERROR 3

extern int g_log_level;
extern FILE *g_log_file;

void log_init(const char *path, int level);
void log_close(void);
void log_write(int level, const char *fmt, ...);

#define LOG_DEBUG(...) log_write(LOG_LEVEL_DEBUG, __VA_ARGS__)
#define LOG_INFO(...)  log_write(LOG_LEVEL_INFO,  __VA_ARGS__)
#define LOG_WARN(...)  log_write(LOG_LEVEL_WARN,  __VA_ARGS__)
#define LOG_ERROR(...) log_write(LOG_LEVEL_ERROR, __VA_ARGS__)

#ifdef __cplusplus
}
#endif
