#include "websocket.h"
#include "log.h"
#include "cJSON.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include <errno.h>

#include "mbedtls/net_sockets.h"
#include "mbedtls/ssl.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/error.h"
#include "mbedtls/base64.h"
#include "mbedtls/sha1.h"
#include "mbedtls/x509_crt.h"
#include "psa/crypto.h"

/* Helper: log mbedTLS error with human-readable description */
#define LOG_MBEDTLS_ERR(msg, ret) do { \
    char _errbuf[128]; \
    mbedtls_strerror(ret, _errbuf, sizeof(_errbuf)); \
    LOG_ERROR(msg ": -0x%04X (%s)", -(ret), _errbuf); \
} while(0)

/* GlobalSign Root CA (used by Google Trust Services / Discord) */
static const char ca_crt_pem[] =
"-----BEGIN CERTIFICATE-----\n"
"MIIDdTCCAl2gAwIBAgILBAAAAAABFUtaw5QwDQYJKoZIhvcNAQEFBQAwVzELMAkGA1UEBhMCQkUx\n"
"GTAXBgNVBAoTEEdsb2JhbFNpZ24gbnYtc2ExEDAOBgNVBAsTB1Jvb3QgQ0ExGzAZBgNVBAMTEkds\n"
"b2JhbFNpZ24gUm9vdCBDQTAeFw05ODA5MDExMjAwMDBaFw0yODAxMjgxMjAwMDBaMFcxCzAJBgNV\n"
"BAYTAkJFMRkwFwYDVQQKExBHbG9iYWxTaWduIG52LXNhMRAwDgYDVQQLEwdSb290IENBMRswGQYD\n"
"VQQDExJHbG9iYWxTaWduIFJvb3QgQ0EwggEiMA0GCSqGSIb3DQEBAQUAA4IBDwAwggEKAoIBAQDa\n"
"DuaZjc6j40+Kfvvxi4Mla+pIH/EqsLmVEQS98GPR4mdmzxzdzxtIK+6NiY6arymAZavpxy0Sy6sc\n"
"THAHoT0KMM0VjU/43dSMUBUc71DuxC73/OlS8pF94G3VNTCOXkNz8kHp1Wrjsok6Vjk4bwY8iGlb\n"
"Kk3Fp1S4bInMm/k8yuX9ifUSPJJ4ltbcdG6TRGHRjcdGsnUOhugZitVtbNV4FpWi6cgKOOvyJBNP\n"
"c1STE4U6G7weNLWLBYy5d4ux2x8gkasJU26Qzns3dLlwR5EiUWMWea6xrkEmCMgZK9FGqkjWZCrX\n"
"gzT/LCrBbBlDSgeF59N89iFo7+ryUp9/k5DPAgMBAAGjQjBAMA4GA1UdDwEB/wQEAwIBBjAPBgNV\n"
"HRMBAf8EBTADAQH/MB0GA1UdDgQWBBRge2YaRQ2XyolQL30EzTSo//z9SzANBgkqhkiG9w0BAQUF\n"
"AAOCAQEA1nPnfE920I2/7LqivjTFKDK1fPxsnCwrvQmeU79rXqoRSLblCKOzyj1hTdNGCbM+w6Dj\n"
"Y1Ub8rrvrTnhQ7k4o+YviiY776BQVvnGCv04zcQLcFGUl5gE38NflNUVyRRBnMRddWQVDf9VMOyG\n"
"j/8N7yy5Y0b2qvzfvGn9LhJIZJrglfCm7ymPAbEVtQwdpf5pLGkkeB6zpxxxYu7KyJesF12KwvhH\n"
"hm4qxFYxldBniYUr+WymXUadDKqC5JlR3XC321Y9YeRq4VzW9v493kHMB65jUr9TU/Qr6cf9tveC\n"
"X4XSQRjbgbMEHMUfpIBvFSDJ3gyICh3WZlXi/EjJKSZp4A==\n"
"-----END CERTIFICATE-----\n";

