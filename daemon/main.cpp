#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <mutex>
#include <string>
#include <thread>
#include <unistd.h>
#include <signal.h>

extern "C" {
#include "config.h"
#include "gateway.h"
#include "log.h"
}

#include "base/BinderMonitor/BinderMonitor.hpp"
#include "include/EncoreLog.hpp"

// EncoreLog uses spdlog; define a minimal log macro if needed.
// We'll use our own log (log.h) instead.

static volatile int g_running = 1;
static void sig_term(int) { g_running = 0; }

static gateway_t *g_gw = nullptr;
static char *g_current_game = nullptr;
static long long g_game_start_ms = 0;
static int g_active_game_pid = 0;
static char g_last_sig[1024] = {0};   /* dedupe key for last presence sent; reset on game close */
static std::mutex g_state_mtx;
static std::atomic<bool> g_pending_clear{false};
static std::atomic<bool> g_pending_game{false};
static char g_pending_pkg[256] = {0};
static int g_pending_pid = 0;

static void on_fg(int32_t pid, int32_t uid, bool foreground) {
    if (!foreground) return;
    std::string pkg = BinderMonitor::get().getPackageNameForUid(uid);
    // sanitize null
    if (auto pos = pkg.find('\0'); pos != std::string::npos) pkg.resize(pos);
    if (pkg.empty()) return;

    LOG_DEBUG("FG activity: %s (pid=%d uid=%d)", pkg.c_str(), pid, uid);

    // check if game
    const char *gname = gamelist_get_name(pkg.c_str());
    if (!gname) {
        // not a game: note foreground changed (clear handled by fallback poll)
        return;
    }
    std::lock_guard<std::mutex> lk(g_state_mtx);
    if (!g_current_game || strcmp(g_current_game, pkg.c_str()) != 0) {
        snprintf(g_pending_pkg, sizeof(g_pending_pkg), "%s", pkg.c_str());
        g_pending_pid = pid;
        g_pending_game.store(true);
    }
}

static void on_died(int32_t pid, int32_t uid) {
    std::lock_guard<std::mutex> lk(g_state_mtx);
    if (g_active_game_pid == pid) {
        LOG_INFO("Process died: pid=%d uid=%d (active game)", pid, uid);
        g_pending_clear.store(true);
    }
}

