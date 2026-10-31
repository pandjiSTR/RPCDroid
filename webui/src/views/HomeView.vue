<script setup>
import { useStatusStore } from '../stores/status'
import { startDaemon, stopDaemon, restartDaemon } from '../helpers/exec'
import { ref } from 'vue'
import { useRouter } from 'vue-router'

const statusStore = useStatusStore()
const router = useRouter()
const actionLoading = ref(false)
const actionMsg = ref('')

async function doStart() {
  actionLoading.value = true
  actionMsg.value = 'Starting...'
  await startDaemon()
  await new Promise(r => setTimeout(r, 2000))
  await statusStore.refresh()
  actionMsg.value = ''
  actionLoading.value = false
}

async function doStop() {
  actionLoading.value = true
  actionMsg.value = 'Stopping...'
  await stopDaemon()
  await new Promise(r => setTimeout(r, 1000))
  await statusStore.refresh()
  actionMsg.value = ''
  actionLoading.value = false
}

async function doRestart() {
  actionLoading.value = true
  actionMsg.value = 'Restarting...'
  await restartDaemon()
  await new Promise(r => setTimeout(r, 2500))
  await statusStore.refresh()
  actionMsg.value = ''
  actionLoading.value = false
}
</script>

<template>
  <div class="page">
    <!-- Header -->
    <div class="header">
      <div class="header-icon">
        <svg width="36" height="36" viewBox="0 0 24 24" fill="currentColor" aria-hidden="true">
          <path d="M15 7.5V2H9v5.5l3 3 3-3zM7.5 9H2v6h5.5l3-3-3-3zM9 16.5V22h6v-5.5l-3-3-3 3zM16.5 9l-3 3 3 3H22V9h-5.5z"/>
        </svg>
      </div>
      <div>
        <h1>RPCDroid</h1>
        <p class="version">Module v0.0.1</p>
      </div>
    </div>

    <!-- No token warning -->
    <div v-if="!statusStore.hasToken" class="banner banner--warn" @click="router.push('/settings')">
      <span class="banner-icon" aria-hidden="true">
        <svg width="16" height="16" viewBox="0 0 24 24" fill="currentColor"><path d="M1 21h22L12 2 1 21zm12-3h-2v-2h2v2zm0-4h-2v-4h2v4z"/></svg>
      </span>
      <span>No Discord token configured. Tap to go to Settings.</span>
    </div>

    <!-- Error banner -->
    <div v-if="statusStore.error" class="banner banner--error">
      <span class="banner-icon" aria-hidden="true">
        <svg width="16" height="16" viewBox="0 0 24 24" fill="currentColor"><path d="M19 6.41L17.59 5 12 10.59 6.41 5 5 6.41 10.59 12 5 17.59 6.41 19 12 13.41 17.59 19 19 17.59 13.41 12z"/></svg>
      </span>
      <span>{{ statusStore.error }}</span>
    </div>

    <!-- Status card -->
    <div class="card">
      <div class="status-row">
        <div class="status-dot" :class="statusStore.isRunning ? 'dot--green' : 'dot--red'"></div>
        <div>
          <div class="status-label">{{ statusStore.isRunning ? 'Daemon Running' : 'Daemon Stopped' }}</div>
          <div v-if="statusStore.discordUsername" class="status-sub">
            Logged in as {{ statusStore.discordUsername }}
          </div>
        </div>
      </div>

      <!-- Current game -->
      <div v-if="statusStore.currentGame" class="game-row">
        <span class="game-badge">▶ Playing</span>
        <span class="game-name">{{ statusStore.currentGame }}</span>
      </div>
      <div v-else-if="statusStore.isRunning" class="no-game">
        No game detected
      </div>
    </div>

    <!-- Actions -->
    <div class="card actions-card">
      <p class="card-label">Daemon Control</p>
      <div class="btn-row">
        <button
          v-if="!statusStore.isRunning"
          class="btn btn--primary"
          :disabled="actionLoading || !statusStore.hasToken"
          @click="doStart"
        >Start</button>
        <button
          v-if="statusStore.isRunning"
          class="btn btn--danger"
          :disabled="actionLoading"
          @click="doStop"
        >Stop</button>
        <button
          v-if="statusStore.isRunning"
          class="btn btn--secondary"
          :disabled="actionLoading"
          @click="doRestart"
        >Restart</button>
      </div>
      <p v-if="actionMsg" class="action-msg">{{ actionMsg }}</p>
      <p v-if="!statusStore.hasToken && !statusStore.isRunning" class="hint">
        Configure your token in Settings first.
      </p>
    </div>
  </div>
</template>

<style scoped>
.page { padding: 16px; display: flex; flex-direction: column; gap: 12px; }

.header { display: flex; align-items: center; gap: 12px; padding: 8px 0; }
.header-icon { font-size: 36px; }
.header h1 { margin: 0; font-size: 20px; font-weight: 700; color: var(--text-primary); }
.version { margin: 2px 0 0; font-size: 12px; color: var(--text-secondary); }

.banner {
  display: flex; align-items: center; gap: 8px;
  padding: 10px 14px; border-radius: 8px; font-size: 14px; cursor: pointer;
}
.banner--warn { background: rgba(254,231,92,0.15); color: var(--yellow); border: 1px solid rgba(254,231,92,0.3); }
.banner--error { background: rgba(237,66,69,0.15); color: var(--red); border: 1px solid rgba(237,66,69,0.3); cursor: default; }

.card { background: var(--bg-card); border-radius: 8px; padding: 16px; }

.status-row { display: flex; align-items: center; gap: 12px; }
.status-dot { width: 12px; height: 12px; border-radius: 50%; flex-shrink: 0; }
.dot--green { background: var(--green); box-shadow: 0 0 6px var(--green); }
.dot--red { background: var(--red); }
.status-label { font-size: 16px; font-weight: 600; color: var(--text-primary); }
.status-sub { font-size: 12px; color: var(--text-secondary); margin-top: 2px; }

.game-row { display: flex; align-items: center; gap: 8px; margin-top: 12px; padding-top: 12px; border-top: 1px solid var(--border); }
.game-badge { background: var(--accent); color: white; font-size: 11px; font-weight: 600; padding: 2px 8px; border-radius: 4px; }
.game-name { font-size: 15px; color: var(--text-primary); font-weight: 500; }
.no-game { margin-top: 10px; font-size: 13px; color: var(--text-secondary); }

.actions-card .card-label { margin: 0 0 12px; font-size: 12px; text-transform: uppercase; letter-spacing: 0.06em; color: var(--text-secondary); font-weight: 600; }
.btn-row { display: flex; gap: 8px; flex-wrap: wrap; }

.btn { padding: 10px 20px; border-radius: 6px; border: none; font-size: 14px; font-weight: 600; cursor: pointer; transition: opacity 0.15s; }
.btn:disabled { opacity: 0.5; cursor: not-allowed; }
.btn--primary { background: var(--accent); color: white; }
.btn--primary:not(:disabled):hover { background: var(--accent-hover); }
.btn--danger { background: var(--red); color: white; }
.btn--secondary { background: var(--bg-input); color: var(--text-primary); border: 1px solid var(--border); }

.action-msg { margin: 10px 0 0; font-size: 13px; color: var(--text-secondary); }
.hint { margin: 8px 0 0; font-size: 12px; color: var(--text-secondary); }
</style>
