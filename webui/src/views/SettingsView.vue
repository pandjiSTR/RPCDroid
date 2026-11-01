<script setup>
import { ref, computed, onMounted } from 'vue'
import { useConfigStore } from '../stores/config'
import { useStatusStore } from '../stores/status'
import { readFile, writeFile } from '../helpers/exec'
import Icon from '../components/Icon.vue'
import { SEEDS, seed, mode, setSeed, setMode } from '../theme'

const configStore = useConfigStore()
const statusStore = useStatusStore()

const TOKEN_PATH = '/data/adb/.config/drpc/token'
const LOG_PATH = '/data/adb/.config/drpc/drpc.log'

const token = ref('')
const showToken = ref(false)
const tokenLoading = ref(false)
const showHowTo = ref(false)
const copiedSnippet = ref(null)

const toastMsg = ref('')
const toastError = ref(false)
let toastTimer = null

const logsContent = ref(null)
const logsLoading = ref(false)

const MODE_OPTIONS = [{ v: 'auto', l: 'Auto' }, { v: 'light', l: 'Light' }, { v: 'dark', l: 'Dark' }]
const LOG_LEVELS = [
  { value: 0, label: 'Debug' },
  { value: 1, label: 'Info' },
  { value: 2, label: 'Warn' },
  { value: 3, label: 'Error' }
]

const ANDROID_SNIPPET = `javascript:(function(){var i=document.createElement('iframe');i.onload=function(){var t=i.contentWindow.localStorage.token;document.body.innerHTML='<h1 style="word-break:break-all">'+t+'</h1>';};i.src='/login';document.body.appendChild(i);})()`

const PC_SNIPPET = `(webpackChunkdiscord_app.push([[''],{},e=>{m=[];for(let c in e.c)m.push(e.c[c])}]),m).find(m=>m?.exports?.default?.getToken).exports.default.getToken()`

onMounted(async () => {
  if (!configStore.loaded) await configStore.load()
  const raw = await readFile(TOKEN_PATH)
  if (raw && raw.length > 10) token.value = raw
})

function toast(msg, isError = false) {
  toastMsg.value = msg
  toastError.value = isError
  clearTimeout(toastTimer)
  toastTimer = setTimeout(() => { toastMsg.value = '' }, 2500)
}

async function saveToken() {
  const t = token.value.trim()
  if (t.length < 10) return
  tokenLoading.value = true
  const ok = await writeFile(TOKEN_PATH, t)
  await statusStore.refresh()
  tokenLoading.value = false
  ok ? toast('Token saved') : toast('Failed to save token', true)
}

async function copySnippet(which, text) {
  try {
    await navigator.clipboard.writeText(text)
    copiedSnippet.value = which
    setTimeout(() => { copiedSnippet.value = null }, 2000)
  } catch {}
}

async function saveSettings() {
  const ok = await configStore.save()
  ok ? toast('Settings saved') : toast('Failed to save settings', true)
}

async function viewLogs() {
  logsLoading.value = true
  logsContent.value = null
  const raw = await readFile(LOG_PATH)
  logsContent.value = raw ? raw.split('\n').slice(-60).join('\n') : '(No log file found)'
  logsLoading.value = false
}

const exporting = ref(false)
const exportMsg = ref('')
async function exportLog() {
  exporting.value = true
  const raw = await readFile(LOG_PATH)
  if (!raw) { exportMsg.value = 'No log file'; setTimeout(() => exportMsg.value = '', 3000); exporting.value = false; return }
  const dest = '/storage/emulated/0/Download/rpcdroid.log'
  const ok = await writeFile(dest, raw)
  exportMsg.value = ok ? `Exported to ${dest}` : 'Export failed'
  setTimeout(() => exportMsg.value = '', 4000)
  exporting.value = false
}

