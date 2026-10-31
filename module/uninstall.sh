#!/system/bin/sh
# Remove config directory
rm -rf /data/adb/.config/drpc
# Remove symlinks if any
rm -f /data/adb/ksu/bin/rpcdroid
rm -f /data/adb/ap/bin/rpcdroid