static int custom_vrfy(void *data, mbedtls_x509_crt *crt, int depth, uint32_t *flags)
{
    (void)data;
    (void)crt;
    (void)depth;
    *flags = 0;
    return 0;
}

struct ws_ctx {
    mbedtls_net_context      net;
    mbedtls_ssl_context      ssl;
    mbedtls_ssl_config       conf;
    mbedtls_entropy_context  entropy;
    mbedtls_ctr_drbg_context ctr_drbg;
    mbedtls_x509_crt         cacert;
    int connected;
    /* fragment reassembly */
    char   *frag_buf;
    size_t  frag_len;
    size_t  frag_cap;
};

/* ── helpers ──────────────────────────────────────────────────────────────── */

static inline int is_ssl_retryable(int ret)
{
    return (ret == MBEDTLS_ERR_SSL_WANT_READ ||
            ret == MBEDTLS_ERR_SSL_WANT_WRITE ||
            ret == MBEDTLS_ERR_SSL_ASYNC_IN_PROGRESS ||
#if defined(MBEDTLS_SSL_CRYPTO_IN_PROGRESS)
            ret == MBEDTLS_ERR_SSL_CRYPTO_IN_PROGRESS ||
#endif
#if defined(MBEDTLS_ERR_SSL_RECEIVED_NEW_SESSION_TICKET)
            ret == MBEDTLS_ERR_SSL_RECEIVED_NEW_SESSION_TICKET ||
#endif
            0);
}

static int ssl_write_all(ws_ctx_t *ctx, const unsigned char *buf, size_t len)
{
    size_t written = 0;
    while (written < len) {
        int ret = mbedtls_ssl_write(&ctx->ssl, buf + written, len - written);
        if (ret < 0) {
            if (is_ssl_retryable(ret)) continue;
            LOG_MBEDTLS_ERR("ssl_write_all failed", ret);
            ctx->connected = 0;
            return -1;
        }
        written += (size_t)ret;
    }
    return 0;
}

static int ssl_read_exact(ws_ctx_t *ctx, unsigned char *buf, size_t len)
{
    size_t got = 0;
    while (got < len) {
        int ret = mbedtls_ssl_read(&ctx->ssl, buf + got, len - got);
        if (is_ssl_retryable(ret)) continue;
        if (ret == MBEDTLS_ERR_SSL_TIMEOUT) {
            if (got == 0) {
                return -1;
            }
            LOG_ERROR("SSL read timeout mid-frame (%zu/%zu bytes)", got, len);
            ctx->connected = 0;
            return -1;
        }
        if (ret <= 0) {
            if (ret < 0) {
                LOG_MBEDTLS_ERR("ssl_read_exact failed", ret);
            }
            ctx->connected = 0;
            return -1;
        }
        got += (size_t)ret;
    }
    return 0;
}

/* Append bytes to the fragment buffer, growing as needed. */
static int frag_append(ws_ctx_t *ctx, const char *data, size_t len)
{
    if (ctx->frag_len + len + 1 > ctx->frag_cap) {
        size_t newcap = ctx->frag_cap ? ctx->frag_cap * 2 : 4096;
        while (newcap < ctx->frag_len + len + 1) newcap *= 2;
        char *nb = realloc(ctx->frag_buf, newcap);
        if (!nb) return -1;
        ctx->frag_buf = nb;
        ctx->frag_cap = newcap;
    }
    memcpy(ctx->frag_buf + ctx->frag_len, data, len);
    ctx->frag_len += len;
    ctx->frag_buf[ctx->frag_len] = '\0';
    return 0;
}

