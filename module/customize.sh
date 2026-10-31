#!/sbin/sh
SKIPUNZIP=1

ui_print "- RPCDroid"
ui_print "- by My-RPC"
ui_print " "

# API check - require Android 10+
if [ "$API" -lt 29 ]; then
    abort "! Android 10+ (API 29) required. Detected API: $API"
fi

ui_print "- Android API: $API"
ui_print "- Architecture: $ARCH"

# Architecture check
case "$ARCH" in
    arm64)
        ARCH_DIR="arm64-v8a"
        ;;
    arm)
        ARCH_DIR="armeabi-v7a"
        ;;
    x64|x86_64)
        ARCH_DIR="x86_64"
        ;;
    *)
        abort "! Unsupported architecture: $ARCH (only arm64, arm, and x64 supported)"
        ;;
esac

# Extract module files (everything except the native libs we don't need)
ui_print "- Extracting module files..."
unzip -o "$ZIPFILE" \
    'module.prop' \
    'service.sh' \
    'action.sh' \
    'uninstall.sh' \
    'gamelist.txt' \
    'META-INF/*' \
    'webroot/*' \
    -d "$MODPATH" >/dev/null 2>&1

# Extract daemon binary for correct arch
ui_print "- Extracting daemon for $ARCH ($ARCH_DIR)..."
mkdir -p "$MODPATH/bin"
unzip -o "$ZIPFILE" "libs/$ARCH_DIR/rpcdroid" -d "$MODPATH" >/dev/null 2>&1
mv "$MODPATH/libs/$ARCH_DIR/rpcdroid" "$MODPATH/bin/rpcdroid"
rm -rf "$MODPATH/libs"

# Set permissions
set_perm "$MODPATH/bin/rpcdroid" root root 0755
set_perm "$MODPATH/service.sh" root root 0755
set_perm "$MODPATH/action.sh" root root 0755
set_perm "$MODPATH/uninstall.sh" root root 0755

# Create config directory
ui_print "- Creating config directory..."
mkdir -p /data/adb/.config/drpc

# Write default config if not exists (preserve existing config on update)
if [ ! -f /data/adb/.config/drpc/config.json ]; then
    echo '{"poll_interval":5,"status":"online","application_id":"962990036020756480","log_level":1}' \
        > /data/adb/.config/drpc/config.json
    ui_print "- Default config written"
else
    ui_print "- Existing config preserved"
fi

# Write initial status
echo '{"connected":false,"current_game":null,"discord_username":null}' \
    > /data/adb/.config/drpc/status.json

# Generate or update gamelist.json
GAMELIST_OUT="/data/adb/.config/drpc/gamelist.json"

