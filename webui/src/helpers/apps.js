import { pm, listPackages, getPackagesInfo } from 'kernelsu'
import { ksuExec } from './exec'

export const KNOWN_GAMES = {
  'com.garena.game.mho': {
    name: 'Monster Hunter Outlanders',
    icon: 'https://cdn.discordapp.com/icons/1281639437708820612/97f43c3ef222a90b1cb9241ac390fa41.png'
  },
  'com.mobile.legends': {
    name: 'Mobile Legends: Bang Bang',
    appId: '1441992389232165056'
  },
  'com.vng.mlbbvn': {
    name: 'Mobile Legends: Bang Bang',
    appId: '1441992389232165056'
  },
  'com.dts.freefireth': {
    name: 'Free Fire',
    appId: '1441993244895477830'
  },
  'com.dts.freefiremax': {
    name: 'Free Fire MAX',
    appId: '1441993244895477830'
  },
  'com.garena.game.freefire': {
    name: 'Free Fire',
    appId: '1441993244895477830'
  },
  'com.miHoYo.GenshinImpact': {
    name: 'Genshin Impact',
    appId: '762434991303950386'
  },
  'com.HoYoverse.hkrpgoversea': {
    name: 'Honkai: Star Rail',
    appId: '1121201675240210523'
  },
  'com.miHoYo.hkrpg': {
    name: 'Honkai: Star Rail',
    appId: '1121201675240210523'
  },
  'com.miHoYo.HonkaiStarRail': {
    name: 'Honkai: Star Rail',
    appId: '1121201675240210523'
  },
  'com.miHoYo.bh3oversea': {
    name: 'Honkai Impact 3rd',
    appId: '1124351937098301590'
  },
  'com.miHoYo.bh3': {
    name: 'Honkai Impact 3rd',
    appId: '1124351937098301590'
  },
  'com.tencent.ig': {
    name: 'PUBG MOBILE',
    appId: '1402418717781921935'
  },
  'com.pubg.krmobile': {
    name: 'PUBG MOBILE',
    appId: '1402418717781921935'
  },
  'com.rekoo.pubgmnewstate': {
    name: 'NEW STATE Mobile',
    appId: '1402418717781921935'
  },
  'com.activision.callofduty.shooter': {
    name: 'Call of Duty: Mobile',
    appId: '1500967742772215959'
  },
  'com.garena.game.codm': {
    name: 'Call of Duty: Mobile',
    appId: '1500967742772215959'
  },
  'com.riotgames.league.wildrift': {
    name: 'League of Legends: Wild Rift',
    appId: '1402418696126992445'
  },
  'com.blizzard.wildrift': {
    name: 'League of Legends: Wild Rift',
    appId: '1402418696126992445'
  },
  'com.supercell.brawlstars': {
    name: 'Brawl Stars',
    appId: '1420796862369239162'
  },
  'com.supercell.clashofclans': {
    name: 'Clash of Clans',
    appId: '1441990808399253575'
  },
  'com.supercell.clashroyale': {
    name: 'Clash Royale',
    appId: '1420796862369239162'
  },
  'com.roblox.client': {
    name: 'Roblox',
    appId: '363445589247131668'
  },
  'com.mojang.minecraftpe': {
    name: 'Minecraft',
    appId: '1402418491272986635'
  },
  'com.kurogame.wutheringwaves.global': {
    name: 'Wuthering Waves',
    appId: '1247227126416146462'
  },
  'com.kurogame.wutheringwaves': {
    name: 'Wuthering Waves',
    appId: '1247227126416146462'
  },
  'com.YoStarEN.Arknights': {
    name: 'Arknights',
    appId: '1441993124996972685'
  },
  'com.YoStarKR.Arknights': {
    name: 'Arknights',
    appId: '1441993124996972685'
  },
  'com.YoStarJP.Arknights': {
    name: 'Arknights',
    appId: '1441993124996972685'
  },
  'com.nexon.bluearchive': {
    name: 'Blue Archive',
    appId: '1390512760860704889'
  },
  'com.proximabeta.nikke': {
    name: 'GODDESS OF VICTORY: NIKKE',
    appId: '1493468373311422615'
  },
  'com.ShiftUp.NIKKE': {
    name: 'GODDESS OF VICTORY: NIKKE',
    appId: '1493468373311422615'
  },
  'com.levelinfinite.hotta.gp': {
    name: 'Tower of Fantasy',
    appId: '1355934660961501254'
  },
  'com.devsisters.ck': {
    name: 'Cookie Run: Kingdom',
    appId: '1442736661237731358'
  },
  'com.devsisters.cookierunkingdom': {
    name: 'Cookie Run: Kingdom',
    appId: '1442736661237731358'
  },
  'com.lilithgame.roc.gp': {
    name: 'Rise of Kingdoms',
    appId: '1441992604911665204'
  },
  'com.ihg.rise.of.kingdoms': {
    name: 'Rise of Kingdoms',
    appId: '1441992604911665204'
  },
  'com.innersloth.spacemafia': {
    name: 'Among Us',
    appId: '1402418440685486130'
  },
  'com.epicgames.fortnite': {
    name: 'Fortnite',
    appId: '1402418703554842694'
  },
  'com.plarium.raidlegends': {
    name: 'Raid: Shadow Legends',
    appId: '1201708402397421618'
  },
  'com.igg.android.lordsmobile': {
    name: 'Lords Mobile',
    appId: '1124352468223012984'
  },
  'com.gameloft.android.ANMP.GloftA9HM': {
    name: 'Asphalt Legends',
    appId: '1440142141027520583'
  },
  'com.king.candycrushsaga': {
    name: 'Candy Crush Saga',
    appId: '1440139657655353394'
  },
  'com.king.candycrushsodasaga': {
    name: 'Candy Crush Soda Saga',
    appId: '1441991036334243901'
  },
  'com.king.farmheroessaga': {
    name: 'Farm Heroes Saga',
    appId: '1440141913473810465'
  },
  'com.halfbrick.jetpackjoyride': {
    name: 'Jetpack Joyride',
    appId: '1440539272900776008'
  },
  'com.rovio.baba': {
    name: 'Angry Birds 2',
    appId: '1441990393683251361'
  },
  'com.rovio.angrybirdsfriends': {
    name: 'Angry Birds Friends',
    appId: '1441990560268288212'
  },
  'com.miniclip.plagueinc': {
    name: 'Plague Inc',
    appId: '1442727662325071912'
  },
  'com.ea.game.pvzfree_row': {
    name: 'Plants vs. Zombies',
    appId: '498985095378632704'
  },
  'com.ea.game.starwarscapital_row': {
    name: 'Star Wars: Galaxy of Heroes',
    appId: '1441991227485585559'
  },
  'com.gameloft.android.ANMP.GloftM5HM': {
    name: 'Modern Combat 5',
    appId: '1124351993230676018'
  },
  'com.fingersoft.hillclimb': {
    name: 'Hill Climb Racing',
    appId: '1440140513306218616'
  },
  'com.wemade.mir4global': {
    name: 'MIR 4',
    appId: '1124351982908493894'
  },
  'com.rockstargames.bully': {
    name: 'Bully',
    appId: '451560105008431124'
  },
  'com.miniclip.agar.io': {
    name: 'Agar.io',
    appId: '1440140390501191742'
  },
  'com.levelinfinite.sgame': {
    name: 'Honor of Kings',
    appId: '1506502874044039248'
  },
  'com.tencent.tmgp.sgame': {
    name: 'Honor of Kings',
    appId: '1506502874044039248'
  }
}