/* Send a raw frame (unmasked or masked). Used internally. */
static int ws_send_frame(ws_ctx_t *ctx, uint8_t opcode, int do_mask,
                         const unsigned char *payload, size_t plen)
{
    /* Header: up to 2 + 8 + 4 = 14 bytes */
    unsigned char hdr[14];
    size_t hlen = 0;

    hdr[hlen++] = 0x80 | (opcode & 0x0F); /* FIN=1 */

    uint8_t mask_bit = do_mask ? 0x80 : 0x00;
    if (plen < 126) {
        hdr[hlen++] = mask_bit | (uint8_t)plen;
    } else if (plen < 65536) {
        hdr[hlen++] = mask_bit | 126;
        hdr[hlen++] = (uint8_t)(plen >> 8);
        hdr[hlen++] = (uint8_t)(plen & 0xFF);
    } else {
        hdr[hlen++] = mask_bit | 127;
        for (int i = 7; i >= 0; i--)
            hdr[hlen++] = (uint8_t)((plen >> (i * 8)) & 0xFF);
    }

    unsigned char mask_key[4] = {0};
    if (do_mask) {
        size_t olen = 0;
        mbedtls_ctr_drbg_random(&ctx->ctr_drbg, mask_key, 4);
        (void)olen;
        memcpy(hdr + hlen, mask_key, 4);
        hlen += 4;
    }

    if (ssl_write_all(ctx, hdr, hlen) != 0) return -1;

    if (plen == 0) return 0;

    if (!do_mask) {
        return ssl_write_all(ctx, payload, plen);
    }

    /* Masked: XOR in chunks to avoid a full copy for large payloads */
    unsigned char chunk[1024];
    size_t sent = 0;
    while (sent < plen) {
        size_t n = plen - sent;
        if (n > sizeof(chunk)) n = sizeof(chunk);
        for (size_t i = 0; i < n; i++)
            chunk[i] = payload[sent + i] ^ mask_key[(sent + i) % 4];
        if (ssl_write_all(ctx, chunk, n) != 0) return -1;
        sent += n;
    }
    return 0;
}

/* ── ws_connect ───────────────────────────────────────────────────────────── */

