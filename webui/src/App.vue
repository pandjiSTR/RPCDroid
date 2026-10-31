<script setup>
import NavBar from './components/NavBar.vue'
import { useStatusStore } from './stores/status'
import { onMounted } from 'vue'

const statusStore = useStatusStore()

function handleClose() {
  try {
    window.close()
  } catch (e) {
    // fallback: navigate back
  }
  // If window.close() is blocked (common in webviews), go back in history
  if (window.history.length > 1) {
    window.history.back()
  }
}

onMounted(() => {
  statusStore.startPolling()
})
</script>

<template>
  <div class="app-wrapper">
    <button
      class="close-btn"
      type="button"
      aria-label="Close and return to KernelSU"
      title="Close"
      @click="handleClose"
    >
      <svg width="20" height="20" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round" aria-hidden="true">
        <line x1="18" y1="6" x2="6" y2="18" />
        <line x1="6" y1="6" x2="18" y2="18" />
      </svg>
    </button>
    <main class="main-content">
      <RouterView />
    </main>
    <NavBar />
  </div>
</template>

<style scoped>
.app-wrapper {
  display: flex;
  flex-direction: column;
  min-height: 100vh;
}
.main-content {
  flex: 1;
  overflow-y: auto;
  padding-bottom: 64px;
  padding-top: 8px;
}
.close-btn {
  position: fixed;
  top: 8px;
  right: 8px;
  z-index: 200;
  width: 44px;
  height: 44px;
  display: flex;
  align-items: center;
  justify-content: center;
  background: var(--bg-card);
  color: var(--text-primary);
  border: 1px solid var(--border);
  border-radius: 8px;
  cursor: pointer;
  transition: background 0.15s, border-color 0.15s;
}
.close-btn:hover {
  background: var(--bg-input);
  border-color: var(--accent);
}
.close-btn:focus-visible {
  outline: 2px solid var(--accent);
  outline-offset: 2px;
}
</style>