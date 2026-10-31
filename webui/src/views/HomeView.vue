<script setup>
import { ref, computed, onMounted } from 'vue'
import { useRouter } from 'vue-router'
import { useStatusStore } from '../stores/status'
import { useGamesStore } from '../stores/games'
import { useConfigStore } from '../stores/config'
import { startDaemon, stopDaemon, restartDaemon } from '../helpers/exec'
import Icon from '../components/Icon.vue'
import GameIcon from '../components/GameIcon.vue'

const statusStore = useStatusStore()
const gamesStore = useGamesStore()
const configStore = useConfigStore()
const router = useRouter()

const actionLoading = ref(false)
const actionMsg = ref('')

onMounted(() => {
  if (!gamesStore.loaded) gamesStore.load()
  if (!configStore.loaded) configStore.load()
})

async function run(msg, fn, wait) {
  actionLoading.value = true
  actionMsg.value = msg
  await fn()
  await new Promise(r => setTimeout(r, wait))
  await statusStore.refresh()
  actionMsg.value = ''
  actionLoading.value = false
}
const doStart = () => run('Starting…', startDaemon, 2000)
const doStop = () => run('Stopping…', stopDaemon, 1000)
const doRestart = () => run('Restarting…', restartDaemon, 2500)

// Resolve the active game to a package so we can show its icon
const nowPlaying = computed(() => {
  const n = statusStore.currentGame
  // status.json goes stale when the daemon dies, so only trust it while running
  if (!n || !statusStore.isRunning) return null
  const hit = Object.entries(gamesStore.games).find(([pkg, g]) => pkg === n || g.name === n)
  return { name: hit?.[1]?.name || n, pkg: hit?.[0] || '', icon: hit?.[1]?.icon || '' }
})

const enabledCount = computed(() => Object.values(gamesStore.games).filter(g => g.enabled).length)
const totalCount = computed(() => Object.keys(gamesStore.games).length)
</script>

<template>
  <div class="page">
    <div v-if="!statusStore.hasToken" class="banner banner--warn tap" @click="router.push('/settings')">
      <Icon name="alert" :size="16" />
      <span>No Discord token configured. Tap to open Settings.</span>
    </div>

    <div v-if="statusStore.error" class="banner banner--error">
      <Icon name="alert" :size="16" />
      <span>{{ statusStore.error }}</span>
    </div>

    <!-- Hero -->
    <section class="card hero" :class="{ 'hero--on': statusStore.isRunning }">
      <div class="orb">
        <span class="ring" />
        <span class="core"><Icon name="power" :size="30" :stroke="2.2" /></span>
      </div>
      <div class="hero-text">
        <h2>{{ statusStore.isRunning ? 'Rich Presence active' : 'Daemon is stopped' }}</h2>
        <p class="muted">
          <template v-if="statusStore.isRunning && statusStore.discordUsername">Signed in as {{ statusStore.discordUsername }}</template>
          <template v-else-if="statusStore.isRunning">Waiting for Discord session…</template>
          <template v-else>Start the daemon to show your game on Discord</template>
        </p>
      </div>

      <div class="actions">
        <button v-if="!statusStore.isRunning" class="btn btn--primary btn--block"
                :disabled="actionLoading || !statusStore.hasToken" @click="doStart">
          <Icon name="play" :size="16" /> Start daemon
        </button>
        <template v-else>
          <button class="btn btn--danger" :disabled="actionLoading" @click="doStop">
            <Icon name="stop" :size="16" /> Stop
          </button>
          <button class="btn btn--ghost" :disabled="actionLoading" @click="doRestart">
            <Icon name="refresh" :size="16" /> Restart
          </button>
        </template>
      </div>
      <p v-if="actionMsg" class="hint"><span class="spinner sm" /> {{ actionMsg }}</p>
      <p v-else-if="!statusStore.hasToken && !statusStore.isRunning" class="hint">Add your token in Settings first.</p>
    </section>

    <!-- Now playing -->
    <section class="card now">
      <template v-if="nowPlaying">
        <GameIcon :pkg="nowPlaying.pkg" :icon="nowPlaying.icon" :name="nowPlaying.name" :size="54" />
        <div class="now-info">
          <span class="eyebrow">Now playing</span>
          <strong>{{ nowPlaying.name }}</strong>
        </div>
        <span class="live"><i />LIVE</span>
      </template>
      <template v-else>
        <span class="idle-ico"><Icon name="gamepad" :size="22" /></span>
        <div class="now-info">
          <span class="eyebrow">Now playing</span>
          <strong class="muted">{{ statusStore.isRunning ? 'No game detected' : 'Idle' }}</strong>
        </div>
      </template>
    </section>

    <!-- Stats -->
    <section class="stats">
      <div class="stat">
        <Icon name="gamepad" :size="16" />
        <strong>{{ enabledCount }}<small>/{{ totalCount }}</small></strong>
        <span>Games on</span>
      </div>
      <div class="stat">
        <Icon name="clock" :size="16" />
        <strong>{{ configStore.pollInterval }}<small>s</small></strong>
        <span>Poll interval</span>
      </div>
    </section>
  </div>