ws_ctx_t *ws_connect(const char *host, int port, const char *path)
{
    psa_status_t psa_status = psa_crypto_init();
    if (psa_status != PSA_SUCCESS) {
        LOG_ERROR("psa_crypto_init failed: %d", (int)psa_status);
        return NULL;
    }

    ws_ctx_t *ctx = calloc(1, sizeof(*ctx));
    if (!ctx) return NULL;

    mbedtls_net_init(&ctx->net);
    mbedtls_ssl_init(&ctx->ssl);
    mbedtls_ssl_config_init(&ctx->conf);
    mbedtls_entropy_init(&ctx->entropy);
    mbedtls_ctr_drbg_init(&ctx->ctr_drbg);

    int ret;
    const char *pers = "rpcdroid";

    ret = mbedtls_ctr_drbg_seed(&ctx->ctr_drbg, mbedtls_entropy_func,
                                 &ctx->entropy,
                                 (const unsigned char *)pers, strlen(pers));
    if (ret != 0) {
        LOG_MBEDTLS_ERR("ctr_drbg_seed failed", ret);
        goto fail;
    }

    char port_str[8];
    snprintf(port_str, sizeof(port_str), "%d", port);
    ret = mbedtls_net_connect(&ctx->net, host, port_str, MBEDTLS_NET_PROTO_TCP);
    if (ret != 0) {
        LOG_MBEDTLS_ERR("net_connect failed", ret);
        goto fail;
    }

    ret = mbedtls_ssl_config_defaults(&ctx->conf,
                                      MBEDTLS_SSL_IS_CLIENT,
                                      MBEDTLS_SSL_TRANSPORT_STREAM,
                                      MBEDTLS_SSL_PRESET_DEFAULT);
    if (ret != 0) {
        LOG_MBEDTLS_ERR("ssl_config_defaults failed", ret);
        goto fail;
    }

    mbedtls_x509_crt_init(&ctx->cacert);
    ret = mbedtls_x509_crt_parse(&ctx->cacert, (const unsigned char *)ca_crt_pem, sizeof(ca_crt_pem));
    if (ret < 0) {
        LOG_MBEDTLS_ERR("x509_crt_parse failed", ret);
        goto fail;
    }

    mbedtls_ssl_conf_ca_chain(&ctx->conf, &ctx->cacert, NULL);
    mbedtls_ssl_conf_verify(&ctx->conf, custom_vrfy, NULL);
    mbedtls_ssl_conf_authmode(&ctx->conf, MBEDTLS_SSL_VERIFY_OPTIONAL);
    mbedtls_ssl_conf_rng(&ctx->conf, mbedtls_ctr_drbg_random, &ctx->ctr_drbg);

    ret = mbedtls_ssl_setup(&ctx->ssl, &ctx->conf);
    if (ret != 0) {
        LOG_MBEDTLS_ERR("ssl_setup failed", ret);
        goto fail;
    }

    ret = mbedtls_ssl_set_hostname(&ctx->ssl, host);
    if (ret != 0) {
        LOG_MBEDTLS_ERR("ssl_set_hostname failed", ret);
        goto fail;
    }

    mbedtls_ssl_set_bio(&ctx->ssl, &ctx->net,
                        mbedtls_net_send, mbedtls_net_recv, NULL);

    /* TLS handshake */
    LOG_DEBUG("Starting TLS handshake to %s:%d", host, port);
    do {
        ret = mbedtls_ssl_handshake(&ctx->ssl);
    } while (ret == MBEDTLS_ERR_SSL_WANT_READ || ret == MBEDTLS_ERR_SSL_WANT_WRITE);
    if (ret != 0) {
        LOG_MBEDTLS_ERR("ssl_handshake failed", ret);
        /* Log TLS version negotiated (may be 0 if handshake failed early) */
        LOG_ERROR("TLS version: 0x%04X, ciphersuite: %s",
                  mbedtls_ssl_get_version_number(&ctx->ssl),
                  mbedtls_ssl_get_ciphersuite(&ctx->ssl));
        goto fail;
    }
    LOG_INFO("TLS handshake OK: %s / %s",
             mbedtls_ssl_get_version(&ctx->ssl),
             mbedtls_ssl_get_ciphersuite(&ctx->ssl));

    /* Generate 16-byte nonce → base64 → Sec-WebSocket-Key */
    unsigned char nonce[16];
    mbedtls_ctr_drbg_random(&ctx->ctr_drbg, nonce, sizeof(nonce));

    unsigned char ws_key_b64[32];
    size_t ws_key_len = 0;
    mbedtls_base64_encode(ws_key_b64, sizeof(ws_key_b64), &ws_key_len,
                          nonce, sizeof(nonce));
    ws_key_b64[ws_key_len] = '\0';

    /* HTTP Upgrade request */
    char req[1024];
    int reqlen = snprintf(req, sizeof(req),
        "GET %s HTTP/1.1\r\n"
        "Host: %s\r\n"
        "Upgrade: websocket\r\n"
        "Connection: Upgrade\r\n"
        "Sec-WebSocket-Key: %s\r\n"
        "Sec-WebSocket-Version: 13\r\n"
        "User-Agent: Mozilla/5.0\r\n"
        "Origin: https://discord.com\r\n"
        "\r\n",
        path, host, (char *)ws_key_b64);
    if (reqlen < 0 || (size_t)reqlen >= sizeof(req)) goto fail;

    if (ssl_write_all(ctx, (unsigned char *)req, (size_t)reqlen) != 0) {
        LOG_ERROR("Failed to send HTTP upgrade request");
        goto fail;
    }

    /* Read HTTP response until \r\n\r\n */
    char resp[2048];
    size_t rlen = 0;
    int found_end = 0;
    while (rlen < sizeof(resp) - 1) {
        int r = mbedtls_ssl_read(&ctx->ssl,
                                 (unsigned char *)resp + rlen, 1);
        if (is_ssl_retryable(r)) {
            continue;
        }
        if (r < 0) {
            LOG_MBEDTLS_ERR("Error reading HTTP response", r);
            goto fail;
        }
        if (r == 0) break;
        rlen++;
        if (rlen >= 4 &&
            resp[rlen-4] == '\r' && resp[rlen-3] == '\n' &&
            resp[rlen-2] == '\r' && resp[rlen-1] == '\n') {
            found_end = 1;
            break;
        }
    }
    resp[rlen] = '\0';

    if (!found_end || strstr(resp, "101") == NULL) {
        LOG_ERROR("WebSocket upgrade failed: %.*s", (int)(rlen > 128 ? 128 : rlen), resp);
        goto fail;
    }

    ctx->connected = 1;
    LOG_INFO("WebSocket connected to %s:%d%s", host, port, path);
    return ctx;

fail:
    mbedtls_x509_crt_free(&ctx->cacert);
    mbedtls_ssl_free(&ctx->ssl);
    mbedtls_ssl_config_free(&ctx->conf);
    mbedtls_ctr_drbg_free(&ctx->ctr_drbg);
    mbedtls_entropy_free(&ctx->entropy);
    mbedtls_net_free(&ctx->net);
    free(ctx->frag_buf);
    free(ctx);
    return NULL;
}

