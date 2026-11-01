<script setup>
import Icon from './Icon.vue'
defineProps({ modelValue: Boolean, disabled: { type: Boolean, default: false } })
defineEmits(['update:modelValue'])
</script>

<template>
  <button
    type="button" role="switch" class="toggle no-ripple"
    :aria-checked="modelValue" :disabled="disabled" :class="{ 'toggle--on': modelValue }"
    @click="!disabled && $emit('update:modelValue', !modelValue)"
  >
    <span class="thumb"><Icon v-if="modelValue" name="check" :size="16" /></span>
  </button>
</template>

<style scoped>
/* Material 3 switch: 52×32, small outline thumb off, large on-primary thumb with check on */
.toggle {
  width: 52px; height: 32px; flex-shrink: 0; padding: 0; cursor: pointer; overflow: visible;
  border: 2px solid var(--md-outline); border-radius: 16px; background: var(--md-surface-container-highest);
  transition: background .2s var(--ease), border-color .2s var(--ease);
}
.toggle--on { background: var(--md-primary); border-color: var(--md-primary); }
.toggle:disabled { opacity: .38; cursor: not-allowed; }
.thumb {
  position: absolute; top: 50%; left: 4px; width: 16px; height: 16px; margin-top: -8px; border-radius: 50%;
  background: var(--md-outline); color: var(--md-primary); display: grid; place-items: center;
  transition: all .2s var(--ease);
}
.toggle--on .thumb { left: 22px; width: 24px; height: 24px; margin-top: -12px; background: var(--md-on-primary); }
.toggle:active .thumb { width: 28px; height: 28px; margin-top: -14px; left: 2px; }
.toggle--on:active .thumb { left: 20px; }
</style>
