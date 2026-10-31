<script setup>
import { ref, onMounted } from 'vue'
import { useConfigStore } from '../stores/config'
import { useStatusStore } from '../stores/status'
import { readFile, writeFile } from '../helpers/exec'
import Icon from '../components/Icon.vue'

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

const STATUS_OPTIONS = ['online', 'idle', 'dnd']
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

const exportMsg = ref('')
async function exportLog() {
  const raw = await readFile(LOG_PATH)
  if (!raw) { exportMsg.value = 'No log file'; setTimeout(() => exportMsg.value = '', 3000); return }
  const dest = '/storage/emulated/0/Download/rpcdroid.log'
  const ok = await writeFile(dest, raw)
  exportMsg.value = ok ? `Exported to ${dest}` : 'Export failed'
  setTimeout(() => exportMsg.value = '', 4000)
}
</script>

<template>
  <div class="page">
    <!-- Token -->
    <section class="card stack">
      <div class="sec-head">
        <h3><Icon name="key" :size="16" /> Discord token</h3>
        <span v-if="statusStore.hasToken" class="badge badge--ok"><Icon name="check" :size="12" :stroke="2.6" /> Saved</span>
        <span v-else class="badge badge--bad">Not set</span>
      </div>

      <textarea
        v-model="token" class="input" :class="{ obscured: !showToken }" rows="3"
        placeholder="Paste your Discord token here…"
        autocomplete="off" autocorrect="off" spellcheck="false"
      />

      <div class="row">
        <button class="btn btn--ghost btn--sm" @click="showToken = !showToken">{{ showToken ? 'Hide' : 'Show' }}</button>
        <button class="btn btn--primary" :disabled="tokenLoading || token.trim().length < 10" @click="saveToken">
          {{ tokenLoading ? 'Saving…' : 'Save token' }}
        </button>
      </div>

      <button class="disclosure" @click="showHowTo = !showHowTo">
        <Icon name="chevron" :size="15" :style="{ transform: showHowTo ? 'rotate(90deg)' : 'none', transition: 'transform .2s' }" />
        How to get your token
      </button>

      <div v-if="showHowTo" class="stack">
        <div class="banner banner--warn">
          <Icon name="alert" :size="15" />
          <span><strong>Never share your token.</strong> It gives full access to your Discord account. It is stored locally on this device only.</span>
        </div>

        <div class="method">
          <p class="method-title">Method 1 · Android browser</p>
          <ol>
            <li>Open Discord Web (<code>discord.com/app</code>) and log in.</li>
            <li>Paste the snippet into the address bar, keeping the <code>javascript:</code> prefix.</li>
            <li>Your token appears on the page. Copy it here.</li>
          </ol>
          <div class="snippet">
            <pre>{{ ANDROID_SNIPPET }}</pre>
            <button class="copy" @click="copySnippet('android', ANDROID_SNIPPET)">
              <Icon :name="copiedSnippet === 'android' ? 'check' : 'copy'" :size="13" />{{ copiedSnippet === 'android' ? 'Copied' : 'Copy' }}
            </button>
          </div>
        </div>

        <div class="method">
          <p class="method-title">Method 2 · PC browser console</p>
          <ol>
            <li>Open Discord Web on a desktop browser.</li>
            <li>Press <kbd>F12</kbd> and open the Console tab.</li>
            <li>Paste the snippet, press Enter, and copy the result here.</li>
          </ol>
          <div class="snippet">
            <pre>{{ PC_SNIPPET }}</pre>
            <button class="copy" @click="copySnippet('pc', PC_SNIPPET)">
              <Icon :name="copiedSnippet === 'pc' ? 'check' : 'copy'" :size="13" />{{ copiedSnippet === 'pc' ? 'Copied' : 'Copy' }}
            </button>
          </div>
        </div>
      </div>
    </section>

    <!-- Presence -->
    <section class="card stack">
      <div class="sec-head"><h3><Icon name="zap" :size="16" /> Presence</h3></div>

      <div class="field">
        <label class="field-label">Poll interval <span class="val">{{ configStore.pollInterval }}s</span></label>
        <input v-model.number="configStore.pollInterval" type="range" min="2" max="30" step="1" class="range" />
        <div class="range-hints"><span>2s · faster</span><span>30s · lighter</span></div>
      </div>

      <div class="field">
        <label class="field-label">Application ID <span class="adv">Advanced</span></label>
        <input v-model="configStore.applicationId" class="input" type="text" inputmode="numeric" placeholder="Discord application ID" />
      </div>
    </section>

    <!-- Daemon -->
    <section class="card stack">
      <div class="sec-head"><h3><Icon name="file" :size="16" /> Daemon</h3></div>

      <div class="field">
        <label class="field-label">Log level</label>
        <div class="seg">
          <button v-for="l in LOG_LEVELS" :key="l.value" :class="{ on: configStore.logLevel === l.value }" @click="configStore.logLevel = l.value">
            {{ l.label }}
          </button>
        </div>
      </div>

      <button class="btn btn--ghost" :disabled="logsLoading" @click="viewLogs">
        <Icon name="file" :size="16" /> {{ logsLoading ? 'Loading…' : 'View logs' }}
      </button>
      <button class="btn btn--ghost" @click="exportLog">
        <Icon name="copy" :size="16" /> Export log
      </button>
      <span v-if="exportMsg" class="export-msg">{{ exportMsg }}</span>

      <div v-if="logsContent !== null" class="logs">
        <div class="logs-bar">
          <span>Last 60 lines</span>
          <button class="copy static" @click="copySnippet('logs', logsContent)">
            <Icon :name="copiedSnippet === 'logs' ? 'check' : 'copy'" :size="13" />{{ copiedSnippet === 'logs' ? 'Copied' : 'Copy' }}
          </button>
        </div>
        <pre>{{ logsContent }}</pre>
      </div>
    </section>

    <button class="btn btn--primary btn--block" @click="saveSettings">Save settings</button>

    <Transition name="toast">
      <div v-if="toastMsg" class="toast" :class="{ 'toast--error': toastError }">{{ toastMsg }}</div>
    </Transition>
  </div>