/* ── ws_send_text ─────────────────────────────────────────────────────────── */

int ws_send_text(ws_ctx_t *ctx, const char *msg)
{
    if (!ctx || !ctx->connected) return -1;
    return ws_send_frame(ctx, 0x1, 1,
                         (const unsigned char *)msg, strlen(msg));
}

/* ── ws_recv_text ─────────────────────────────────────────────────────────── */

/* Internal: read one complete frame, handle control frames inline.
   Returns malloc'd payload string for data frames, NULL for control/error. */
static char *ws_recv_frame(ws_ctx_t *ctx, int first_byte_given,
                            unsigned char first_byte)
{
again:;
    unsigned char hdr[2];

    if (first_byte_given) {
        hdr[0] = first_byte;
        first_byte_given = 0;
        if (ssl_read_exact(ctx, hdr + 1, 1) != 0) return NULL;
    } else {
        if (ssl_read_exact(ctx, hdr, 2) != 0) return NULL;
    }

    int fin    = (hdr[0] & 0x80) != 0;
    int opcode = (hdr[0] & 0x0F);
    int masked = (hdr[1] & 0x80) != 0;
    uint64_t plen = hdr[1] & 0x7F;

    if (plen == 126) {
        unsigned char ext[2];
        if (ssl_read_exact(ctx, ext, 2) != 0) return NULL;
        plen = ((uint64_t)ext[0] << 8) | ext[1];
    } else if (plen == 127) {
        unsigned char ext[8];
        if (ssl_read_exact(ctx, ext, 8) != 0) return NULL;
        plen = 0;
        for (int i = 0; i < 8; i++)
            plen = (plen << 8) | ext[i];
    }

    unsigned char mask_key[4] = {0};
    if (masked) {
        if (ssl_read_exact(ctx, mask_key, 4) != 0) return NULL;
    }

    /* Read payload */
    char *payload = NULL;
    if (plen > 0) {
        if (plen > 64 * 1024 * 1024) {
            /* ponytail: limit 64 MiB; stream large payloads when needed */
            LOG_WARN("WebSocket frame too large: %llu", (unsigned long long)plen);
            ctx->connected = 0;
            return NULL;
        }
        payload = malloc(plen + 1);
        if (!payload) { ctx->connected = 0; return NULL; }
        if (ssl_read_exact(ctx, (unsigned char *)payload, (size_t)plen) != 0) {
            free(payload);
            return NULL;
        }
        if (masked) {
            for (uint64_t i = 0; i < plen; i++)
                payload[i] ^= mask_key[i % 4];
        }
        payload[plen] = '\0';
    } else {
        payload = calloc(1, 1);
    }

    /* Handle control frames */
    if (opcode == 0x8) { /* Close */
        ctx->connected = 0;
        free(payload);
        return NULL;
    }
    if (opcode == 0x9) { /* Ping → Pong */
        ws_send_frame(ctx, 0xA, 1, (unsigned char *)payload, (size_t)plen);
        free(payload);
        goto again;
    }
    if (opcode == 0xA) { /* Pong — ignore */
        free(payload);
        goto again;
    }

    /* Data frames: text (0x1) and continuation (0x0) */
    if (opcode == 0x1 || opcode == 0x0) {
        if (fin && ctx->frag_len == 0) {
            /* Fast path: no fragmentation */
            return payload;
        }
        /* Fragment */
        if (frag_append(ctx, payload, (size_t)plen) != 0) {
            free(payload);
            ctx->connected = 0;
            return NULL;
        }
        free(payload);

        if (fin) {
            char *result = ctx->frag_buf;
            ctx->frag_buf = NULL;
            ctx->frag_len = 0;
            ctx->frag_cap = 0;
            return result;
        }
        goto again;
    }

    /* Unknown opcode */
    free(payload);
    return NULL;
}

