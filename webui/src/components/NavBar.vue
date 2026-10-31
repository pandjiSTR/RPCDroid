<script setup>
import { useRoute } from 'vue-router'
import Icon from './Icon.vue'

const route = useRoute()
const tabs = [
  { to: '/', label: 'Home', icon: 'home' },
  { to: '/games', label: 'Games', icon: 'gamepad' },
  { to: '/settings', label: 'Settings', icon: 'sliders' }
]
</script>

<template>
  <nav class="nav">
    <RouterLink
      v-for="t in tabs" :key="t.to" :to="t.to"
      class="nav-item" :class="{ active: route.path === t.to }"
    >
      <Icon :name="t.icon" :size="21" />
      <span>{{ t.label }}</span>
    </RouterLink>
  </nav>
</template>

<style scoped>
.nav {
  position: fixed; z-index: 100;
  left: 16px; right: 16px; bottom: calc(12px + env(safe-area-inset-bottom, 0px));
  height: 64px; padding: 6px; display: flex; gap: 4px;
  background: var(--glass); border: 1px solid var(--border-strong);
  border-radius: 22px; backdrop-filter: blur(18px); -webkit-backdrop-filter: blur(18px);
  box-shadow: 0 18px 40px -16px rgba(0, 0, 0, 0.7);
}
.nav-item {
  flex: 1; display: flex; flex-direction: column; align-items: center; justify-content: center; gap: 3px;
  border-radius: 16px; text-decoration: none; color: var(--muted);
  font-size: 11px; font-weight: 600; transition: background 0.2s, color 0.2s;
}
.nav-item.active { background: var(--accent-soft); color: var(--accent); }
</style>
