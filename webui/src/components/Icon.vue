<script setup>
// Google Material Icons (Apache-2.0) from @material-design-icons/svg, inlined at build time.
import { computed } from 'vue'

const outlined = import.meta.glob('/node_modules/@material-design-icons/svg/outlined/{home,sports_esports,tune,power_settings_new,play_arrow,stop,restart_alt,search,add,delete,edit,close,chevron_right,expand_more,check,warning,vpn_key,description,content_copy,image,bolt,schedule,person,palette}.svg', { query: '?raw', import: 'default', eager: true })
const filled = import.meta.glob('/node_modules/@material-design-icons/svg/filled/{home,sports_esports,tune,power_settings_new,play_arrow,stop,restart_alt,search,add,delete,edit,close,chevron_right,expand_more,check,warning,vpn_key,description,content_copy,image,bolt,schedule,person,palette}.svg', { query: '?raw', import: 'default', eager: true })

const inner = svg => svg.replace(/^[\s\S]*?<svg[^>]*>/, '').replace(/<\/svg>\s*$/, '')
const index = (glob) => Object.fromEntries(
  Object.entries(glob).map(([p, raw]) => [p.split('/').pop().replace('.svg', ''), inner(raw)])
)
const OUT = index(outlined)
const FILL = index(filled)

const props = defineProps({
  name: { type: String, required: true },
  size: { type: Number, default: 24 },
  filled: { type: Boolean, default: false }
})
const html = computed(() => (props.filled ? FILL[props.name] : OUT[props.name]) || OUT[props.name] || '')
</script>

<template>
  <svg :width="size" :height="size" viewBox="0 0 24 24" fill="currentColor" aria-hidden="true" v-html="html" />
</template>
