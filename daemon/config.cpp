#include "config.h"
#include "log.h"
#include "cJSON.h"
#include "websocket.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>

extern "C" {
Config     g_config  = { .poll_interval = 3, .application_id = "", .log_level = 1 };
GameEntry *g_gamelist = NULL;
char       g_token[512] = {0};

static void ensure_config_dir(void)
{
    struct stat st;
    if (stat(CONFIG_DIR, &st) != 0)
        mkdir(CONFIG_DIR, 0700);
}

int config_load(void)
{
    ensure_config_dir();
    FILE *f = fopen(CONFIG_FILE, "r");
    if (!f) {
        LOG_WARN("config.json not found, using defaults");
        return -1;
    }
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    rewind(f);
    if (sz <= 0) { fclose(f); return -1; }

    char *buf = (char*)malloc(sz + 1);
    if (!buf) { fclose(f); return -1; }
    fread(buf, 1, sz, f);
    buf[sz] = '\0';
    fclose(f);

    cJSON *root = cJSON_Parse(buf);
    free(buf);
    if (!root) {
        LOG_ERROR("Failed to parse config.json");
        return -1;
    }

    cJSON *item;
    item = cJSON_GetObjectItemCaseSensitive(root, "poll_interval");
    if (cJSON_IsNumber(item))
        g_config.poll_interval = (int)item->valuedouble;

    item = cJSON_GetObjectItemCaseSensitive(root, "application_id");
    if (cJSON_IsString(item) && item->valuestring)
        snprintf(g_config.application_id, sizeof(g_config.application_id), "%s", item->valuestring);

    item = cJSON_GetObjectItemCaseSensitive(root, "log_level");
    if (cJSON_IsNumber(item))
        g_config.log_level = (int)item->valuedouble;

    cJSON_Delete(root);
    LOG_INFO("Config loaded: poll=%ds", g_config.poll_interval);
    return 0;
}

void gamelist_free(void)
{
    GameEntry *e = g_gamelist;
    while (e) {
        GameEntry *next = e->next;
        free(e);
        e = next;
    }
    g_gamelist = NULL;
}

static const char *get_default_game_appid(const char *pkg)
{
    if (!pkg) return NULL;
    if (strcmp(pkg, "com.mobile.legends") == 0 || strcmp(pkg, "com.vng.mlbbvn") == 0) return "1441992389232165056";
    if (strcmp(pkg, "com.dts.freefireth") == 0 || strcmp(pkg, "com.dts.freefiremax") == 0 || strcmp(pkg, "com.garena.game.freefire") == 0) return "1441993244895477830";
    if (strcmp(pkg, "com.miHoYo.GenshinImpact") == 0) return "762434991303950386";
    if (strcmp(pkg, "com.HoYoverse.hkrpgoversea") == 0 || strcmp(pkg, "com.miHoYo.hkrpg") == 0 || strcmp(pkg, "com.miHoYo.HonkaiStarRail") == 0) return "1121201675240210523";
    if (strcmp(pkg, "com.miHoYo.bh3oversea") == 0 || strcmp(pkg, "com.miHoYo.bh3") == 0) return "1124351937098301590";
    if (strcmp(pkg, "com.tencent.ig") == 0 || strcmp(pkg, "com.pubg.krmobile") == 0 || strcmp(pkg, "com.rekoo.pubgmnewstate") == 0) return "1402418717781921935";
    if (strcmp(pkg, "com.activision.callofduty.shooter") == 0 || strcmp(pkg, "com.garena.game.codm") == 0) return "1500967742772215959";
    if (strcmp(pkg, "com.riotgames.league.wildrift") == 0 || strcmp(pkg, "com.blizzard.wildrift") == 0) return "1402418696126992445";
    if (strcmp(pkg, "com.supercell.brawlstars") == 0 || strcmp(pkg, "com.supercell.clashroyale") == 0) return "1420796862369239162";
    if (strcmp(pkg, "com.supercell.clashofclans") == 0) return "1441990808399253575";
    if (strcmp(pkg, "com.roblox.client") == 0) return "363445589247131668";
    if (strcmp(pkg, "com.mojang.minecraftpe") == 0) return "1402418491272986635";
    if (strcmp(pkg, "com.kurogame.wutheringwaves.global") == 0 || strcmp(pkg, "com.kurogame.wutheringwaves") == 0) return "1247227126416146462";
    if (strcmp(pkg, "com.YoStarEN.Arknights") == 0 || strcmp(pkg, "com.YoStarKR.Arknights") == 0 || strcmp(pkg, "com.YoStarJP.Arknights") == 0) return "1441993124996972685";
    if (strcmp(pkg, "com.nexon.bluearchive") == 0) return "1390512760860704889";
    if (strcmp(pkg, "com.proximabeta.nikke") == 0 || strcmp(pkg, "com.ShiftUp.NIKKE") == 0) return "1493468373311422615";
    if (strcmp(pkg, "com.levelinfinite.hotta.gp") == 0) return "1355934660961501254";
    if (strcmp(pkg, "com.devsisters.ck") == 0 || strcmp(pkg, "com.devsisters.cookierunkingdom") == 0) return "1442736661237731358";
    if (strcmp(pkg, "com.lilithgame.roc.gp") == 0 || strcmp(pkg, "com.ihg.rise.of.kingdoms") == 0) return "1441992604911665204";
    if (strcmp(pkg, "com.innersloth.spacemafia") == 0) return "1402418440685486130";
    if (strcmp(pkg, "com.epicgames.fortnite") == 0) return "1402418703554842694";
    if (strcmp(pkg, "com.plarium.raidlegends") == 0) return "1201708402397421618";
    if (strcmp(pkg, "com.igg.android.lordsmobile") == 0) return "1124352468223012984";
    if (strcmp(pkg, "com.gameloft.android.ANMP.GloftA9HM") == 0) return "1440142141027520583";
    if (strcmp(pkg, "com.king.candycrushsaga") == 0) return "1440139657655353394";
    if (strcmp(pkg, "com.king.candycrushsodasaga") == 0) return "1441991036334243901";
    if (strcmp(pkg, "com.halfbrick.jetpackjoyride") == 0) return "1440539272900776008";
    if (strcmp(pkg, "com.rovio.baba") == 0) return "1441990393683251361";
    if (strcmp(pkg, "com.levelinfinite.sgame") == 0 || strcmp(pkg, "com.tencent.tmgp.sgame") == 0) return "1506502874044039248";
    return NULL;
}

int gamelist_load(void)
{
    int count = 0;
    FILE *f = fopen(GAMELIST_FILE, "r");
    if (f) {
        fseek(f, 0, SEEK_END);
        long sz = ftell(f);
        rewind(f);
        if (sz > 0) {
            char *buf = (char*)malloc(sz + 1);
            if (buf) {
                if (fread(buf, 1, sz, f) == (size_t)sz) {
                    buf[sz] = '\0';
                    cJSON *root = cJSON_Parse(buf);
                    if (root) {
                        gamelist_free();
                        cJSON *entry = NULL;
                        cJSON_ArrayForEach(entry, root) {
                            if (!entry->string) continue;

                            cJSON *name_item    = cJSON_GetObjectItemCaseSensitive(entry, "name");
                            cJSON *icon_item    = cJSON_GetObjectItemCaseSensitive(entry, "icon");
                            cJSON *app_id_item  = cJSON_GetObjectItemCaseSensitive(entry, "app_id");
                            cJSON *enabled_item = cJSON_GetObjectItemCaseSensitive(entry, "enabled");

                            /* skip disabled entries */
                            if (cJSON_IsBool(enabled_item) && !cJSON_IsTrue(enabled_item))
                                continue;

                            GameEntry *ge = (GameEntry*)calloc(1, sizeof(GameEntry));
                            if (!ge) continue;

                            snprintf(ge->package, sizeof(ge->package), "%s", entry->string);
                            if (cJSON_IsString(name_item) && name_item->valuestring) {
                                /* fix bad legacy default name for mho */
                                if (strcmp(ge->package, "com.garena.game.mho") == 0 && strcmp(name_item->valuestring, "mho") == 0)
                                    snprintf(ge->name, sizeof(ge->name), "Monster Hunter Outlanders");
                                else
                                    snprintf(ge->name, sizeof(ge->name), "%s", name_item->valuestring);
                            } else {
                                if (strcmp(ge->package, "com.garena.game.mho") == 0)
                                    snprintf(ge->name, sizeof(ge->name), "Monster Hunter Outlanders");
                                else
                                    snprintf(ge->name, sizeof(ge->name), "%s", entry->string);
                            }

                            if (cJSON_IsString(icon_item) && icon_item->valuestring)
                                snprintf(ge->icon, sizeof(ge->icon), "%s", icon_item->valuestring);

                            if (cJSON_IsString(app_id_item) && app_id_item->valuestring)
                                snprintf(ge->app_id, sizeof(ge->app_id), "%s", app_id_item->valuestring);

                            /* If app_id not explicitly set, assign known Discord appId */
                            if (ge->app_id[0] == '\0') {
                                const char *def_app = get_default_game_appid(ge->package);
                                if (def_app)
                                    snprintf(ge->app_id, sizeof(ge->app_id), "%s", def_app);
                            }

                            /* Drop dummy/invalid app IDs (MH: World, Kizzy generic) for any game */
                            if (strcmp(ge->app_id, "477152881196269569") == 0 ||
                                strcmp(ge->app_id, "962990036020756480") == 0) {
                                ge->app_id[0] = '\0';
                            }

                            /* If app_id is set (e.g. Mobile Legends), Discord Gateway handles icon natively */
                            if (ge->app_id[0] != '\0' && (strncmp(ge->icon, "http://", 7) == 0 || strncmp(ge->icon, "https://", 8) == 0)) {
                                ge->icon[0] = '\0';
                            }

                            /* Default official icon for MHO if none set */
                            if (strcmp(ge->package, "com.garena.game.mho") == 0 && ge->icon[0] == '\0') {
                                snprintf(ge->icon, sizeof(ge->icon), "https://cdn.discordapp.com/icons/1281639437708820612/97f43c3ef222a90b1cb9241ac390fa41.png");
                            }

                            ge->enabled = true;
                            ge->next = g_gamelist;
                            g_gamelist = ge;
                            count++;
                        }
                        cJSON_Delete(root);
                    } else {
                        LOG_WARN("Failed to parse gamelist.json");
                    }
                }
                free(buf);
            }
        }
        fclose(f);
    } else {
        LOG_WARN("gamelist.json not found: " GAMELIST_FILE);
    }

    /* Fallback: if gamelist.json was empty or missing, load from gamelist.txt */
    if (count == 0) {
        FILE *tf = fopen("/data/adb/modules/discord-rpc/gamelist.txt", "r");
        if (tf) {
            gamelist_free();
            char line[256];
            while (fgets(line, sizeof(line), tf)) {
                size_t l = strlen(line);
                while (l > 0 && (line[l-1] == '\r' || line[l-1] == '\n' || line[l-1] == ' '))
                    line[--l] = '\0';
                if (l == 0 || line[0] == '#') continue;

                GameEntry *ge = (GameEntry*)calloc(1, sizeof(GameEntry));
                if (!ge) continue;
                snprintf(ge->package, sizeof(ge->package), "%s", line);
                if (strcmp(ge->package, "com.garena.game.mho") == 0) {
                    snprintf(ge->name, sizeof(ge->name), "Monster Hunter Outlanders");
                    snprintf(ge->icon, sizeof(ge->icon), "https://cdn.discordapp.com/icons/1281639437708820612/97f43c3ef222a90b1cb9241ac390fa41.png");
                } else {
                    const char *dot = strrchr(line, '.');
                    snprintf(ge->name, sizeof(ge->name), "%s", dot ? dot + 1 : line);
                }
                const char *def_app = get_default_game_appid(ge->package);
                if (def_app)
                    snprintf(ge->app_id, sizeof(ge->app_id), "%s", def_app);

                ge->enabled = true;
                ge->next = g_gamelist;
                g_gamelist = ge;
                count++;
            }
            fclose(tf);
            LOG_INFO("Loaded %d games from gamelist.txt fallback", count);
        }
    }

    LOG_INFO("Gamelist loaded: %d games active", count);
    return 0;
}

int token_load(void)
{
    FILE *f = fopen(TOKEN_FILE, "r");
    if (!f) {
        LOG_ERROR("Token file not found: " TOKEN_FILE);
        return -1;
    }
    size_t n = fread(g_token, 1, sizeof(g_token) - 1, f);
    fclose(f);
    g_token[n] = '\0';
    /* strip trailing whitespace/newline */
    while (n > 0 && (g_token[n-1] == '\n' || g_token[n-1] == '\r' || g_token[n-1] == ' '))
        g_token[--n] = '\0';
    if (n == 0) {
        LOG_ERROR("Token file is empty");
        return -1;
    }
    LOG_INFO("Token loaded (%zu chars)", n);
    return 0;
}

const char *gamelist_get_name(const char *package)
{
    if (!package) return NULL;
    for (GameEntry *e = g_gamelist; e; e = e->next) {
        if (e->enabled && strcmp(e->package, package) == 0)
            return e->name;
    }
    return NULL;
}

static void resolve_external_icon(GameEntry *e)
{
    if (!e || e->app_id[0] != '\0') return;
    /* If already has mp: or registered asset key, no need to resolve */
    if (e->icon[0] && strncmp(e->icon, "http://", 7) != 0 && strncmp(e->icon, "https://", 8) != 0)
        return;

    /* Check persistent cache first */
    const char *cached = icon_cache_get(e->package);
    if (cached) {
        snprintf(e->icon, sizeof(e->icon), "%s", cached);
        return;
    }

    /* If icon is an http/https URL, resolve via native Discord API */
    if (strncmp(e->icon, "http://", 7) == 0 || strncmp(e->icon, "https://", 8) == 0) {
        if (g_token[0] == '\0') token_load();
        if (g_token[0] == '\0') {
            LOG_WARN("Cannot resolve external icon for %s: Discord token not loaded", e->package);
            return;
        }

        const char *aid = (e->app_id[0] != '\0') ? e->app_id : g_config.application_id;
        if (!aid || aid[0] == '\0') aid = "962990036020756480";

        char mp_buf[512] = {0};
        LOG_INFO("Resolving external icon via Discord API for %s (app_id: %s)...", e->package, aid);
        int res = discord_resolve_external_asset(aid, g_token, e->icon, mp_buf, sizeof(mp_buf));
        if (res == 0 && mp_buf[0]) {
            LOG_INFO("Resolved media proxy icon for %s: %s", e->package, mp_buf);
            snprintf(e->icon, sizeof(e->icon), "%s", mp_buf);
            icon_cache_set(e->package, mp_buf);
        } else {
            LOG_WARN("Failed to resolve media proxy icon for %s", e->package);
            e->icon[0] = '\0';
        }
    }
}

const char *gamelist_get_icon(const char *package)
{
    if (!package) return NULL;
    for (GameEntry *e = g_gamelist; e; e = e->next) {
        if (e->enabled && strcmp(e->package, package) == 0) {
            resolve_external_icon(e);
            return e->icon[0] ? e->icon : NULL;
        }
    }
    return NULL;
}

const char *gamelist_get_app_id(const char *package)
{
    if (!package) return NULL;
    for (GameEntry *e = g_gamelist; e; e = e->next) {
        if (e->enabled && strcmp(e->package, package) == 0)
            return e->app_id[0] ? e->app_id : NULL;
    }
    return NULL;
}

/* Persistent error shown in status.json until daemon restarts (e.g. binder init failure) */
static char g_status_error[256] = {0};

void config_write_status(bool connected, const char *game, const char *discord_username, const char *discord_avatar)
{
    static bool last_connected = false;
    static char last_game[256] = {0};
    static char last_user[256] = {0};
    static char last_avatar[256] = {0};
    static int initialized = 0;

    const char *g = game ? game : "";
    const char *u = discord_username ? discord_username : "";
    const char *a = discord_avatar ? discord_avatar : "";

    if (initialized && last_connected == connected &&
        strcmp(last_game, g) == 0 && strcmp(last_user, u) == 0 &&
        strcmp(last_avatar, a) == 0) {
        return; /* no change, skip write */
    }

    initialized = 1;
    last_connected = connected;
    snprintf(last_game, sizeof(last_game), "%s", g);
    snprintf(last_user, sizeof(last_user), "%s", u);
    snprintf(last_avatar, sizeof(last_avatar), "%s", a);

    ensure_config_dir();
    FILE *f = fopen(STATUS_FILE, "w");
    if (!f) return;

    cJSON *root = cJSON_CreateObject();
    cJSON_AddBoolToObject(root, "connected", connected);
    cJSON_AddStringToObject(root, "current_game",       game             ? game             : "");
    cJSON_AddStringToObject(root, "discord_username",   discord_username ? discord_username : "");
    cJSON_AddStringToObject(root, "discord_avatar",     discord_avatar   ? discord_avatar   : "");
    cJSON_AddNumberToObject(root, "pid", (double)getpid());
    if (g_status_error[0]) cJSON_AddStringToObject(root, "error", g_status_error);

    char *s = cJSON_PrintUnformatted(root);
    if (s) {
        fputs(s, f);
        free(s);
    }
    cJSON_Delete(root);
    fclose(f);
}

/* ── icon cache ──────────────────────────────────────────────────────── */

static char icon_cache_buf[512] = {0};

const char *icon_cache_get(const char *package)
{
    if (!package || !package[0]) return NULL;
    icon_cache_buf[0] = '\0';
    FILE *f = fopen(ICON_CACHE_FILE, "r");
    if (!f) return NULL;
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    rewind(f);
    if (sz <= 0) { fclose(f); return NULL; }
    char *buf = (char*)malloc(sz + 1);
    if (!buf) { fclose(f); return NULL; }
    if (fread(buf, 1, sz, f) != (size_t)sz) { free(buf); fclose(f); return NULL; }
    buf[sz] = '\0';
    fclose(f);
    cJSON *root = cJSON_Parse(buf);
    free(buf);
    if (!root) return NULL;
    cJSON *item = cJSON_GetObjectItemCaseSensitive(root, package);
    if (cJSON_IsString(item) && item->valuestring && item->valuestring[0]) {
        snprintf(icon_cache_buf, sizeof(icon_cache_buf), "%s", item->valuestring);
    }
    cJSON_Delete(root);
    return icon_cache_buf[0] ? icon_cache_buf : NULL;
}

void icon_cache_set(const char *package, const char *mp_url)
{
    if (!package || !package[0] || !mp_url || !mp_url[0]) return;
    ensure_config_dir();
    cJSON *root = NULL;
    FILE *f = fopen(ICON_CACHE_FILE, "r");
    if (f) {
        fseek(f, 0, SEEK_END);
        long sz = ftell(f);
        rewind(f);
        if (sz > 0) {
            char *buf = (char*)malloc(sz + 1);
            if (buf && fread(buf, 1, sz, f) == (size_t)sz) {
                buf[sz] = '\0';
                root = cJSON_Parse(buf);
            }
            free(buf);
        }
        fclose(f);
    }
    if (!root) root = cJSON_CreateObject();
    cJSON_DeleteItemFromObject(root, package);
    cJSON_AddStringToObject(root, package, mp_url);
    char *s = cJSON_PrintUnformatted(root);
    if (s) {
        FILE *out = fopen(ICON_CACHE_FILE, "w");
        if (out) { fputs(s, out); fclose(out); }
        free(s);
    }
    cJSON_Delete(root);
}

int runtime_save(const char *package, long long start_ms)
{
    if (!package || !package[0]) return -1;
    ensure_config_dir();
    FILE *f = fopen(RUNTIME_FILE, "w");
    if (!f) return -1;
    fprintf(f, "{\"game\":\"%s\",\"start\":%lld}", package, start_ms);
    fclose(f);
    return 0;
}

int runtime_load(char *package, size_t pkg_len, long long *start_ms)
{
    if (!package || pkg_len == 0 || !start_ms) return -1;
    package[0] = '\0';
    *start_ms = 0;
    FILE *f = fopen(RUNTIME_FILE, "r");
    if (!f) return -1;
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    rewind(f);
    if (sz <= 0) { fclose(f); return -1; }
    char *buf = (char*)malloc(sz + 1);
    if (!buf) { fclose(f); return -1; }
    if (fread(buf, 1, sz, f) != (size_t)sz) { free(buf); fclose(f); return -1; }
    buf[sz] = '\0';
    fclose(f);
    cJSON *root = cJSON_Parse(buf);
    free(buf);
    if (!root) return -1;
    cJSON *g = cJSON_GetObjectItemCaseSensitive(root, "game");
    cJSON *s = cJSON_GetObjectItemCaseSensitive(root, "start");
    if (cJSON_IsString(g) && g->valuestring && cJSON_IsNumber(s)) {
        snprintf(package, pkg_len, "%s", g->valuestring);
        *start_ms = (long long)s->valuedouble;
    }
    cJSON_Delete(root);
    return (package[0] && *start_ms > 0) ? 0 : -1;
}

void runtime_clear(void)
{
    remove(RUNTIME_FILE);
}


/* Write an error message into status.json (preserving other fields) */
void config_write_error(const char *error_msg) {
    snprintf(g_status_error, sizeof(g_status_error), "%s", error_msg ? error_msg : "");
    ensure_config_dir();
    FILE *f = fopen(STATUS_FILE, "r");
    char game[256] = {0}, user[256] = {0}, avatar[256] = {0};
    int connected = 0, pid = (int)getpid();
    if (f) {
        fseek(f, 0, SEEK_END);
        long sz = ftell(f);
        rewind(f);
        if (sz > 0) {
            char *buf = (char*)malloc(sz + 1);
            if (buf && fread(buf, 1, sz, f) == (size_t)sz) {
                buf[sz] = '\0';
                cJSON *root = cJSON_Parse(buf);
                if (root) {
                    cJSON *c = cJSON_GetObjectItemCaseSensitive(root, "connected");
                    if (cJSON_IsBool(c)) connected = cJSON_IsTrue(c);
                    cJSON *g = cJSON_GetObjectItemCaseSensitive(root, "current_game");
                    if (cJSON_IsString(g) && g->valuestring) snprintf(game, sizeof(game), "%s", g->valuestring);
                    cJSON *u = cJSON_GetObjectItemCaseSensitive(root, "discord_username");
                    if (cJSON_IsString(u) && u->valuestring) snprintf(user, sizeof(user), "%s", u->valuestring);
                    cJSON *a = cJSON_GetObjectItemCaseSensitive(root, "discord_avatar");
                    if (cJSON_IsString(a) && a->valuestring) snprintf(avatar, sizeof(avatar), "%s", a->valuestring);
                    cJSON *p = cJSON_GetObjectItemCaseSensitive(root, "pid");
                    if (cJSON_IsNumber(p)) pid = (int)p->valuedouble;
                    cJSON_Delete(root);
                }
            }
            free(buf);
        }
        fclose(f);
    }
    f = fopen(STATUS_FILE, "w");
    if (!f) return;
    cJSON *root = cJSON_CreateObject();
    cJSON_AddBoolToObject(root, "connected", connected);
    cJSON_AddStringToObject(root, "current_game", game);
    cJSON_AddStringToObject(root, "discord_username", user);
    cJSON_AddStringToObject(root, "discord_avatar", avatar);
    cJSON_AddNumberToObject(root, "pid", pid);
    if (error_msg && error_msg[0]) cJSON_AddStringToObject(root, "error", error_msg);
    char *s = cJSON_PrintUnformatted(root);
    if (s) { fputs(s, f); free(s); }
    cJSON_Delete(root);
    fclose(f);
}
}
