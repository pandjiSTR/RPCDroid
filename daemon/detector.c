#include "detector.h"
#include "config.h"
#include "log.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Extract package name from a line containing "<package>/<activity>"
   Returns newly malloc'd string or NULL. */
static char *extract_package_from_line(const char *line)
{
    if (!line) return NULL;

    const char *cur = line;
    while ((cur = strchr(cur, '/')) != NULL) {
        /* Walk backwards from '/' to find start of package name */
        const char *p = cur - 1;
        while (p >= line && (isalnum((unsigned char)*p) || *p == '.' || *p == '_')) {
            p--;
        }
        const char *start = p + 1;
        size_t len = (size_t)(cur - start);

        /* Validate package name:
           - at least 3 chars
           - starts with letter or underscore
           - contains at least one dot '.'
           - max 128 chars */
        if (len >= 3 && len < 128 &&
            (isalpha((unsigned char)start[0]) || start[0] == '_') &&
            memchr(start, '.', len) != NULL) {
            char *pkg = malloc(len + 1);
            if (pkg) {
                memcpy(pkg, start, len);
                pkg[len] = '\0';
                return pkg;
            }
        }
        cur++; /* search next slash in line */
    }
    return NULL;
}

char *detect_foreground_app(void)
{
    /* Query window manager and activity task manager for foreground focus:
       - dumpsys window (mCurrentFocus, mFocusedApp)
       - dumpsys activity top (topmost task activity)
       - dumpsys activity activities (fallback) */
    static const char *cmd =
        "dumpsys window 2>/dev/null | grep -E 'mCurrentFocus|mFocusedApp'; "
        "dumpsys activity top 2>/dev/null | grep -m1 'ACTIVITY '; "
        "dumpsys activity activities 2>/dev/null | grep -E 'mResumedActivity|topResumedActivity'";

    FILE *fp = popen(cmd, "r");
    if (!fp) return NULL;

    char line[512] = {0};
    char *first_pkg = NULL;
    char *matched_game_pkg = NULL;

    while (fgets(line, sizeof(line), fp)) {
        char *pkg = extract_package_from_line(line);
        if (!pkg) continue;

        /* If this package is in gamelist, prioritize it immediately */
        if (gamelist_get_name(pkg)) {
            matched_game_pkg = pkg;
            break;
        }

        /* Otherwise remember the first valid foreground package found */
        if (!first_pkg) {
            first_pkg = pkg;
        } else {
            free(pkg);
        }
    }
    pclose(fp);

    if (matched_game_pkg) {
        free(first_pkg);
        return matched_game_pkg;
    }
    return first_pkg;
}