get_game_info() {
    _pkg="$1"
    _name=""
    _appid=""
    _icon=""
    case "$_pkg" in
        "com.garena.game.mho") _name="Monster Hunter Outlanders"; _appid=""; _icon="https://cdn.discordapp.com/icons/1281639437708820612/97f43c3ef222a90b1cb9241ac390fa41.png" ;;
        "com.mobile.legends"|"com.vng.mlbbvn") _name="Mobile Legends: Bang Bang"; _appid="1441992389232165056" ;;
        "com.dts.freefireth"|"com.dts.freefiremax"|"com.garena.game.freefire") _name="Free Fire"; _appid="1441993244895477830" ;;
        "com.miHoYo.GenshinImpact") _name="Genshin Impact"; _appid="762434991303950386" ;;
        "com.HoYoverse.hkrpgoversea"|"com.miHoYo.hkrpg"|"com.miHoYo.HonkaiStarRail") _name="Honkai: Star Rail"; _appid="1121201675240210523" ;;
        "com.miHoYo.bh3oversea"|"com.miHoYo.bh3") _name="Honkai Impact 3rd"; _appid="1124351937098301590" ;;
        "com.tencent.ig"|"com.pubg.krmobile"|"com.rekoo.pubgmnewstate") _name="PUBG MOBILE"; _appid="1402418717781921935" ;;
        "com.activision.callofduty.shooter"|"com.garena.game.codm") _name="Call of Duty: Mobile"; _appid="1500967742772215959" ;;
        "com.riotgames.league.wildrift"|"com.blizzard.wildrift") _name="League of Legends: Wild Rift"; _appid="1402418696126992445" ;;
        "com.supercell.brawlstars") _name="Brawl Stars"; _appid="1420796862369239162" ;;
        "com.supercell.clashofclans") _name="Clash of Clans"; _appid="1441990808399253575" ;;
        "com.supercell.clashroyale") _name="Clash Royale"; _appid="1420796862369239162" ;;
        "com.roblox.client") _name="Roblox"; _appid="363445589247131668" ;;
        "com.mojang.minecraftpe") _name="Minecraft"; _appid="1402418491272986635" ;;
        "com.kurogame.wutheringwaves.global"|"com.kurogame.wutheringwaves") _name="Wuthering Waves"; _appid="1247227126416146462" ;;
        "com.YoStarEN.Arknights"|"com.YoStarKR.Arknights"|"com.YoStarJP.Arknights") _name="Arknights"; _appid="1441993124996972685" ;;
        "com.nexon.bluearchive") _name="Blue Archive"; _appid="1390512760860704889" ;;
        "com.proximabeta.nikke"|"com.ShiftUp.NIKKE") _name="GODDESS OF VICTORY: NIKKE"; _appid="1493468373311422615" ;;
        "com.levelinfinite.hotta.gp") _name="Tower of Fantasy"; _appid="1355934660961501254" ;;
        "com.devsisters.ck"|"com.devsisters.cookierunkingdom") _name="Cookie Run: Kingdom"; _appid="1442736661237731358" ;;
        "com.lilithgame.roc.gp"|"com.ihg.rise.of.kingdoms") _name="Rise of Kingdoms"; _appid="1441992604911665204" ;;
        "com.innersloth.spacemafia") _name="Among Us"; _appid="1402418440685486130" ;;
        "com.epicgames.fortnite") _name="Fortnite"; _appid="1402418703554842694" ;;
        "com.plarium.raidlegends") _name="Raid: Shadow Legends"; _appid="1201708402397421618" ;;
        "com.igg.android.lordsmobile") _name="Lords Mobile"; _appid="1124352468223012984" ;;
        "com.gameloft.android.ANMP.GloftA9HM") _name="Asphalt Legends"; _appid="1440142141027520583" ;;
        "com.king.candycrushsaga") _name="Candy Crush Saga"; _appid="1440139657655353394" ;;
        "com.king.candycrushsodasaga") _name="Candy Crush Soda Saga"; _appid="1441991036334243901" ;;
        "com.king.farmheroessaga") _name="Farm Heroes Saga"; _appid="1440141913473810465" ;;
        "com.halfbrick.jetpackjoyride") _name="Jetpack Joyride"; _appid="1440539272900776008" ;;
        "com.rovio.baba") _name="Angry Birds 2"; _appid="1441990393683251361" ;;
        "com.miniclip.plagueinc") _name="Plague Inc"; _appid="1442727662325071912" ;;
        "com.ea.game.pvzfree_row") _name="Plants vs. Zombies"; _appid="498985095378632704" ;;
        "com.ea.game.starwarscapital_row") _name="Star Wars: Galaxy of Heroes"; _appid="1441991227485585559" ;;
        "com.gameloft.android.ANMP.GloftM5HM") _name="Modern Combat 5"; _appid="1124351993230676018" ;;
        "com.fingersoft.hillclimb") _name="Hill Climb Racing"; _appid="1440140513306218616" ;;
        "com.wemade.mir4global") _name="MIR 4"; _appid="1124351982908493894" ;;
        "com.rockstargames.bully") _name="Bully"; _appid="451560105008431124" ;;
        "com.miniclip.agar.io") _name="Agar.io"; _appid="1440140390501191742" ;;
        "com.levelinfinite.sgame"|"com.tencent.tmgp.sgame") _name="Honor of Kings"; _appid="1506502874044039248" ;;
        *) _name=$(echo "$_pkg" | awk -F. '{print $NF}'); _appid="" ;;
    esac
}

