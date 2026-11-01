<script setup>
import { ref, computed, watch } from 'vue'
import { getIconDisplayUrl } from '../helpers/apps'

const props = defineProps({
  pkg: { type: String, default: '' },
  icon: { type: String, default: '' },
  name: { type: String, default: '' },
  size: { type: Number, default: 48 }
})

const failed = ref(false)
const src = computed(() => getIconDisplayUrl(props.icon) || (props.pkg ? `ksu://icon/${props.pkg}` : ''))
watch(src, () => { failed.value = false })

// Letter fallback cycles through the three tonal containers
const tone = computed(() => {
  let h = 0
  for (const c of props.name || props.pkg) h = (h * 31 + c.charCodeAt(0)) % 3
  return ['primary', 'secondary', 'tertiary'][h]
})
</script>

<template>
  <div class="gicon" :style="{ width: size + 'px', height: size + 'px', borderRadius: size * 0.28 + 'px' }">
    <img v-if="src && !failed" :src="src" :alt="name" loading="lazy" @error="failed = true" />
    <span v-else :class="'tone-' + tone" :style="{ fontSize: size * 0.42 + 'px' }">
      {{ (name || pkg || '?').trim().charAt(0).toUpperCase() }}
    </span>
  </div>
</template>

<style scoped>
.gicon { flex-shrink: 0; overflow: hidden; background: var(--md-surface-container-highest); }
.gicon img { width: 100%; height: 100%; object-fit: cover; display: block; }
.gicon span { width: 100%; height: 100%; display: grid; place-items: center; font-weight: 500; letter-spacing: 0; }
.tone-primary { background: var(--md-primary-container); color: var(--md-on-primary-container); }
.tone-secondary { background: var(--md-secondary-container); color: var(--md-on-secondary-container); }
.tone-tertiary { background: var(--md-tertiary-container); color: var(--md-on-tertiary-container); }
</style>
