#!/system/bin/sh
# Try different WebUI launchers in order
MODDIR=${0%/*}

# Try KSUWebUIStandalone
if command -v ksud >/dev/null 2>&1; then
    ksud webui "$MODDIR" && exit 0
fi

# Try MMRL WebUI
if command -v mmrl >/dev/null 2>&1; then
    mmrl webui "$MODDIR" && exit 0
fi

# Try am start with WebUI X
if pm list packages 2>/dev/null | grep -q "com.dergoogler.mmrl"; then
    am start -n com.dergoogler.mmrl/.ui.activity.WebUIActivity \
        --es moduleId discord-rpc 2>/dev/null && exit 0
fi

# Fallback: open KSUWebUIStandalone install page
am start -a android.intent.action.VIEW \
    -d "https://github.com/5ec1cff/KsuWebUIStandalone/releases" 2>/dev/null
