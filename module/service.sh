#!/system/bin/sh
MODDIR=${0%/*}
CONFIG_DIR=/data/adb/.config/drpc

# Wait for boot to complete
while [ "$(getprop sys.boot_completed)" != "1" ]; do
    sleep 2
done

# Extra wait for system to stabilize
sleep 5

# Create config dir if somehow missing
mkdir -p "$CONFIG_DIR"

# Check if token is configured
if [ ! -f "$CONFIG_DIR/token" ] || [ ! -s "$CONFIG_DIR/token" ]; then
    # No token configured, write status and exit - user must configure via WebUI
    echo '{"connected":false,"current_game":null,"discord_username":null,"error":"No token configured. Open WebUI to set your Discord token."}' > "$CONFIG_DIR/status.json"
    exit 0
fi

# Start daemon
"$MODDIR/bin/rpcdroid" daemon &