</template>

<style scoped>
.obscured { -webkit-text-security: disc; }

.disclosure {
  display: flex; align-items: center; gap: 6px; align-self: flex-start; padding: 0;
  border: 0; background: none; color: var(--accent); font-size: 13px; font-weight: 600; cursor: pointer;
}

.method { display: flex; flex-direction: column; gap: 8px; }
.method-title { font-size: 13px; font-weight: 700; }
.method ol { margin: 0; padding-left: 18px; font-size: 13px; line-height: 1.65; color: var(--muted); }
.method code, kbd {
  padding: 1px 6px; border-radius: 6px; font-size: 11.5px; font-family: ui-monospace, monospace;
  background: var(--surface-2); color: var(--text); border: 1px solid var(--border);
}

.snippet { position: relative; }
.snippet pre {
  margin: 0; padding: 12px 76px 12px 12px; max-height: 96px; overflow-y: auto;
  background: var(--surface-2); border: 1px solid var(--border); border-radius: 12px;
  font: 11px/1.5 ui-monospace, monospace; color: var(--text); white-space: pre-wrap; word-break: break-all;
}
.copy {
  position: absolute; top: 8px; right: 8px; display: inline-flex; align-items: center; gap: 5px;
  height: 28px; padding: 0 10px; border: 0; border-radius: 8px;
  background: var(--accent-soft); color: var(--accent); font-size: 11px; font-weight: 700; cursor: pointer;
}
.copy.static { position: static; }

.val { color: var(--accent); font-weight: 700; font-size: 13px; }
.adv { font-size: 10px; font-weight: 800; letter-spacing: 0.05em; padding: 2px 7px; border-radius: 6px; background: var(--accent-soft); color: var(--accent); }
.range { width: 100%; accent-color: var(--accent); height: 24px; }
.range-hints { display: flex; justify-content: space-between; font-size: 11px; color: var(--muted); margin-top: -4px; }

.logs { background: var(--surface-2); border: 1px solid var(--border); border-radius: 14px; overflow: hidden; }
.logs-bar {
  display: flex; align-items: center; justify-content: space-between; padding: 8px 10px 8px 12px;
  border-bottom: 1px solid var(--border); font-size: 11px; font-weight: 700; letter-spacing: 0.06em; text-transform: uppercase; color: var(--muted);
}
.logs pre {
  margin: 0; padding: 12px; max-height: 280px; overflow-y: auto;
  font: 11px/1.65 ui-monospace, monospace; color: var(--muted); white-space: pre-wrap; word-break: break-all;
}
</style>
