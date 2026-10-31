#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <sys/inotify.h>
#include <sys/select.h>
#include <errno.h>

#include "config.h"
#include "gateway.h"
#include "detector.h"
#include "log.h"

static volatile int g_running = 1;
static volatile int g_reload  = 0;

static void sig_term(int s) { (void)s; g_running = 0; }
static void sig_hup(int s)  { (void)s; g_reload  = 1; }

int main(void)
{
    daemon(0, 0);

    log_init(LOG_FILE, LOG_LEVEL_INFO);
    LOG_INFO("rpcdroid starting");

    char *current_game = NULL;
    long long game_start_ms = 0;

    config_load();
    token_load();
    gamelist_load();

    /* Restore persisted playtime if the game process is still alive */
    {
        char saved_pkg[256] = {0};
        long long saved_start = 0;
        if (runtime_load(saved_pkg, sizeof(saved_pkg), &saved_start) == 0 &&
            saved_pkg[0] && process_running(saved_pkg) && gamelist_get_name(saved_pkg)) {
            current_game = strdup(saved_pkg);
            game_start_ms = saved_start;
            LOG_INFO("Restored playtime for %s (start=%lld)", saved_pkg, saved_start);
        } else {
            runtime_clear();
        }
    }

    if (g_config.log_level > 0)
        g_log_level = g_config.log_level;

    signal(SIGTERM, sig_term);
    signal(SIGINT,  sig_term);
    signal(SIGHUP,  sig_hup);

    int ifd = inotify_init1(IN_NONBLOCK);
    if (ifd < 0) {
        LOG_WARN("inotify_init1 failed, hot-reload disabled");
    } else {
        inotify_add_watch(ifd, CONFIG_DIR, IN_CLOSE_WRITE);
    }

    /* ── Connect to gateway with exponential backoff ─────────────────────── */
    gateway_t *gw = NULL;
    int backoff = 1;
    while (g_running && !gw) {
        gw = gateway_connect(g_token);
        if (!gw) {
            LOG_WARN("Gateway connect failed, retry in %ds", backoff);
            sleep(backoff);
            backoff = backoff < 30 ? backoff * 2 : 30;
        }
    }

    config_write_status(true, NULL, gw ? gw->username : NULL);

    /* Send initial presence (restore timer or clear stale status) */
    if (gw) {
        if (current_game) {
            const char *gname = gamelist_get_name(current_game);
            const char *gicon = gamelist_get_icon(current_game);
            const char *gappid = gamelist_get_app_id(current_game);
            const char *active_app_id = (gappid && gappid[0]) ? gappid : g_config.application_id;
            if (gname) gateway_send_presence(gw, gname, game_start_ms, active_app_id, NULL, gicon);
        } else {
            gateway_send_presence(gw, NULL, 0, g_config.application_id, NULL, NULL);
        }
    }
    time_t last_poll = 0;

    while (g_running) {
        time_t now = time(NULL);

        /* Heartbeat */
        if (gw && gw->heartbeat_interval_ms > 0) {
            int interval_sec = gw->heartbeat_interval_ms / 1000;
            if (!gw->heartbeat_acked &&
                (now - gw->last_heartbeat_sec) > interval_sec + 5) {
                LOG_WARN("Heartbeat zombie, reconnecting");
                goto reconnect;
            }
            if ((now - gw->last_heartbeat_sec) >= interval_sec) {
                gateway_send_heartbeat(gw);
            }
        }

        /* Poll gateway events */
        if (gw) {
            int ret = gateway_poll(gw);
            if (ret == -1 || ret == 2) goto reconnect;
        }

        /* SIGHUP reload */
        if (g_reload) {
            g_reload = 0;
            config_load();
            gamelist_free();
            gamelist_load();
            LOG_INFO("Config reloaded via SIGHUP");
        }

        /* inotify: check config dir changes (non-blocking) */
        if (ifd >= 0) {
            char ibuf[512];
            ssize_t n = read(ifd, ibuf, sizeof(ibuf));
            if (n > 0) {
                char *p = ibuf;
                while (p < ibuf + n) {
                    struct inotify_event *ev = (struct inotify_event *)p;
                    if (ev->len > 0) {
                        if (strcmp(ev->name, "config.json") == 0) {
                            config_load();
                            LOG_INFO("Config hot-reloaded");
                        } else if (strcmp(ev->name, "gamelist.json") == 0) {
                            gamelist_free();
                            gamelist_load();
                            LOG_INFO("Gamelist hot-reloaded");
                        } else if (strcmp(ev->name, "token") == 0) {
                            token_load();
                            LOG_INFO("Token changed, reconnecting");
                            goto reconnect;
                        }
                    }
                    p += sizeof(struct inotify_event) + ev->len;
                }
            }
        }

        /* Game detection */
        if (now - last_poll >= g_config.poll_interval) {
            last_poll = now;
            char *fg = detect_foreground_app();
            const char *fg_name = fg ? gamelist_get_name(fg) : NULL;

            /* Log when foreground app transitions */
            static char *last_logged_fg = NULL;
            if (!last_logged_fg ||
                (fg && strcmp(last_logged_fg, fg) != 0) ||
                (!fg && last_logged_fg[0] != '\0')) {
                LOG_INFO("Foreground: %s (matched game: %s)",
                         fg ? fg : "(none)",
                         fg_name ? fg_name : "no");
                free(last_logged_fg);
                last_logged_fg = fg ? strdup(fg) : strdup("");
            }

            if (fg_name) {
                if (!current_game || strcmp(fg, current_game) != 0) {
                    free(current_game);
                    current_game = strdup(fg);
                    char saved_pkg[256] = {0};
                    long long saved_start = 0;
                    if (runtime_load(saved_pkg, sizeof(saved_pkg), &saved_start) == 0 &&
                        strcmp(saved_pkg, fg) == 0 && saved_start > 0) {
                        game_start_ms = saved_start;
                    } else {
                        game_start_ms = (long long)time(NULL) * 1000LL;
                        runtime_save(fg, game_start_ms);
                    }
                    const char *gicon = gamelist_get_icon(fg);
                    const char *gappid = gamelist_get_app_id(fg);
                    const char *active_app_id = (gappid && gappid[0]) ? gappid : g_config.application_id;
                    LOG_INFO("Game detected: %s (%s, icon: %s, app_id: %s)",
                             fg_name, fg, gicon ? gicon : "none", active_app_id);
                    if (gw) gateway_send_presence(gw, fg_name, game_start_ms,
                                                  active_app_id,
                                                  NULL,
                                                  gicon);
                    config_write_status(true, fg_name, gw ? gw->username : NULL);
                }
            } else {
                if (current_game && process_running(current_game)) {
                    /* game still alive in background: keep presence and timer */
                } else if (current_game) {
                    LOG_INFO("Game closed: %s", current_game);
                    free(current_game);
                    current_game = NULL;
                    game_start_ms = 0;
                    runtime_clear();
                    if (gw) gateway_send_presence(gw, NULL, 0,
                                                  g_config.application_id,
                                                  NULL,
                                                  NULL);
                    config_write_status(true, NULL, gw ? gw->username : NULL);
                }
            }
            free(fg);
        }

        sleep(1);
        continue;

reconnect:
        if (gw) { gateway_disconnect(gw); gw = NULL; }
        config_write_status(false, NULL, NULL);
        backoff = 1;
        while (g_running) {
            sleep(backoff);
            backoff = backoff < 30 ? backoff * 2 : 30;
            token_load();
            gw = gateway_connect(g_token);
            if (gw) {
                LOG_INFO("Reconnected to gateway");
                config_write_status(true, current_game, gw->username);
                if (current_game) {
                    const char *gname = gamelist_get_name(current_game);
                    const char *gicon = gamelist_get_icon(current_game);
                    const char *gappid = gamelist_get_app_id(current_game);
                    const char *active_app_id = (gappid && gappid[0]) ? gappid : g_config.application_id;
                    if (gname) gateway_send_presence(gw, gname, game_start_ms,
                                                     active_app_id,
                                                     NULL,
                                                     gicon);
                } else {
                    gateway_send_presence(gw, NULL, 0, g_config.application_id, NULL, NULL);
                }
                break;
            }
        }
    }

    /* ── Cleanup ──────────────────────────────────────────────────────────── */
    if (gw) {
        gateway_send_presence(gw, NULL, 0,
                              g_config.application_id, g_config.status, NULL);
        gateway_disconnect(gw);
    }
    config_write_status(false, NULL, NULL);
    free(current_game);
    gamelist_free();
    log_close();
    if (ifd >= 0) close(ifd);
    return 0;
}
