LOCAL_PATH := $(call my-dir)

# ── mbedTLS static library ──────────────────────────────────────────────────
include $(CLEAR_VARS)
LOCAL_MODULE        := mbedtls_static
MBEDTLS_DIR         := $(LOCAL_PATH)/mbedtls
LOCAL_SRC_FILES     := $(wildcard $(MBEDTLS_DIR)/library/*.c) \
                       $(LOCAL_PATH)/entropy_hardware_alt.c
LOCAL_C_INCLUDES    := $(MBEDTLS_DIR)/include $(LOCAL_PATH)
LOCAL_CFLAGS        := -O2 -DMBEDTLS_CONFIG_FILE=\"mbedtls_config.h\"
LOCAL_EXPORT_C_INCLUDES := $(MBEDTLS_DIR)/include $(LOCAL_PATH)
include $(BUILD_STATIC_LIBRARY)

# ── rpcdroid executable ─────────────────────────────────────────────────
include $(CLEAR_VARS)
LOCAL_MODULE        := rpcdroid
LOCAL_SRC_FILES     := main.c gateway.c websocket.c detector.c config.c \
                       log.c cJSON.c
LOCAL_C_INCLUDES    := $(LOCAL_PATH)
LOCAL_CFLAGS        := -O2 -Wall -Wextra -std=c11 -D_GNU_SOURCE \
                       -DMBEDTLS_CONFIG_FILE=\"mbedtls_config.h\"
LOCAL_STATIC_LIBRARIES := mbedtls_static
LOCAL_LDLIBS        := -llog
include $(BUILD_EXECUTABLE)
