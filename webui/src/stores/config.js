import { defineStore } from 'pinia'
import { ref } from 'vue'
import { readFile, writeFile } from '../helpers/exec'

const CONFIG_PATH = '/data/adb/.config/drpc/config.json'

export const useConfigStore = defineStore('config', () => {
  const pollInterval = ref(3)
  const applicationId = ref('962990036020756480')
  const logLevel = ref(1)
  const loaded = ref(false)

  async function load() {
    const raw = await readFile(CONFIG_PATH)
    if (raw) {
      try {
        const c = JSON.parse(raw)
        pollInterval.value = c.poll_interval ?? 3
        applicationId.value = c.application_id ?? '962990036020756480'
        logLevel.value = c.log_level ?? 1
      } catch {}
    }
    loaded.value = true
  }

  async function save() {
    const obj = {
      poll_interval: pollInterval.value,
      application_id: applicationId.value,
      log_level: logLevel.value
    }
    return writeFile(CONFIG_PATH, JSON.stringify(obj))
  }

  return { pollInterval, applicationId, logLevel, loaded, load, save }
})
