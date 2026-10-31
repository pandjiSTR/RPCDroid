#include "log.h"
#include <stdarg.h>
#include <string.h>

int   g_log_level = LOG_LEVEL_INFO;
FILE *g_log_file  = NULL;

static const char *level_str[] = {"DEBUG", "INFO", "WARN", "ERROR"};

void log_init(const char *path, int level)
{
    g_log_level = level;
    if (g_log_file && g_log_file != stderr)
        fclose(g_log_file);
    g_log_file = fopen(path, "a");
    if (!g_log_file)
        g_log_file = stderr;
}

void log_close(void)
{
    if (g_log_file && g_log_file != stderr) {
        fclose(g_log_file);
        g_log_file = NULL;
    }
}

void log_write(int level, const char *fmt, ...)
{
    if (level < g_log_level)
        return;
    if (!g_log_file)
        g_log_file = stderr;

    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    char ts[32];
    strftime(ts, sizeof(ts), "%Y-%m-%d %H:%M:%S", t);

    fprintf(g_log_file, "[%s] [%s] ", ts, level_str[level < 4 ? level : 3]);

    va_list ap;
    va_start(ap, fmt);
    vfprintf(g_log_file, fmt, ap);
    va_end(ap);

    fputc('\n', g_log_file);
    fflush(g_log_file);
}
