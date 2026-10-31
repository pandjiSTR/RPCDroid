#include "gateway.h"
#include "cJSON.h"
#include "log.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

/* ── helpers ──────────────────────────────────────────────────────────────── */

static int gw_send_json(gateway_t *gw, cJSON *obj)
{
    char *s = cJSON_PrintUnformatted(obj);
    if (!s) return -1;
    int ret = ws_send_text(gw->ws, s);
    free(s);
    return ret;
}

/* Update gw->username from a user object (d.user in READY). */
static void parse_user(gateway_t *gw, cJSON *user)
{
    if (!user) return;
    cJSON *uname = cJSON_GetObjectItemCaseSensitive(user, "username");
    cJSON *discrim = cJSON_GetObjectItemCaseSensitive(user, "discriminator");
    if (!cJSON_IsString(uname)) return;

    const char *d = cJSON_IsString(discrim) ? discrim->valuestring : "0";
    if (strcmp(d, "0") == 0 || strcmp(d, "") == 0) {
        snprintf(gw->username, sizeof(gw->username), "%s", uname->valuestring);
    } else {
        snprintf(gw->username, sizeof(gw->username), "%s#%s",
                 uname->valuestring, d);
    }
}

/* ── gateway_connect ──────────────────────────────────────────────────────── */

gateway_t *gateway_connect(const char *token)
{
    ws_ctx_t *ws = ws_connect("gateway.discord.gg", 443, "/?v=10&encoding=json");
    if (!ws) return NULL;

    gateway_t *gw = calloc(1, sizeof(*gw));
    if (!gw) { ws_close(ws); return NULL; }
    gw->ws = ws;
    gw->heartbeat_acked = true;

    /* ── Wait for op 10 HELLO ─────────────────────────────────────────────── */
    for (int tries = 0; tries < 20; tries++) {
        char *msg = ws_recv_text(gw->ws, 5000);
        if (!msg) {
            if (!ws_is_connected(gw->ws)) {
                LOG_ERROR("Gateway disconnected waiting for HELLO");
                goto fail;
            }
            continue;
        }

        cJSON *root = cJSON_Parse(msg);
        free(msg);
        if (!root) continue;

        cJSON *op_j = cJSON_GetObjectItemCaseSensitive(root, "op");
        int op = cJSON_IsNumber(op_j) ? (int)op_j->valuedouble : -1;

        if (op == 10) {
            cJSON *d = cJSON_GetObjectItemCaseSensitive(root, "d");
            cJSON *hb = d ? cJSON_GetObjectItemCaseSensitive(d, "heartbeat_interval") : NULL;
            if (cJSON_IsNumber(hb))
                gw->heartbeat_interval_ms = (int)hb->valuedouble;
            cJSON_Delete(root);
            LOG_INFO("Gateway HELLO: heartbeat_interval=%d ms", gw->heartbeat_interval_ms);
            break;
        }
        cJSON_Delete(root);
    }

    if (gw->heartbeat_interval_ms == 0) {
        LOG_ERROR("Did not receive HELLO from gateway");
        goto fail;
    }

    /* ── Send IDENTIFY (op 2) ─────────────────────────────────────────────── */
    {
        cJSON *root = cJSON_CreateObject();
        cJSON_AddNumberToObject(root, "op", 2);

        cJSON *d = cJSON_CreateObject();
        cJSON_AddStringToObject(d, "token", token);

        cJSON *props = cJSON_CreateObject();
        cJSON_AddStringToObject(props, "os",      "Windows");
        cJSON_AddStringToObject(props, "browser", "Discord Client");
        cJSON_AddStringToObject(props, "device",  "discord-rpc");
        cJSON_AddItemToObject(d, "properties", props);

        cJSON_AddNumberToObject(d, "capabilities", 65);
        cJSON_AddFalseToObject(d,  "compress");
        cJSON_AddItemToObject(root, "d", d);

        int ret = gw_send_json(gw, root);
        cJSON_Delete(root);
        if (ret != 0) {
            LOG_ERROR("Failed to send IDENTIFY");
            goto fail;
        }
        LOG_INFO("Sent IDENTIFY");
    }

    /* ── Wait for op 0 READY ─────────────────────────────────────────────── */
    for (int tries = 0; tries < 30; tries++) {
        char *msg = ws_recv_text(gw->ws, 5000);
        if (!msg) {
            if (!ws_is_connected(gw->ws)) {
                LOG_ERROR("Gateway disconnected waiting for READY");
                goto fail;
            }
            continue;
        }

        cJSON *root = cJSON_Parse(msg);
        free(msg);
        if (!root) continue;

        cJSON *op_j = cJSON_GetObjectItemCaseSensitive(root, "op");
        cJSON *t_j  = cJSON_GetObjectItemCaseSensitive(root, "t");
        int op = cJSON_IsNumber(op_j) ? (int)op_j->valuedouble : -1;

        /* Handle op 11 ACK that may arrive before READY */
        if (op == 11) { gw->heartbeat_acked = true; cJSON_Delete(root); continue; }
        /* Handle op 1 server heartbeat request */
        if (op == 1)  { gateway_send_heartbeat(gw);  cJSON_Delete(root); continue; }

        if (op == 9) {
            LOG_WARN("IDENTIFY rejected (Invalid Session)");
            cJSON_Delete(root);
            goto fail;
        }

        if (op == 0 && cJSON_IsString(t_j) && strcmp(t_j->valuestring, "READY") == 0) {
            cJSON *d = cJSON_GetObjectItemCaseSensitive(root, "d");
            if (d) {
                cJSON *sid = cJSON_GetObjectItemCaseSensitive(d, "session_id");
                if (cJSON_IsString(sid))
                    snprintf(gw->session_id, sizeof(gw->session_id), "%s", sid->valuestring);

                cJSON *rurl = cJSON_GetObjectItemCaseSensitive(d, "resume_gateway_url");
                if (cJSON_IsString(rurl))
                    snprintf(gw->resume_url, sizeof(gw->resume_url), "%s", rurl->valuestring);

                cJSON *user = cJSON_GetObjectItemCaseSensitive(d, "user");
                parse_user(gw, user);
            }
            cJSON_Delete(root);
            gw->ready = true;
            gw->last_heartbeat_sec = time(NULL);
            LOG_INFO("Gateway READY — logged in as %s", gw->username);
            return gw;
        }

        cJSON_Delete(root);
    }

    LOG_ERROR("Did not receive READY from gateway");

fail:
    ws_close(gw->ws);
    free(gw);
    return NULL;
}

