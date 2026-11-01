<script setup>
import { useRoute } from 'vue-router'
import Icon from './Icon.vue'

const route = useRoute()
const tabs = [
  { to: '/', label: 'Home', icon: 'home' },
  { to: '/games', label: 'Games', icon: 'sports_esports' },
  { to: '/settings', label: 'Settings', icon: 'tune' }
]
</script>

<template>
  <nav class="nav">
    <RouterLink v-for="t in tabs" :key="t.to" :to="t.to" class="nav-item" :class="{ active: route.path === t.to }">
      <span class="nav-ind"><Icon :name="t.icon" :size="24" :filled="route.path === t.to" /></span>
      <span class="label">{{ t.label }}</span>
    </RouterLink>
  </nav>
</template>

<style scoped>
.nav {
  position: fixed; z-index: 100; left: 0; right: 0; bottom: 0;
  display: flex; gap: 8px; padding: 12px 8px calc(16px + env(safe-area-inset-bottom, 0px));
  background: var(--md-surface-container);
}
.nav-item {
  flex: 1; display: flex; flex-direction: column; align-items: center; gap: 4px;
  text-decoration: none; color: var(--md-on-surface-variant);
}
.nav-ind {
  width: 64px; height: 32px; border-radius: 16px; display: grid; place-items: center;
  transition: background .25s var(--ease), color .25s var(--ease);
}
.label { font-size: 12px; line-height: 16px; font-weight: 500; letter-spacing: .5px; }
.nav-item.active { color: var(--md-on-surface); }
.nav-item.active .nav-ind { background: var(--md-secondary-container); color: var(--md-on-secondary-container); }
</style>