char *ws_recv_text(ws_ctx_t *ctx, int timeout_ms)
{
    if (!ctx || !ctx->connected) return NULL;

    if (timeout_ms == 0) {
        /* Non-blocking: peek first byte */
        mbedtls_net_set_nonblock(&ctx->net);
        unsigned char b;
        int ret;
        do {
            ret = mbedtls_ssl_read(&ctx->ssl, &b, 1);
        } while (is_ssl_retryable(ret) &&
                 ret != MBEDTLS_ERR_SSL_WANT_READ &&
                 ret != MBEDTLS_ERR_SSL_WANT_WRITE);
        mbedtls_net_set_block(&ctx->net);
        if (ret == MBEDTLS_ERR_SSL_WANT_READ ||
            ret == MBEDTLS_ERR_SSL_WANT_WRITE ||
            ret == MBEDTLS_ERR_SSL_TIMEOUT) return NULL;
        if (ret <= 0) {
            if (ret < 0) {
                LOG_MBEDTLS_ERR("ws_recv_text non-blocking read failed", ret);
            }
            ctx->connected = 0;
            return NULL;
        }
        return ws_recv_frame(ctx, 1, b);
    }

    /* Blocking with timeout via mbedtls_net_set_block + bio timeout */
    mbedtls_ssl_conf_read_timeout(&ctx->conf, (uint32_t)timeout_ms);
    mbedtls_ssl_set_bio(&ctx->ssl, &ctx->net,
                        mbedtls_net_send, mbedtls_net_recv,
                        mbedtls_net_recv_timeout);

    char *result = ws_recv_frame(ctx, 0, 0);

    /* Restore no-timeout bio */
    mbedtls_ssl_conf_read_timeout(&ctx->conf, 0);
    mbedtls_ssl_set_bio(&ctx->ssl, &ctx->net,
                        mbedtls_net_send, mbedtls_net_recv, NULL);
    return result;
}

/* ── ws_close ─────────────────────────────────────────────────────────────── */

void ws_close(ws_ctx_t *ctx)
{
    if (!ctx) return;
    if (ctx->connected) {
        /* Close frame: status 1000 Normal Closure */
        unsigned char close_payload[2] = {0x03, 0xE8};
        ws_send_frame(ctx, 0x8, 1, close_payload, sizeof(close_payload));
        mbedtls_ssl_close_notify(&ctx->ssl);
        ctx->connected = 0;
    }
    mbedtls_x509_crt_free(&ctx->cacert);
    mbedtls_ssl_free(&ctx->ssl);
    mbedtls_ssl_config_free(&ctx->conf);
    mbedtls_ctr_drbg_free(&ctx->ctr_drbg);
    mbedtls_entropy_free(&ctx->entropy);
    mbedtls_net_free(&ctx->net);
    free(ctx->frag_buf);
    free(ctx);
}

/* ── ws_is_connected ──────────────────────────────────────────────────────── */

