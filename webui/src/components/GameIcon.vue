<script setup>
import { ref, computed, watch } from 'vue'
import { getIconDisplayUrl } from '../helpers/apps'

const props = defineProps({
  pkg: { type: String, default: '' },
  icon: { type: String, default: '' },
  name: { type: String, default: '' },
  size: { type: Number, default: 44 }
})

const failed = ref(false)
const src = computed(() => getIconDisplayUrl(props.icon) || (props.pkg ? `ksu://icon/${props.pkg}` : ''))
watch(src, () => { failed.value = false })

// Stable colour per game for the letter fallback
const hue = computed(() => {
  let h = 0
  for (const c of props.name || props.pkg) h = (h * 31 + c.charCodeAt(0)) % 360
  return h
})
const fallbackBg = computed(() =>
  `linear-gradient(135deg, hsl(${hue.value} 62% 52%), hsl(${(hue.value + 40) % 360} 62% 36%))`
)
</script>

<template>
  <div class="gicon" :style="{ width: size + 'px', height: size + 'px', borderRadius: size * 0.27 + 'px' }">
    <img v-if="src && !failed" :src="src" :alt="name" loading="lazy" @error="failed = true" />
    <span v-else :style="{ background: fallbackBg, fontSize: size * 0.42 + 'px' }">
      {{ (name || pkg || '?').trim().charAt(0).toUpperCase() }}
    </span>
  </div>
</template>

<style scoped>
.gicon { flex-shrink: 0; overflow: hidden; background: var(--surface-2); border: 1px solid var(--border); }
.gicon img { width: 100%; height: 100%; object-fit: cover; display: block; }
.gicon span { width: 100%; height: 100%; display: grid; place-items: center; color: #fff; font-weight: 700; }
</style>
