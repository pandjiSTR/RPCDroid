#include "log.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <unistd.h>
#include <sys/stat.h>
#include <stdarg.h>

int g_log_level = LOG_LEVEL_INFO;
FILE *g_log_file = nullptr;

static const char *level_str[] = {"DEBUG", "INFO", "WARN", "ERROR"};

static const size_t MAX_LOG_SIZE = 512 * 1024;

void log_init(const char *path, int level) {
    g_log_level = level;
    if (g_log_file && g_log_file != stderr) fclose(g_log_file);
    g_log_file = fopen(path, "a");
    if (!g_log_file) g_log_file = stderr;
}

void log_close(void) {
    if (g_log_file && g_log_file != stderr) {
        fclose(g_log_file);
        g_log_file = nullptr;
    }
}

void log_write(int level, const char *fmt, ...) {
    if (level < g_log_level) return;
    if (!g_log_file) g_log_file = stderr;

    if (g_log_file != stderr) {
        fflush(g_log_file);
        struct stat st;
        if (fstat(fileno(g_log_file), &st) == 0 && st.st_size > (off_t)MAX_LOG_SIZE) {
            ftruncate(fileno(g_log_file), 0);
            rewind(g_log_file);
        }
    }

    time_t now = time(nullptr);
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