</template>

<style scoped>
.tap { cursor: pointer; }

.hero { display: flex; flex-direction: column; align-items: center; text-align: center; gap: 14px; padding: 24px 18px 18px; }
.hero--on { background: radial-gradient(120% 90% at 50% 0%, var(--accent-soft), transparent 65%), var(--surface); border-color: rgba(123, 131, 255, 0.3); }
.hero-text h2 { font-size: 20px; font-weight: 750; letter-spacing: -0.01em; }
.hero-text p { font-size: 13px; margin-top: 4px; }

.orb { position: relative; width: 96px; height: 96px; display: grid; place-items: center; }
.core {
  width: 72px; height: 72px; border-radius: 50%; display: grid; place-items: center;
  background: var(--surface-2); border: 1px solid var(--border); color: var(--muted);
  transition: all 0.3s;
}
.ring { position: absolute; inset: 0; border-radius: 50%; border: 2px solid var(--accent); opacity: 0; }
.hero--on .core { background: var(--grad); color: #fff; border-color: transparent; box-shadow: 0 0 44px -4px rgba(88, 101, 242, 0.75); }
.hero--on .ring { animation: ping 2.2s cubic-bezier(0, 0, 0.2, 1) infinite; }
@keyframes ping { 0% { transform: scale(0.75); opacity: 0.65; } 100% { transform: scale(1.3); opacity: 0; } }

.actions { display: flex; gap: 10px; width: 100%; }
.actions .btn { flex: 1; }
.hint { font-size: 12px; color: var(--muted); display: flex; align-items: center; gap: 8px; }
.spinner.sm { width: 14px; height: 14px; border-width: 2px; }

.now { display: flex; align-items: center; gap: 14px; padding: 14px; }
.now-info { flex: 1; min-width: 0; display: flex; flex-direction: column; gap: 3px; }
.now-info strong { font-size: 16px; font-weight: 700; line-height: 1.25; display: -webkit-box; -webkit-line-clamp: 2; -webkit-box-orient: vertical; overflow: hidden; }
.eyebrow { font-size: 11px; font-weight: 700; letter-spacing: 0.08em; text-transform: uppercase; color: var(--muted); }
.idle-ico { width: 54px; height: 54px; border-radius: 15px; display: grid; place-items: center; background: var(--surface-2); color: var(--muted); }
.live { display: inline-flex; align-items: center; gap: 6px; font-size: 10px; font-weight: 800; letter-spacing: 0.08em; padding: 5px 10px; border-radius: 99px; background: var(--green-soft); color: var(--green); }
.live i { width: 6px; height: 6px; border-radius: 50%; background: currentColor; animation: blink 1.4s ease-in-out infinite; }
@keyframes blink { 50% { opacity: 0.25; } }

.stats { display: grid; grid-template-columns: repeat(3, 1fr); gap: 10px; }
.stat { background: var(--surface); border: 1px solid var(--border); border-radius: 16px; padding: 12px; display: flex; flex-direction: column; gap: 4px; }
.stat svg { color: var(--accent); margin-bottom: 2px; }
.stat strong { font-size: 18px; font-weight: 750; }
.stat small { font-size: 12px; color: var(--muted); font-weight: 600; margin-left: 1px; }
.stat span { font-size: 11px; color: var(--muted); }
</style>
