<script setup>
import { computed, onMounted, onUnmounted, ref } from 'vue'
import { useRoute } from 'vue-router'
import NavBar from './components/NavBar.vue'
import Icon from './components/Icon.vue'
import { useStatusStore } from './stores/status'
import { exit } from 'kernelsu'

const statusStore = useStatusStore()
const route = useRoute()
const scrolled = ref(false)

const title = computed(() => ({ '/': 'RPCDroid', '/games': 'Games', '/settings': 'Settings' }[route.path] || 'RPCDroid'))

function handleClose() {
  try { exit() } catch (e) { /* ignored */ }
  try { window.close() } catch (e) { /* ignored */ }
  if (window.history.length > 1) window.history.back()
}
const onScroll = () => { scrolled.value = window.scrollY > 4 }

onMounted(() => { statusStore.startPolling(); window.addEventListener('scroll', onScroll, { passive: true }) })
onUnmounted(() => { statusStore.stopPolling(); window.removeEventListener('scroll', onScroll) })
</script>

<template>
  <header class="appbar" :class="{ scrolled }">
    <h1 class="t-title-l">{{ title }}</h1>
    <span class="status t-label-m" :class="{ on: statusStore.isRunning }"><i />{{ statusStore.isRunning ? 'Running' : 'Stopped' }}</span>
    <span class="grow" />
    <button class="icon-btn" type="button" aria-label="Close" @click="handleClose"><Icon name="close" /></button>
  </header>

  <main>
    <RouterView v-slot="{ Component }">
      <Transition name="page" mode="out-in"><component :is="Component" /></Transition>
    </RouterView>
  </main>

  <NavBar />
</template>

<style scoped>
.appbar {
  position: sticky; top: 0; z-index: 50; height: calc(64px + env(safe-area-inset-top, 0px));
  padding: env(safe-area-inset-top, 0px) 4px 0 20px; display: flex; align-items: center; gap: 12px;
  background: var(--md-surface); transition: background .2s linear;
}
.appbar.scrolled { background: var(--md-surface-container); }
.status {
  display: inline-flex; align-items: center; gap: 6px; height: 24px; padding: 0 10px; border-radius: var(--shape-s);
  background: var(--md-surface-container-highest); color: var(--md-on-surface-variant);
}
.status i { width: 8px; height: 8px; border-radius: 50%; background: currentColor; }
.status.on { background: var(--md-primary-container); color: var(--md-on-primary-container); }
.status.on i { background: var(--md-primary); }
</style>
