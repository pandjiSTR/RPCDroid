#pragma once
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define CONFIG_DIR      "/data/adb/.config/drpc"
#define TOKEN_FILE      CONFIG_DIR "/token"
#define CONFIG_FILE     CONFIG_DIR "/config.json"
#define GAMELIST_FILE   CONFIG_DIR "/gamelist.json"
#define STATUS_FILE     CONFIG_DIR "/status.json"
#define LOG_FILE        CONFIG_DIR "/drpc.log"
#define RUNTIME_FILE    CONFIG_DIR "/runtime.json"
#define ICON_CACHE_FILE CONFIG_DIR "/icon_cache.json"

typedef struct {
    int  poll_interval;       /* seconds between foreground app checks, default 5 */
    char application_id[32]; /* Discord application ID */
    int  log_level;           /* 0=debug 1=info 2=warn 3=error */
} Config;

typedef struct GameEntry {
    char package[256];
    char name[256];
    char icon[512];
    char app_id[32];
    bool enabled;
    struct GameEntry *next;
} GameEntry;

extern Config     g_config;
extern GameEntry *g_gamelist;
extern char       g_token[512];

int         config_load(void);
int         gamelist_load(void);
int         token_load(void);
void        gamelist_free(void);
const char *gamelist_get_name(const char *package);   /* NULL if not found/disabled */
const char *gamelist_get_icon(const char *package);   /* NULL if not found/no icon */
const char *gamelist_get_app_id(const char *package); /* NULL if not found/no custom app_id */
void        config_write_status(bool connected, const char *game, const char *discord_username, const char *discord_avatar);
int         runtime_save(const char *package, long long start_ms);
int         runtime_load(char *package, size_t pkg_len, long long *start_ms);
void        runtime_clear(void);
const char *icon_cache_get(const char *package);   /* NULL if not cached */
void        icon_cache_set(const char *package, const char *mp_url);

#ifdef __cplusplus
}
#endif
