import { defineStore } from 'pinia'
import { ref } from 'vue'
import { readFile, writeFile } from '../helpers/exec'
import { KNOWN_GAMES } from '../helpers/apps'

const GAMELIST_PATH = '/data/adb/.config/drpc/gamelist.json'

export const useGamesStore = defineStore('games', () => {
  const games = ref({})
  const loaded = ref(false)
  const loading = ref(false)

  async function load() {
    loading.value = true
    const raw = await readFile(GAMELIST_PATH)
    if (raw) {
      try {
        games.value = JSON.parse(raw)
      } catch {
        games.value = {}
      }
    }
    // Auto-fix legacy names and sync appId/icon for known games
    let changed = false
    for (const [pkg, g] of Object.entries(games.value)) {
      if (g.app_id === '477152881196269569' || g.app_id === '962990036020756480') {
        delete g.app_id
        changed = true
      }
      if (pkg === 'com.garena.game.mho' && !g.icon) {
        g.icon = 'https://cdn.discordapp.com/icons/1281639437708820612/97f43c3ef222a90b1cb9241ac390fa41.png'
        changed = true
      }
      const known = KNOWN_GAMES[pkg]
      if (known) {
        if (known.appId && (!g.app_id || g.app_id === '962990036020756480')) {
          g.app_id = known.appId
          changed = true
        }
        if (known.icon && !g.icon) {
          g.icon = known.icon
          changed = true
        }
        if ((!g.name || g.name === 'mho' || g.name === pkg.split('.').pop()) && known.name) {
          g.name = known.name
          changed = true
        }
      }
      // Remove raw URLs only for games that have app_id (since app_id games use Discord native icon)
      if (g.app_id && g.icon && g.icon.startsWith('http')) {
        delete g.icon
        changed = true
      }
    }
    if (changed) {
      await save()
    }
    loaded.value = true
    loading.value = false
  }

  async function save() {
    return writeFile(GAMELIST_PATH, JSON.stringify(games.value))
  }

  async function toggleGame(pkg) {
    if (games.value[pkg]) {
      games.value[pkg].enabled = !games.value[pkg].enabled
      await save()
    }
  }

  async function updateName(pkg, name) {
    if (games.value[pkg]) {
      games.value[pkg].name = name
      await save()
    }
  }

  async function updateIcon(pkg, icon) {
    if (games.value[pkg]) {
      if (icon) {
        games.value[pkg].icon = icon
      } else {
        delete games.value[pkg].icon
      }
      await save()
    }
  }

  async function updateAppId(pkg, appId) {
    if (games.value[pkg]) {
      if (appId) {
        games.value[pkg].app_id = appId
      } else {
        delete games.value[pkg].app_id
      }
      await save()
    }
  }

  async function addGame(pkg, name, icon = '', appId = '') {
    games.value[pkg] = {
      name: name || pkg.split('.').pop(),
      ...(icon ? { icon } : {}),
      ...(appId ? { app_id: appId } : {}),
      enabled: true
    }
    await save()
  }

  async function removeGame(pkg) {
    delete games.value[pkg]
    await save()
  }

  return { games, loaded, loading, load, save, toggleGame, updateName, updateIcon, updateAppId, addGame, removeGame }
})