const pollPct = computed(() => ((configStore.pollInterval - 2) / 28) * 100 + '%')
</script>
<template>
  <div class="page">
    <!-- Token -->
    <h3 class="section-title t-label-l">Account</h3>
    <section class="card stack">
      <div class="row row--between">
        <div class="row"><Icon name="vpn_key" /><h3 class="t-title-m">Discord token</h3></div>
        <span v-if="statusStore.hasToken" class="tag tag--ok"><Icon name="check" :size="14" /> Saved</span>
        <span v-else class="tag err">Not set</span>
      </div>

      <div class="tf">
        <label>Token</label>
        <textarea v-model="token" :class="{ obscured: !showToken }" rows="3" placeholder="Paste your Discord token here"
                  autocomplete="off" autocorrect="off" spellcheck="false" />
      </div>

      <div class="row row--between">
        <button class="btn" @click="showToken = !showToken">{{ showToken ? 'Hide' : 'Show' }}</button>
        <button class="btn btn--filled" :disabled="tokenLoading || token.trim().length < 10" @click="saveToken">
          {{ tokenLoading ? 'Saving…' : 'Save token' }}
        </button>
      </div>

      <button class="btn disclosure" @click="showHowTo = !showHowTo">
        <Icon name="expand_more" :style="{ transform: showHowTo ? 'rotate(180deg)' : 'none', transition: 'transform .2s' }" />
        How to get your token
      </button>

      <div v-if="showHowTo" class="stack">
        <div class="banner banner--warn">
          <Icon name="warning" />
          <span class="t-body-m"><strong>Never share your token.</strong> It gives full access to your Discord account. It is stored locally on this device only.</span>
        </div>

        <div class="method">
          <p class="t-title-m">Method 1 · Android browser</p>
          <ol class="t-body-m variant">
            <li>Open Discord Web (<code>discord.com/app</code>) and log in.</li>
            <li>Paste the snippet into the address bar, keeping the <code>javascript:</code> prefix.</li>
            <li>Your token appears on the page. Copy it here.</li>
          </ol>
          <div class="snippet">
            <pre>{{ ANDROID_SNIPPET }}</pre>
            <button class="btn btn--tonal copy" @click="copySnippet('android', ANDROID_SNIPPET)">
              <Icon :name="copiedSnippet === 'android' ? 'check' : 'content_copy'" :size="18" />{{ copiedSnippet === 'android' ? 'Copied' : 'Copy' }}
            </button>
          </div>
        </div>

        <div class="method">
          <p class="t-title-m">Method 2 · PC browser console</p>
          <ol class="t-body-m variant">
            <li>Open Discord Web on a desktop browser.</li>
            <li>Press <kbd>F12</kbd> and open the Console tab.</li>
            <li>Paste the snippet, press Enter, and copy the result here.</li>
          </ol>
          <div class="snippet">
            <pre>{{ PC_SNIPPET }}</pre>
            <button class="btn btn--tonal copy" @click="copySnippet('pc', PC_SNIPPET)">
              <Icon :name="copiedSnippet === 'pc' ? 'check' : 'content_copy'" :size="18" />{{ copiedSnippet === 'pc' ? 'Copied' : 'Copy' }}
            </button>
          </div>
        </div>
      </div>
    </section>

    <!-- Appearance -->
    <h3 class="section-title t-label-l">Appearance</h3>
    <section class="card stack">
      <div class="row"><Icon name="palette" /><h3 class="t-title-m">Material You</h3></div>
      <div class="stack tight">
        <span class="t-label-l variant">Theme</span>
        <div class="seg">
          <button v-for="m in MODE_OPTIONS" :key="m.v" :class="{ on: mode === m.v }" @click="setMode(m.v)">
            <Icon v-if="mode === m.v" name="check" :size="18" />{{ m.l }}
          </button>
        </div>
      </div>
      <div class="stack tight">
        <span class="t-label-l variant">Accent colour</span>
        <div class="swatches">
          <button v-for="c in SEEDS" :key="c.hex" class="swatch" :style="{ background: c.hex }"
                  :aria-label="c.name" :aria-pressed="seed === c.hex" @click="setSeed(c.hex)">
            <Icon v-if="seed === c.hex" name="check" :size="22" />
          </button>
        </div>
        <p class="t-body-s variant">The whole palette is generated from this colour, like Material You dynamic colour.</p>
      </div>
    </section>

    <!-- Presence -->
    <h3 class="section-title t-label-l">Presence</h3>
    <section class="card stack">
      <div class="stack tight">
        <div class="row row--between"><span class="t-label-l variant">Poll interval</span><span class="t-title-m val">{{ configStore.pollInterval }}s</span></div>
        <input v-model.number="configStore.pollInterval" type="range" min="2" max="30" step="1" class="slider" :style="{ '--p': pollPct }" />
        <div class="row row--between t-body-s variant"><span>2s · faster</span><span>30s · lighter</span></div>
      </div>

      <div class="tf">
        <label>Application ID <span class="tag tag--action">Advanced</span></label>
        <input v-model="configStore.applicationId" type="text" inputmode="numeric" placeholder="Discord application ID" />
      </div>
    </section>

    <!-- Daemon -->
    <h3 class="section-title t-label-l">Daemon</h3>
    <section class="card stack">
      <div class="row"><Icon name="description" /><h3 class="t-title-m">Logging</h3></div>

      <div class="seg">
        <button v-for="l in LOG_LEVELS" :key="l.value" :class="{ on: configStore.logLevel === l.value }" @click="configStore.logLevel = l.value">
          <Icon v-if="configStore.logLevel === l.value" name="check" :size="18" />{{ l.label }}
        </button>
      </div>

      <button class="btn btn--outlined" :disabled="logsLoading" @click="viewLogs">
        <Icon name="description" :size="18" /> {{ logsLoading ? 'Loading…' : 'View logs' }}
      </button>
      <button class="btn btn--outlined" :disabled="exporting" @click="exportLog">
        <Icon name="file" :size="18" /> {{ exporting ? 'Exporting…' : 'Export log' }}
      </button>
      <span v-if="exportMsg" class="t-body-s variant">{{ exportMsg }}</span>

      <div v-if="logsContent !== null" class="logs">
        <div class="logs-bar">
          <span class="t-label-m variant">LAST 60 LINES</span>
          <button class="btn" @click="copySnippet('logs', logsContent)">
            <Icon :name="copiedSnippet === 'logs' ? 'check' : 'content_copy'" :size="18" />{{ copiedSnippet === 'logs' ? 'Copied' : 'Copy' }}
          </button>
        </div>
        <pre>{{ logsContent }}</pre>
      </div>
    </section>

    <button class="btn btn--filled btn--lg btn--block" @click="saveSettings">Save settings</button>

    <Transition name="snackbar">
      <div v-if="toastMsg" class="snackbar" :class="{ 'snackbar--error': toastError }">{{ toastMsg }}</div>
    </Transition>
  </div>
