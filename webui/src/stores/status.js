import { defineStore } from 'pinia'
import { ref } from 'vue'
import { isDaemonRunning, readFile } from '../helpers/exec'

export const useStatusStore = defineStore('status', () => {
  const isRunning = ref(false)
  const currentGame = ref(null)
  const discordUsername = ref(null)
  const error = ref(null)
  const hasToken = ref(false)
  let pollTimer = null

  async function refresh() {
    isRunning.value = await isDaemonRunning()

    const token = await readFile('/data/adb/.config/drpc/token')
    hasToken.value = !!(token && token.length > 10)

    const raw = await readFile('/data/adb/.config/drpc/status.json')
    if (raw) {
      try {
        const s = JSON.parse(raw)
        currentGame.value = s.current_game || null
        discordUsername.value = s.discord_username || null
        error.value = s.error || null
      } catch {}
    }
  }

  function startPolling() {
    refresh()
    pollTimer = setInterval(refresh, 3000)
  }

  function stopPolling() {
    if (pollTimer) clearInterval(pollTimer)
  }

  return { isRunning, currentGame, discordUsername, error, hasToken, refresh, startPolling, stopPolling }
})
