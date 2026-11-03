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
    echo '{"connected":false,"current_game":null,"discord_username":null,"error":"No token configured. Open WebUI to set your Discord token."}' > "$CONFIG_DIR/status.json"
    exit 0
fi

# Start daemon with watchdog
while true; do
    if [ -z "$(pidof rpcdroid)" ]; then
        "$MODDIR/bin/rpcdroid" daemon &
        DAEMON_PID=$!
        wait $DAEMON_PID 2>/dev/null
    fi
    sleep 10
done
