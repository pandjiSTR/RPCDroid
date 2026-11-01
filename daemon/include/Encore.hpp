#pragma once

#include <string>

#define NOTIFY_TITLE "RPCDroid"
#define LOG_TAG "RPCDroid"

#define CONFIG_DIR "/data/adb/.config/drpc"
#define MODPATH "/data/adb/modules/rpcdroid"

#define LOCK_FILE CONFIG_DIR "/.lock"
#define LOG_FILE CONFIG_DIR "/drpc.log"
#define CONFIG_FILE CONFIG_DIR "/config.json"
#define GAMELIST_FILE CONFIG_DIR "/gamelist.json"

#define MODULE_PROP MODPATH "/module.prop"
#define MODULE_UPDATE MODPATH "/update"
#define BINDER_RESOLVER MODPATH "/binder_resolver.apk"

enum EncoreProfileMode : char {
    PERFCOMMON,
    PERFORMANCE_PROFILE,
    BALANCE_PROFILE,
    POWERSAVE_PROFILE
};

struct EncoreGameList {
    std::string package_name;
    bool lite_mode;
    bool enable_dnd;
};