int ws_is_connected(ws_ctx_t *ctx)
{
    return ctx ? ctx->connected : 0;
}

/* ── discord_resolve_external_asset ───────────────────────────────────────── */

int discord_resolve_external_asset(const char *app_id, const char *token,
                                   const char *image_url,
                                   char *out_mp, size_t out_len)
{
    if (!app_id || !token || !image_url || !out_mp || out_len == 0)
        return -1;

    psa_crypto_init();

    mbedtls_net_context      net;
    mbedtls_ssl_context      ssl;
    mbedtls_ssl_config       conf;
    mbedtls_entropy_context  entropy;
    mbedtls_ctr_drbg_context ctr_drbg;
    mbedtls_x509_crt         cacert;

    mbedtls_net_init(&net);
    mbedtls_ssl_init(&ssl);
    mbedtls_ssl_config_init(&conf);
    mbedtls_entropy_init(&entropy);
    mbedtls_ctr_drbg_init(&ctr_drbg);
    mbedtls_x509_crt_init(&cacert);

    int ret;
    const char *pers = "rpcdroid-http";
    ret = mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy,
                                 (const unsigned char *)pers, strlen(pers));
    if (ret != 0) goto cleanup;

    ret = mbedtls_net_connect(&net, "discord.com", "443", MBEDTLS_NET_PROTO_TCP);
    if (ret != 0) {
        LOG_MBEDTLS_ERR("net_connect to discord.com failed", ret);
        goto cleanup;
    }

    ret = mbedtls_ssl_config_defaults(&conf, MBEDTLS_SSL_IS_CLIENT,
                                      MBEDTLS_SSL_TRANSPORT_STREAM,
                                      MBEDTLS_SSL_PRESET_DEFAULT);
    if (ret != 0) goto cleanup;

    ret = mbedtls_x509_crt_parse(&cacert, (const unsigned char *)ca_crt_pem, sizeof(ca_crt_pem));
    if (ret < 0) goto cleanup;

    mbedtls_ssl_conf_ca_chain(&conf, &cacert, NULL);
    mbedtls_ssl_conf_verify(&conf, custom_vrfy, NULL);
    mbedtls_ssl_conf_authmode(&conf, MBEDTLS_SSL_VERIFY_OPTIONAL);
    mbedtls_ssl_conf_rng(&conf, mbedtls_ctr_drbg_random, &ctr_drbg);
    mbedtls_ssl_conf_read_timeout(&conf, 10000); /* 10s timeout */

    ret = mbedtls_ssl_setup(&ssl, &conf);
    if (ret != 0) goto cleanup;

    mbedtls_ssl_set_hostname(&ssl, "discord.com");
    mbedtls_ssl_set_bio(&ssl, &net, mbedtls_net_send, mbedtls_net_recv, mbedtls_net_recv_timeout);

    do {
        ret = mbedtls_ssl_handshake(&ssl);
    } while (ret == MBEDTLS_ERR_SSL_WANT_READ || ret == MBEDTLS_ERR_SSL_WANT_WRITE);
    if (ret != 0) {
        LOG_MBEDTLS_ERR("ssl_handshake to discord.com failed", ret);
        goto cleanup;
    }

    /* Build JSON body: {"urls":["<image_url>"]} */
    cJSON *req_root = cJSON_CreateObject();
    cJSON *urls_arr = cJSON_CreateArray();
    cJSON_AddItemToArray(urls_arr, cJSON_CreateString(image_url));
    cJSON_AddItemToObject(req_root, "urls", urls_arr);
    char *body_str = cJSON_PrintUnformatted(req_root);
    cJSON_Delete(req_root);
    if (!body_str) goto cleanup;

    size_t body_len = strlen(body_str);

    /* Build HTTP POST */
    char req[2048];
    int reqlen = snprintf(req, sizeof(req),
        "POST /api/v10/applications/%s/external-assets HTTP/1.1\r\n"
        "Host: discord.com\r\n"
        "Authorization: %s\r\n"
        "Content-Type: application/json\r\n"
        "Content-Length: %zu\r\n"
        "User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64)\r\n"
        "Connection: close\r\n"
        "\r\n"
        "%s",
        app_id, token, body_len, body_str);
    free(body_str);

    if (reqlen < 0 || (size_t)reqlen >= sizeof(req)) goto cleanup;

    /* Write request */
    size_t written = 0;
    while (written < (size_t)reqlen) {
        ret = mbedtls_ssl_write(&ssl, (const unsigned char *)req + written, (size_t)reqlen - written);
        if (ret < 0) {
            if (is_ssl_retryable(ret)) continue;
            LOG_MBEDTLS_ERR("ssl_write for external-assets failed", ret);
            goto cleanup;
        }
        written += (size_t)ret;
    }

    /* Read response */
    char resp[8192];
    size_t resp_len = 0;
    while (resp_len < sizeof(resp) - 1) {
        ret = mbedtls_ssl_read(&ssl, (unsigned char *)resp + resp_len, sizeof(resp) - 1 - resp_len);
        if (ret < 0) {
            if (is_ssl_retryable(ret)) continue;
            break; /* EOF or error */
        }
        if (ret == 0) break;
        resp_len += (size_t)ret;
    }
    resp[resp_len] = '\0';

    char *hdr_end = strstr(resp, "\r\n\r\n");
    if (!hdr_end) hdr_end = strstr(resp, "\n\n");
    if (!hdr_end) {
        LOG_WARN("discord_resolve_external_asset: Invalid HTTP response (no header end)");
        goto cleanup;
    }

    int status_code = 0;
    if (sscanf(resp, "HTTP/%*s %d", &status_code) == 1) {
        if (status_code != 200) {
            LOG_WARN("discord_resolve_external_asset: HTTP %d response: %s", status_code, hdr_end);
            goto cleanup;
        }
    }

    char *json_start = strpbrk(hdr_end, "[{");
    if (!json_start) {
        LOG_WARN("discord_resolve_external_asset: No JSON found in response");
        goto cleanup;
    }

    cJSON *resp_json = cJSON_Parse(json_start);
    if (!resp_json) {
        LOG_WARN("discord_resolve_external_asset: Failed to parse JSON: %s", json_start);
        goto cleanup;
    }

    int success = -1;
    if (cJSON_IsArray(resp_json)) {
        cJSON *first = cJSON_GetArrayItem(resp_json, 0);
        if (first) {
            cJSON *path_item = cJSON_GetObjectItem(first, "external_asset_path");
            if (path_item && path_item->valuestring && path_item->valuestring[0]) {
                const char *p = path_item->valuestring;
                if (strncmp(p, "mp:", 3) == 0) {
                    snprintf(out_mp, out_len, "%s", p);
                } else {
                    snprintf(out_mp, out_len, "mp:%s", p);
                }
                success = 0;
            }
        }
    } else {
        cJSON *msg = cJSON_GetObjectItem(resp_json, "message");
        LOG_WARN("discord_resolve_external_asset: Discord error: %s",
                 msg && msg->valuestring ? msg->valuestring : json_start);
    }
    cJSON_Delete(resp_json);

    mbedtls_ssl_close_notify(&ssl);
    mbedtls_net_free(&net);
    mbedtls_ssl_free(&ssl);
    mbedtls_ssl_config_free(&conf);
    mbedtls_x509_crt_free(&cacert);
    mbedtls_ctr_drbg_free(&ctr_drbg);
    mbedtls_entropy_free(&entropy);
    return success;

cleanup:
    mbedtls_net_free(&net);
    mbedtls_ssl_free(&ssl);
    mbedtls_ssl_config_free(&conf);
    mbedtls_x509_crt_free(&cacert);
    mbedtls_ctr_drbg_free(&ctr_drbg);
    mbedtls_entropy_free(&entropy);
    return -1;
}
