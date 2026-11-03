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
  // poll until daemon state matches intent (max ~10s)
  const target = msg.startsWith('Stopping') ? false : true
  for (let i = 0; i < 20; i++) {
    await new Promise(r => setTimeout(r, 500))
    await statusStore.refresh()
    if (statusStore.isRunning === target) break
  }
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
    <button v-if="!statusStore.hasToken" class="banner banner--warn no-ripple" @click="router.push('/settings')">
      <Icon name="warning" />
      <span class="grow t-body-m">No Discord token configured. Tap to open Settings.</span>
      <Icon name="chevron_right" />
    </button>

    <div v-if="statusStore.error" class="banner banner--error">
      <Icon name="warning" /><span class="t-body-m">{{ statusStore.error }}</span>
    </div>

    <!-- Hero -->
    <section class="hero" :class="{ 'hero--on': statusStore.isRunning }">
      <div class="orb">
        <span class="ring" />
        <span class="core">
          <img v-if="statusStore.discordAvatar" :src="statusStore.discordAvatar" alt="Profile" class="avatar" />
          <Icon v-else name="power_settings_new" :size="36" />
        </span>
      </div>
      <div class="hero-text">
        <h2 class="t-headline-s">{{ statusStore.isRunning ? 'Rich Presence active' : 'Daemon is stopped' }}</h2>
        <p v-if="statusStore.isRunning" class="t-body-s" :class="statusStore.connected ? 'ok' : 'warn'">
          Gateway: {{ statusStore.connected ? 'connected' : 'disconnected' }}
        </p>
        <p class="t-body-m">
          <template v-if="statusStore.isRunning && statusStore.discordUsername">Signed in as {{ statusStore.discordUsername }}</template>
          <template v-else-if="statusStore.isRunning">Waiting for Discord session…</template>
          <template v-else>Start the daemon to show your game on Discord</template>
        </p>
      </div>

      <div class="actions">
        <button v-if="!statusStore.isRunning" class="btn btn--filled btn--lg btn--block"
                :disabled="actionLoading || !statusStore.hasToken" @click="doStart">
          <Icon name="play_arrow" /> Start daemon
        </button>
        <template v-else>
          <button class="btn btn--outlined btn--lg grow stop" :disabled="actionLoading" @click="doStop"><Icon name="stop" /> Stop</button>
          <button class="btn btn--filled btn--lg grow" :disabled="actionLoading" @click="doRestart"><Icon name="restart_alt" /> Restart</button>
        </template>
      </div>
      <p v-if="actionMsg" class="hint t-body-s"><span class="spinner sm" /> {{ actionMsg }}</p>
      <p v-else-if="!statusStore.hasToken && !statusStore.isRunning" class="hint t-body-s">Add your token in Settings first.</p>
    </section>

    <!-- Now playing -->
    <section class="card card--low now">
      <template v-if="nowPlaying">
        <GameIcon :pkg="nowPlaying.pkg" :icon="nowPlaying.icon" :name="nowPlaying.name" :size="56" />
        <div class="grow info">
          <span class="t-label-m variant">NOW PLAYING</span>
          <strong class="t-title-m name">{{ nowPlaying.name }}</strong>
        </div>
        <span class="tag tag--ok live"><i />Live</span>
      </template>
      <template v-else>
        <span class="idle"><Icon name="sports_esports" :size="28" /></span>
        <div class="grow info">
          <span class="t-label-m variant">NOW PLAYING</span>
          <strong class="t-title-m variant">{{ statusStore.isRunning ? 'No game detected' : 'Idle' }}</strong>
        </div>
      </template>
    </section>

    <!-- Stats -->
    <section class="stats">
      <div class="stat">
        <Icon name="sports_esports" :size="20" />
        <strong class="t-title-l">{{ enabledCount }}<small class="t-body-m">/{{ totalCount }}</small></strong>
        <span class="t-label-m">Games on</span>
      </div>
      <div class="stat">
        <Icon name="schedule" :size="20" />
        <strong class="t-title-l">{{ configStore.pollInterval }}<small class="t-body-m">s</small></strong>
        <span class="t-label-m">Poll interval</span>
      </div>
      <div class="stat">
        <Icon name="vpn_key" :size="20" />
        <strong class="t-title-l">{{ statusStore.pid || '—' }}</strong>
        <span class="t-label-m">Daemon PID</span>
      </div>
    </section>
  </div>
</template>

<style scoped>
.banner { border: 0; width: 100%; text-align: left; cursor: pointer; font-family: inherit; }

.hero {
  display: flex; flex-direction: column; align-items: center; text-align: center; gap: 16px;
  padding: 28px 20px 20px; border-radius: var(--shape-xl);
  background: var(--md-surface-container-high); color: var(--md-on-surface);
  transition: background .3s var(--ease);
}
.hero--on { background: var(--md-primary-container); color: var(--md-on-primary-container); }
.hero-text p { margin-top: 4px; opacity: .78; }

.orb { position: relative; width: 112px; height: 112px; display: grid; place-items: center; }
.core {
  width: 88px; height: 88px; border-radius: 50%; display: grid; place-items: center;
  background: var(--md-surface-container-highest); color: var(--md-on-surface-variant); transition: all .3s var(--ease);
  overflow: hidden;
}
.core .avatar { width: 100%; height: 100%; object-fit: cover; border-radius: 50%; }
.ring { position: absolute; inset: 0; border-radius: 50%; border: 3px solid var(--md-primary); opacity: 0; }
.hero--on .core { background: var(--md-primary); color: var(--md-on-primary); }
.hero--on .ring { animation: ping 2.4s cubic-bezier(0, 0, .2, 1) infinite; }
@keyframes ping { 0% { transform: scale(.78); opacity: .7; } 100% { transform: scale(1.25); opacity: 0; } }

.actions { display: flex; gap: 12px; width: 100%; }
.stop { color: inherit; border-color: currentColor; }
.hint { display: flex; align-items: center; gap: 8px; opacity: .78; }
.spinner.sm { width: 14px; height: 14px; border-width: 2px; }

.now { display: flex; align-items: center; gap: 16px; padding: 16px; border-radius: var(--shape-l); }
.info { display: flex; flex-direction: column; gap: 2px; }
.name { display: -webkit-box; -webkit-line-clamp: 2; -webkit-box-orient: vertical; overflow: hidden; }
.idle { width: 56px; height: 56px; border-radius: 16px; display: grid; place-items: center; background: var(--md-surface-container-highest); color: var(--md-on-surface-variant); }
.live i { width: 8px; height: 8px; border-radius: 50%; background: var(--md-tertiary); animation: blink 1.4s ease-in-out infinite; }
@keyframes blink { 50% { opacity: .25; } }

.stats { display: grid; grid-template-columns: repeat(3, 1fr); gap: 8px; }
.stat {
  background: var(--md-secondary-container); color: var(--md-on-secondary-container);
  border-radius: var(--shape-l); padding: 14px 14px 12px; display: flex; flex-direction: column; gap: 2px;
}
.stat svg { margin-bottom: 6px; }
.stat small { opacity: .75; margin-left: 1px; }
.stat span { opacity: .78; }
</style>
