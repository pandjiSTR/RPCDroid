<script setup>
defineProps({ modelValue: Boolean, disabled: { type: Boolean, default: false } })
defineEmits(['update:modelValue'])
</script>

<template>
  <button
    type="button" role="switch" class="toggle"
    :aria-checked="modelValue" :disabled="disabled"
    :class="{ 'toggle--on': modelValue }"
    @click="!disabled && $emit('update:modelValue', !modelValue)"
  >
    <span class="thumb" />
  </button>
</template>

<style scoped>
.toggle {
  width: 48px; height: 28px; border-radius: 99px; border: 1px solid var(--border);
  background: var(--surface-2); position: relative; cursor: pointer; flex-shrink: 0;
  transition: background 0.2s, border-color 0.2s;
}
.toggle--on { background: var(--grad); border-color: transparent; }
.toggle:disabled { opacity: 0.5; cursor: not-allowed; }
.thumb {
  position: absolute; top: 3px; left: 3px; width: 20px; height: 20px; border-radius: 50%;
  background: #fff; box-shadow: 0 2px 6px rgba(0, 0, 0, 0.35);
  transition: transform 0.22s cubic-bezier(0.3, 1.4, 0.5, 1);
}
.toggle--on .thumb { transform: translateX(20px); }
</style>