</template>

<style scoped>
.stack.tight { gap: 8px; }
.err { background: var(--md-error-container); color: var(--md-on-error-container); }
.obscured { -webkit-text-security: disc; }
.disclosure { align-self: flex-start; padding-left: 8px !important; }

.method { display: flex; flex-direction: column; gap: 8px; }
.method ol { margin: 0; padding-left: 20px; }
.method li { margin-bottom: 2px; }
.method code, kbd {
  padding: 1px 6px; border-radius: 6px; font: 12px ui-monospace, monospace;
  background: var(--md-surface-container-highest); color: var(--md-on-surface);
}

.snippet { position: relative; }
.snippet pre {
  margin: 0; padding: 12px 12px 52px; max-height: 132px; overflow-y: auto; border-radius: var(--shape-m);
  background: var(--md-surface-container-highest); color: var(--md-on-surface);
  font: 11px/1.5 ui-monospace, monospace; white-space: pre-wrap; word-break: break-all;
}
.copy { position: absolute; right: 8px; bottom: 8px; height: 36px; padding: 0 16px; }

.swatches { display: flex; justify-content: space-between; gap: 8px; }
.swatch {
  width: 42px; height: 42px; flex-shrink: 0; border: 0; border-radius: 50%; display: grid; place-items: center; cursor: pointer;
  color: #fff; transition: transform .2s var(--ease), border-radius .2s var(--ease);
}
.swatch[aria-pressed="true"] { border-radius: 14px; transform: scale(1.05); }

.val { color: var(--md-primary); }

.logs { border-radius: var(--shape-m); background: var(--md-surface-container-highest); overflow: hidden; }
.logs-bar { display: flex; align-items: center; justify-content: space-between; padding: 4px 4px 4px 16px; }
.logs pre {
  margin: 0; padding: 4px 16px 16px; max-height: 280px; overflow-y: auto;
  font: 11px/1.65 ui-monospace, monospace; color: var(--md-on-surface-variant); white-space: pre-wrap; word-break: break-all;
}
</style>