/* ── gateway_disconnect ───────────────────────────────────────────────────── */

void gateway_disconnect(gateway_t *gw)
{
    if (!gw) return;
    ws_close(gw->ws);
    free(gw);
}

/* ── gateway_send_heartbeat ───────────────────────────────────────────────── */

int gateway_send_heartbeat(gateway_t *gw)
{
    if (!gw) return -1;

    cJSON *root = cJSON_CreateObject();
    cJSON_AddNumberToObject(root, "op", 1);
    if (gw->sequence > 0)
        cJSON_AddNumberToObject(root, "d", gw->sequence);
    else
        cJSON_AddNullToObject(root, "d");

    int ret = gw_send_json(gw, root);
    cJSON_Delete(root);

    if (ret == 0) {
        gw->last_heartbeat_sec = time(NULL);
        gw->heartbeat_acked = false;
        LOG_DEBUG("Sent heartbeat (seq=%d)", gw->sequence);
    }
    return ret;
}

/* ── gateway_send_presence ────────────────────────────────────────────────── */

int gateway_send_presence(gateway_t *gw,
                           const char *game_name,
                           long long   start_time_ms,
                           const char *app_id,
                           const char *status,
                           const char *icon_url)
{
    if (!gw) return -1;

    cJSON *root = cJSON_CreateObject();
    cJSON_AddNumberToObject(root, "op", 3);

    cJSON *d = cJSON_CreateObject();

    cJSON *activities = cJSON_CreateArray();
    if (game_name) {
        cJSON *act = cJSON_CreateObject();
        cJSON_AddStringToObject(act, "name", game_name);
        cJSON_AddNumberToObject(act, "type", 0);

        cJSON *ts = cJSON_CreateObject();
        cJSON_AddNumberToObject(ts, "start", (double)start_time_ms);
        cJSON_AddItemToObject(act, "timestamps", ts);

        if (app_id && app_id[0])
            cJSON_AddStringToObject(act, "application_id", app_id);

        if (icon_url && icon_url[0]) {
            /* Only send assets.large_image if it's a valid media proxy (mp:) or registered asset key.
             * Any raw http(s):// URL will cause Discord client to render a missing asset question mark (?). */
            bool is_allowed = false;
            if (strncmp(icon_url, "mp:", 3) == 0) {
                is_allowed = true;
            } else if (strncmp(icon_url, "http://", 7) != 0 && strncmp(icon_url, "https://", 8) != 0) {
                is_allowed = true;
            }

            if (is_allowed) {
                cJSON *assets = cJSON_CreateObject();
                cJSON_AddStringToObject(assets, "large_image", icon_url);
                cJSON_AddStringToObject(assets, "large_text", game_name);
                cJSON_AddItemToObject(act, "assets", assets);
            }
        }

        cJSON_AddItemToArray(activities, act);

        cJSON_AddNumberToObject(d, "since", (double)start_time_ms);
    } else {
        cJSON_AddNumberToObject(d, "since", 0);
    }

    cJSON_AddItemToObject(d, "activities", activities);
    cJSON_AddStringToObject(d, "status", status ? status : "online");
    cJSON_AddFalseToObject(d, "afk");

    cJSON_AddItemToObject(root, "d", d);

    int ret = gw_send_json(gw, root);
    cJSON_Delete(root);

    if (ret == 0) {
        if (game_name)
            LOG_INFO("Presence updated: %s", game_name);
        else
            LOG_INFO("Presence cleared");
    }
    return ret;
}

