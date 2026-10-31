#pragma once
#include "websocket.h"
#include <stdbool.h>
#include <time.h>

typedef struct {
    ws_ctx_t *ws;
    int       heartbeat_interval_ms;
    time_t    last_heartbeat_sec;
    bool      heartbeat_acked;
    int       sequence;
    char      session_id[256];
    char      resume_url[512];
    char      username[128];
    bool      ready;
} gateway_t;

/* Connect, IDENTIFY, wait for READY. Returns NULL on failure. */
gateway_t *gateway_connect(const char *token);

/* Graceful disconnect. */
void gateway_disconnect(gateway_t *gw);

/* Send op 1 heartbeat. */
int gateway_send_heartbeat(gateway_t *gw);

/* Send op 3 PRESENCE_UPDATE.
   game_name=NULL clears presence.
   start_time_ms: epoch ms when game started (used in timestamps.start).
   app_id: Discord application ID string.
   status: "online", "idle", or "dnd".
   icon_url: optional external image URL or asset key for large_image. */
int gateway_send_presence(gateway_t *gw,
                          const char *game_name,
                          long long   start_time_ms,
                          const char *app_id,
                          const char *status,
                          const char *icon_url);

/* Poll for incoming events (non-blocking).
   Returns:  0 = nothing received
             1 = event processed OK
            -1 = connection error (caller must reconnect)
             2 = RECONNECT or INVALID_SESSION (caller must reconnect) */
int gateway_poll(gateway_t *gw);