int main(int argc, char *argv[]) {
    (void)argc; (void)argv;
    daemon(0, 0);

    /* Make daemon unkillable by LMK */
    {
        FILE *oom = fopen("/proc/self/oom_score_adj", "w");
        if (oom) { fprintf(oom, "-1000"); fclose(oom); }
    }

    log_init(LOG_FILE, LOG_LEVEL_INFO);
    LOG_INFO("rpcdroid daemon started (pid=%d)", (int)getpid());

    config_load();
    token_load();
    gamelist_load();

    if (g_config.log_level > 0) g_log_level = g_config.log_level;

    signal(SIGTERM, sig_term);
    signal(SIGINT, sig_term);

    // Initialize binder monitor
    ProcessObserverCallbacks cbs;
    cbs.onForegroundActivitiesChanged = on_fg;
    cbs.onProcessDied = on_died;
    BinderMonitor::get().setProcessObserverCallbacks(cbs);
    if (!BinderMonitor::get().initialize()) {
        LOG_ERROR("BinderMonitor init failed, falling back to polling");
        // fall through; main loop will poll
    }

    // Connect to gateway
    while (g_running && !g_gw) {
        g_gw = gateway_connect(g_token);
        if (!g_gw) { sleep(2); }
    }
    if (g_gw) config_write_status(true, nullptr, g_gw->username, g_gw->avatar_url);

    // Block on binder thread pool in a separate thread
    std::thread binder_thread([]() {
        BinderMonitor::get().joinThreadPool();
    });

    time_t last_poll = 0;
    while (g_running) {
        time_t now = time(nullptr);

        // Handle pending game start
        if (g_pending_game.load()) {
            g_pending_game.store(false);
            char pkg[256];
            int pid;
            {
                std::lock_guard<std::mutex> lk(g_state_mtx);
                snprintf(pkg, sizeof(pkg), "%s", g_pending_pkg);
                pid = g_pending_pid;
            }
            free(g_current_game);
            g_current_game = strdup(pkg);
            g_active_game_pid = pid;
            g_game_start_ms = (long long)now * 1000LL;
            const char *gname = gamelist_get_name(pkg);
            const char *gicon = gamelist_get_icon(pkg);
            const char *gappid = gamelist_get_app_id(pkg);
            const char *aid = (gappid && gappid[0]) ? gappid : g_config.application_id;
            char sig[1024];
            snprintf(sig, sizeof(sig), "%s|%s|%s", gname ? gname : "", aid ? aid : "", gicon ? gicon : "");
            if (!(g_last_sig[0] && strcmp(g_last_sig, sig) == 0)) {
                if (gname && g_gw) gateway_send_presence(g_gw, gname, g_game_start_ms, aid, gicon);
                snprintf(g_last_sig, sizeof(g_last_sig), "%s", sig);
            }
            if (gname) config_write_status(true, gname, g_gw ? g_gw->username : nullptr, g_gw ? g_gw->avatar_url : nullptr);
        }

        // Handle pending clear (process died)
        if (g_pending_clear.load()) {
            g_pending_clear.store(false);
            if (g_current_game) {
                LOG_INFO("Game process died: %s", g_current_game);
                free(g_current_game);
                g_current_game = nullptr;
                g_game_start_ms = 0;
                runtime_clear();
                g_last_sig[0] = '\0';
                if (g_gw) gateway_send_presence(g_gw, nullptr, 0, g_config.application_id, nullptr);
                config_write_status(true, nullptr, g_gw ? g_gw->username : nullptr, g_gw ? g_gw->avatar_url : nullptr);
            }
        }

        /* Fallback close detection: process_running check every 5s */
        static time_t last_proc_check = 0;
        if (g_current_game && (now - last_proc_check) >= 5) {
            last_proc_check = now;
            if (g_active_game_pid > 0 && kill(g_active_game_pid, 0) != 0) {
                LOG_INFO("Game process gone (fallback): %s", g_current_game);
                g_pending_clear.store(true);
            }
        }

        // Heartbeat
        if (g_gw && g_gw->heartbeat_interval_ms > 0) {
            int iv = g_gw->heartbeat_interval_ms / 1000;
            if (!g_gw->heartbeat_acked && (now - g_gw->last_heartbeat_sec) > iv + 5) {
                LOG_WARN("Heartbeat zombie, reconnecting");
                goto reconnect;
            }
            if ((now - g_gw->last_heartbeat_sec) >= iv) {
                gateway_send_heartbeat(g_gw);
            }
        }

        // Poll gateway
        if (g_gw) {
            int ret = gateway_poll(g_gw);
            if (ret == -1) { LOG_WARN("Gateway connection lost, reconnecting"); goto reconnect; }
            if (ret == 2) { LOG_WARN("Gateway requested reconnect"); goto reconnect; }
        }

        sleep(1);
        continue;

    reconnect:
        if (g_gw) { gateway_disconnect(g_gw); g_gw = nullptr; }
        config_write_status(false, nullptr, nullptr, nullptr);
        // reconnect loop
        while (g_running && !g_gw) {
            sleep(2);
            token_load();
            g_gw = gateway_connect(g_token);
            if (g_gw) {
                LOG_INFO("Reconnected to gateway");
                config_write_status(true, nullptr, g_gw->username, g_gw->avatar_url);
                if (g_current_game) {
                    const char *gname = gamelist_get_name(g_current_game);
                    const char *gicon = gamelist_get_icon(g_current_game);
                    const char *gappid = gamelist_get_app_id(g_current_game);
                    const char *aid = (gappid && gappid[0]) ? gappid : g_config.application_id;
                    if (gname) gateway_send_presence(g_gw, gname, g_game_start_ms, aid, gicon);
                }
            }
        }
    }

    // Cleanup
    if (g_gw) {
        gateway_send_presence(g_gw, nullptr, 0, g_config.application_id, nullptr);
        gateway_disconnect(g_gw);
    }
    LOG_INFO("rpcdroid daemon stopping");
    config_write_status(false, nullptr, nullptr, nullptr);
    free(g_current_game);
    gamelist_free();
    log_close();
    binder_thread.join();
    return 0;
}
