<script setup>
defineProps({ modelValue: Boolean, disabled: { type: Boolean, default: false } })
defineEmits(['update:modelValue'])
</script>

<template>
  <button
    type="button"
    role="switch"
    :aria-checked="modelValue"
    :disabled="disabled"
    class="toggle"
    :class="{ 'toggle--on': modelValue, 'toggle--disabled': disabled }"
    @click="!disabled && $emit('update:modelValue', !modelValue)"
  >
    <span class="toggle-thumb" />
  </button>
</template>

<style scoped>
.toggle {
  width: 44px;
  height: 24px;
  border-radius: 12px;
  background: var(--border);
  border: none;
  cursor: pointer;
  position: relative;
  transition: background 0.2s;
  flex-shrink: 0;
}
.toggle--on { background: var(--accent); }
.toggle--disabled { opacity: 0.5; cursor: not-allowed; }
.toggle-thumb {
  position: absolute;
  top: 2px;
  left: 2px;
  width: 20px;
  height: 20px;
  border-radius: 50%;
  background: white;
  transition: transform 0.2s;
}
.toggle--on .toggle-thumb { transform: translateX(20px); }
</style>
