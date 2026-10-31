<script setup>
import { computed, onMounted, onUnmounted } from 'vue'
import { useRoute } from 'vue-router'
import NavBar from './components/NavBar.vue'
import Icon from './components/Icon.vue'
import { useStatusStore } from './stores/status'
import { exit } from 'kernelsu'

const statusStore = useStatusStore()
const route = useRoute()

const title = computed(() => ({ '/': 'RPCDroid', '/games': 'Games', '/settings': 'Settings' }[route.path] || 'RPCDroid'))

function handleClose() {
  try { exit() } catch (e) { /* ignored */ }
  try { window.close() } catch (e) { /* ignored */ }
  if (window.history.length > 1) window.history.back()
}

onMounted(() => statusStore.startPolling())
onUnmounted(() => statusStore.stopPolling())
</script>

<template>
  <header class="topbar">
    <div class="brand">
      <span class="logo"><Icon name="gamepad" :size="18" :stroke="2.2" /></span>
      <h1>{{ title }}</h1>
      <span class="pill" :class="statusStore.isRunning ? 'pill--on' : 'pill--off'">
        <i />{{ statusStore.isRunning ? 'Running' : 'Stopped' }}
      </span>
    </div>
    <button class="icon-btn" type="button" aria-label="Close" title="Close" @click="handleClose">
      <Icon name="x" :size="20" />
    </button>
  </header>

  <main>
    <RouterView v-slot="{ Component }">
      <Transition name="page" mode="out-in">
        <component :is="Component" />
      </Transition>
    </RouterView>
  </main>

  <NavBar />
</template>

<style scoped>
.topbar {
  position: sticky; top: 0; z-index: 50;
  display: flex; align-items: center; justify-content: space-between;
  padding: calc(env(safe-area-inset-top, 0px) + 12px) 16px 12px;
  background: linear-gradient(var(--bg) 55%, transparent);
}
.brand { display: flex; align-items: center; gap: 10px; min-width: 0; }
.brand h1 { font-size: 19px; font-weight: 750; letter-spacing: -0.01em; }
.logo {
  width: 32px; height: 32px; border-radius: 10px; display: grid; place-items: center;
  background: var(--grad); color: #fff; box-shadow: 0 8px 18px -8px rgba(88, 101, 242, 0.9);
}
.pill {
  display: inline-flex; align-items: center; gap: 6px; height: 22px; padding: 0 9px;
  border-radius: 99px; font-size: 11px; font-weight: 700;
}
.pill i { width: 6px; height: 6px; border-radius: 50%; background: currentColor; }
.pill--on { background: var(--green-soft); color: var(--green); }
.pill--on i { box-shadow: 0 0 8px currentColor; }
.pill--off { background: var(--surface-2); color: var(--muted); }
</style>