if [ -f "$GAMELIST_OUT" ] && [ -s "$GAMELIST_OUT" ] && [ "$(cat "$GAMELIST_OUT")" != "{}" ]; then
    ui_print "- Existing gamelist.json preserved"
    # Fix legacy names
    sed -i 's/"com.garena.game.mho":{"name":"mho"/"com.garena.game.mho":{"name":"Monster Hunter Outlanders"/g' "$GAMELIST_OUT" 2>/dev/null
    sed -i 's/"name":"mho"/"name":"Monster Hunter Outlanders"/g' "$GAMELIST_OUT" 2>/dev/null

    # Clean up broken raw Google Play icon URLs
    sed -i 's/"icon":"https:\/\/play-lh.googleusercontent.com\/[^"]*",*//g' "$GAMELIST_OUT" 2>/dev/null
    sed -i 's/,}/}/g' "$GAMELIST_OUT" 2>/dev/null

    # Migrate com.garena.game.mho from MH: World app_id to official Outlanders Discord icon
    sed -i 's/"app_id":"477152881196269569",*//g' "$GAMELIST_OUT" 2>/dev/null
    sed -i 's/"app_id":"962990036020756480",*//g' "$GAMELIST_OUT" 2>/dev/null
    sed -i 's/,}/}/g' "$GAMELIST_OUT" 2>/dev/null
    if ! grep -q '"com.garena.game.mho"' "$GAMELIST_OUT" 2>/dev/null; then
        sed -i 's/}$/,"com.garena.game.mho":{"name":"Monster Hunter Outlanders","icon":"https:\/\/cdn.discordapp.com\/icons\/1281639437708820612\/97f43c3ef222a90b1cb9241ac390fa41.png","enabled":true}}/' "$GAMELIST_OUT" 2>/dev/null
    elif ! grep -q '97f43c3ef222a90b1cb9241ac390fa41' "$GAMELIST_OUT" 2>/dev/null; then
        sed -i 's/"com.garena.game.mho":{"name":"Monster Hunter Outlanders"[^}]*/"com.garena.game.mho":{"name":"Monster Hunter Outlanders","icon":"https:\/\/cdn.discordapp.com\/icons\/1281639437708820612\/97f43c3ef222a90b1cb9241ac390fa41.png","enabled":true/g' "$GAMELIST_OUT" 2>/dev/null
    fi
else
    ui_print "- Scanning installed games..."
    first=1
    printf '{' > "$GAMELIST_OUT"
    count=0

    while IFS= read -r pkg; do
        [ -z "$pkg" ] && continue
        case "$pkg" in '#'*) continue ;; esac

        # Check if app is installed (multiple user/data paths)
        if [ -d "/data/data/$pkg" ] || [ -d "/data/user/0/$pkg" ] || [ -d "/data/user_de/0/$pkg" ]; then
            get_game_info "$pkg"
            if [ "$first" = "1" ]; then
                first=0
            else
                printf ',' >> "$GAMELIST_OUT"
            fi
            if [ -n "$_appid" ]; then
                printf '"%s":{"name":"%s","app_id":"%s","enabled":true}' "$pkg" "$_name" "$_appid" >> "$GAMELIST_OUT"
            elif [ -n "$_icon" ]; then
                printf '"%s":{"name":"%s","icon":"%s","enabled":true}' "$pkg" "$_name" "$_icon" >> "$GAMELIST_OUT"
            else
                printf '"%s":{"name":"%s","enabled":true}' "$pkg" "$_name" >> "$GAMELIST_OUT"
            fi
            count=$((count + 1))
        fi
    done < "$MODPATH/gamelist.txt"

    # If no installed games detected via folder check, populate full list
    if [ "$count" -eq 0 ]; then
        ui_print "- Populating default game list..."
        first=1
        while IFS= read -r pkg; do
            [ -z "$pkg" ] && continue
            case "$pkg" in '#'*) continue ;; esac
            get_game_info "$pkg"
            if [ "$first" = "1" ]; then
                first=0
            else
                printf ',' >> "$GAMELIST_OUT"
            fi
            if [ -n "$_appid" ]; then
                printf '"%s":{"name":"%s","app_id":"%s","enabled":true}' "$pkg" "$_name" "$_appid" >> "$GAMELIST_OUT"
            elif [ -n "$_icon" ]; then
                printf '"%s":{"name":"%s","icon":"%s","enabled":true}' "$pkg" "$_name" "$_icon" >> "$GAMELIST_OUT"
            else
                printf '"%s":{"name":"%s","enabled":true}' "$pkg" "$_name" >> "$GAMELIST_OUT"
            fi
            count=$((count + 1))
        done < "$MODPATH/gamelist.txt"
    fi

    printf '}' >> "$GAMELIST_OUT"
    ui_print "- Gamelist initialized with $count games"
fi

# Set permissions on config dir
set_perm_recursive /data/adb/.config/drpc root root 0755 0644

ui_print " "
ui_print "- Installation complete!"
ui_print "- Open the module WebUI to configure your Discord token"
ui_print "- The daemon will start automatically after reboot"
ui_print " "