export function getIconDisplayUrl(icon) {
  if (!icon) return ''
  if (icon.startsWith('mp:')) {
    // mp:external/... -> https://media.discordapp.net/external/...
    return 'https://media.discordapp.net/' + icon.replace(/^mp:\/?/, '')
  }
  return icon
}

export async function getInstalledApps() {
  // Method 1: KernelSU pm API (non-system / user packages with labels)
  try {
    let pkgs = []
    if (pm && pm.support) {
      pkgs = pm.listPackages('user')
    } else if (typeof listPackages === 'function') {
      pkgs = listPackages('user')
    }

    if (Array.isArray(pkgs) && pkgs.length > 0) {
      const getInfo = (pm && pm.support) ? pm.getPackagesInfo : getPackagesInfo
      let infoList = []
      try {
        infoList = getInfo(pkgs)
      } catch (err) {
        console.warn('getPackagesInfo error:', err)
      }

      if (Array.isArray(infoList) && infoList.length > 0) {
        const apps = infoList
          .filter(app => app && app.packageName && !app.isSystem)
          .map(app => {
            const known = KNOWN_GAMES[app.packageName]
            const fallbackLabel = app.packageName.split('.').pop()
            let label = (known && known.name) || app.appLabel || fallbackLabel
            if (label.toLowerCase() === 'mho' && app.packageName === 'com.garena.game.mho') {
              label = 'Monster Hunter Outlanders'
            }
            return {
              packageName: app.packageName,
              appLabel: label
            }
          })
        if (apps.length > 0) {
          apps.sort((a, b) => a.appLabel.localeCompare(b.appLabel))
          return apps
        }
      } else {
        const apps = pkgs.map(p => {
          const known = KNOWN_GAMES[p]
          const last = p.split('.').pop()
          const label = (known && known.name) || (last.charAt(0).toUpperCase() + last.slice(1))
          return {
            packageName: p,
            appLabel: label
          }
        })
        apps.sort((a, b) => a.appLabel.localeCompare(b.appLabel))
        return apps
      }
    }
  } catch (e) {
    console.warn('KernelSU pm API failed:', e)
  }

  // Method 2: Shell fallback via pm list packages -3 (third party only)
  try {
    const res = await ksuExec('pm list packages -3')
    if (res.exitCode === 0 && res.stdout) {
      const lines = res.stdout.split('\n')
      const apps = []
      for (const line of lines) {
        const trimmed = line.trim()
        if (!trimmed) continue
        const pkg = trimmed.replace(/^package:/, '').trim()
        if (pkg) {
          const known = KNOWN_GAMES[pkg]
          const last = pkg.split('.').pop()
          const label = (known && known.name) || (last.charAt(0).toUpperCase() + last.slice(1))
          apps.push({ packageName: pkg, appLabel: label })
        }
      }
      apps.sort((a, b) => a.appLabel.localeCompare(b.appLabel))
      return apps
    }
  } catch (e) {
    console.error('pm list packages fallback failed:', e)
  }

  return []
}