/* ── gateway_poll ─────────────────────────────────────────────────────────── */

int gateway_poll(gateway_t *gw)
{
    if (!gw) return -1;
    if (!ws_is_connected(gw->ws)) return -1;

    char *msg = ws_recv_text(gw->ws, 0);
    if (!msg) {
        if (!ws_is_connected(gw->ws)) return -1;
        return 0;
    }

    cJSON *root = cJSON_Parse(msg);
    free(msg);
    if (!root) return 1;

    cJSON *op_j = cJSON_GetObjectItemCaseSensitive(root, "op");
    cJSON *d_j  = cJSON_GetObjectItemCaseSensitive(root, "d");
    cJSON *s_j  = cJSON_GetObjectItemCaseSensitive(root, "s");
    cJSON *t_j  = cJSON_GetObjectItemCaseSensitive(root, "t");

    int op = cJSON_IsNumber(op_j) ? (int)op_j->valuedouble : -1;

    if (cJSON_IsNumber(s_j) && (int)s_j->valuedouble > 0)
        gw->sequence = (int)s_j->valuedouble;

    int result = 1;

    switch (op) {
    case 1: /* Heartbeat request */
        gateway_send_heartbeat(gw);
        break;

    case 7: /* Reconnect */
        LOG_INFO("Gateway requested reconnect");
        result = 2;
        break;

    case 9: /* Invalid Session */
        LOG_WARN("Invalid session");
        gw->session_id[0] = '\0';
        result = 2;
        break;

    case 10: /* Hello (re-HELLO after resume) */
        if (cJSON_IsObject(d_j)) {
            cJSON *hb = cJSON_GetObjectItemCaseSensitive(d_j, "heartbeat_interval");
            if (cJSON_IsNumber(hb))
                gw->heartbeat_interval_ms = (int)hb->valuedouble;
        }
        break;

    case 11: /* Heartbeat ACK */
        gw->heartbeat_acked = true;
        LOG_DEBUG("Heartbeat ACK");
        break;

    case 0: /* Dispatch */
        if (cJSON_IsString(t_j) && strcmp(t_j->valuestring, "READY") == 0) {
            cJSON *user = d_j ? cJSON_GetObjectItemCaseSensitive(d_j, "user") : NULL;
            parse_user(gw, user);
            LOG_INFO("READY dispatch: %s", gw->username);
        }
        break;

    default:
        LOG_DEBUG("Unhandled gateway op %d", op);
        break;
    }

    cJSON_Delete(root);
    return result;
}
