LOCAL_PATH := $(call my-dir)

# ── mbedTLS static library ──────────────────────────────────────────────
include $(CLEAR_VARS)
LOCAL_MODULE        := mbedtls_static
MBEDTLS_DIR         := $(LOCAL_PATH)/mbedtls
LOCAL_SRC_FILES     := $(wildcard $(MBEDTLS_DIR)/library/*.c) \
                       $(LOCAL_PATH)/entropy_hardware_alt.c
LOCAL_C_INCLUDES    := $(MBEDTLS_DIR)/include $(LOCAL_PATH)
LOCAL_CFLAGS        := -O2 -DMBEDTLS_CONFIG_FILE=\"mbedtls_config.h\"
LOCAL_EXPORT_C_INCLUDES := $(MBEDTLS_DIR)/include $(LOCAL_PATH)
include $(BUILD_STATIC_LIBRARY)

# ── rpcdroid executable (C + C++) ───────────────────────────────────────
include $(CLEAR_VARS)
LOCAL_MODULE        := rpcdroid

LOCAL_C_INCLUDES    := $(LOCAL_PATH) $(LOCAL_PATH)/include
LOCAL_CPPFLAGS      := -std=c++20 -O2 -fexceptions -fno-rtti
LOCAL_CFLAGS        := -O2 -Wall -Wextra -std=c11 -D_GNU_SOURCE \
                       -DMBEDTLS_CONFIG_FILE=\"mbedtls_config.h\"

# C sources (existing modules)
LOCAL_SRC_FILES     := websocket.c config.cpp \
                       cJSON.c
# C++ sources
LOCAL_SRC_FILES     += main.cpp log.cpp gateway.cpp

LOCAL_STATIC_LIBRARIES := mbedtls_static spdlog \
                          BinderNDK BinderMonitor
LOCAL_LDLIBS        := -llog -lbinder_ndk -landroid

include $(BUILD_EXECUTABLE)

# ── Encore base + external (must come after BUILD_EXECUTABLE) ────────────
include $(LOCAL_PATH)/external/Android.mk $(LOCAL_PATH)/base/Android.mk
