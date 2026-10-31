#pragma once
#include <stddef.h>

typedef struct ws_ctx ws_ctx_t;

/* Connect to wss://host:port/path. Returns NULL on failure. */
ws_ctx_t *ws_connect(const char *host, int port, const char *path);

/* Send a text frame. Returns 0 on success, -1 on error. */
int ws_send_text(ws_ctx_t *ctx, const char *msg);

/* Receive a text frame.
   timeout_ms=0: non-blocking (returns NULL immediately if nothing).
   timeout_ms>0: block up to timeout_ms milliseconds.
   Returns malloc'd string (caller frees), NULL if nothing/error. */
char *ws_recv_text(ws_ctx_t *ctx, int timeout_ms);

/* Send a close frame and free the context. */
void ws_close(ws_ctx_t *ctx);

/* 1 if connected, 0 if not. */
int ws_is_connected(ws_ctx_t *ctx);

/* Resolve an external image URL to a Discord media proxy URL (mp:external/...) via REST API.
   Returns 0 on success, -1 on failure. */
int discord_resolve_external_asset(const char *app_id, const char *token,
                                   const char *image_url,
                                   char *out_mp, size_t out_len);