export async function fetchPlayStoreMetadata(pkg) {
  if (!pkg) return null
  const known = KNOWN_GAMES[pkg]
  if (known && known.appId) {
    return {
      title: known.name,
      icon: null,
      appId: known.appId
    }
  }

  try {
    const cmd = `
PKG="${pkg}"
URL=$(curl -s -L -A "Mozilla/5.0 (Windows NT 10.0; Win64; x64)" "https://play.google.com/store/apps/details?id=$PKG" 2>/dev/null)
TITLE=$(echo "$URL" | grep -o 'property="og:title" content="[^"]*"' | head -1 | cut -d'"' -f4 | sed 's/ - Apps on Google Play$//')
ICON=$(echo "$URL" | grep -o 'property="og:image" content="[^"]*"' | head -1 | cut -d'"' -f4)
[ -z "$ICON" ] && ICON=$(echo "$URL" | grep -o 'https://play-lh.googleusercontent.com/[^" ]*' | head -1)

# Daemon resolves http(s) icons to mp:external natively; keep raw Play Store URL.
echo "TITLE=$TITLE"
echo "ICON=$ICON"
`
    const r = await ksuExec(cmd)
    let title = null
    let icon = null
    if (r.exitCode === 0 && r.stdout) {
      const titleMatch = r.stdout.match(/TITLE=(.*)/)
      const iconMatch = r.stdout.match(/ICON=(.*)/)
      if (titleMatch && titleMatch[1].trim()) title = titleMatch[1].trim()
      if (iconMatch && iconMatch[1].trim()) icon = iconMatch[1].trim()
    }

    // Sanitize title if needed
    if (pkg === 'com.garena.game.mho' && (!title || title.toLowerCase() === 'mho')) {
      title = 'Monster Hunter Outlanders'
    }

    return {
      title: title || (known && known.name) || null,
      icon: (icon && (icon.startsWith('mp:') || icon.startsWith('https://') || icon.startsWith('http://')))
        ? icon
        : ((known && known.icon) || null),
      appId: (known && known.appId) || null
    }
  } catch (e) {
    console.warn('fetchPlayStoreMetadata failed:', e)
  }

  if (known) {
    return {
      title: known.name,
      icon: null,
      appId: known.appId || null
    }
  }

  return null
}

export async function fetchPlayStoreIcon(pkg) {
  const meta = await fetchPlayStoreMetadata(pkg)
  return meta ? meta.icon : null
}
