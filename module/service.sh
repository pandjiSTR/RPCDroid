#
# Copyright (C) 2024-2026 PandjiSTR
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

MODDIR=$(dirname "$0")
CONFIG_DIR=/data/adb/.config/drpc
FLAG="$CONFIG_DIR/enabled"

# Wait for boot to complete
while [ "$(getprop sys.boot_completed)" != "1" ]; do
    sleep 2
done

# Extra wait for system to stabilize
sleep 5

# Create config dir if somehow missing
mkdir -p "$CONFIG_DIR"

# If token exists but enabled flag missing, auto-enable (default after boot)
if [ -f "$CONFIG_DIR/token" ] && [ -s "$CONFIG_DIR/token" ] && [ ! -f "$FLAG" ]; then
    touch "$FLAG"
fi

# If no token, write status error but keep loop idle
if [ ! -f "$CONFIG_DIR/token" ] || [ ! -s "$CONFIG_DIR/token" ]; then
    echo '{"connected":false,"current_game":null,"discord_username":null,"error":"No token configured. Open WebUI to set your Discord token."}' > "$CONFIG_DIR/status.json"
fi

# Watchdog loop: spawn daemon only when enabled flag exists
while true; do
    if [ -f "$FLAG" ] && [ -s "$CONFIG_DIR/token" ]; then
        if [ -z "$(pidof rpcdroid)" ]; then
            "$MODDIR/bin/rpcdroid" daemon >/dev/null 2>&1 </dev/null
        fi
    fi
    sleep 5
done
