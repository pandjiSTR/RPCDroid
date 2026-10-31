<script setup>
import { ref, onMounted } from 'vue'
import { useConfigStore } from '../stores/config'
import { useStatusStore } from '../stores/status'
import { readFile, writeFile } from '../helpers/exec'

const configStore = useConfigStore()
const statusStore = useStatusStore()

const TOKEN_PATH = '/data/adb/.config/drpc/token'
const LOG_PATH = '/data/adb/.config/drpc/drpc.log'

const token = ref('')
const showToken = ref(false)
const tokenSaved = ref(false)
const tokenLoading = ref(false)

const showHowTo = ref(false)
const copiedSnippet = ref(null)

const saveMsg = ref('')
const saveError = ref(false)

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
  if (raw && raw.length > 10) {
    token.value = raw
    tokenSaved.value = true
  }
})

async function saveToken() {
  tokenLoading.value = true
  const t = token.value.trim()
  if (t.length < 10) {
    tokenLoading.value = false
    return
  }
  await writeFile(TOKEN_PATH, t)
  await statusStore.refresh()
  tokenSaved.value = true
  tokenLoading.value = false
}

async function copySnippet(which, text) {
  try {
    await navigator.clipboard.writeText(text)
    copiedSnippet.value = which
    setTimeout(() => { copiedSnippet.value = null }, 2000)
  } catch {}
}

async function saveSettings() {
  saveMsg.value = ''
  const ok = await configStore.save()
  if (ok) {
    saveMsg.value = 'Settings saved!'
    saveError.value = false
  } else {
    saveMsg.value = 'Failed to save settings.'
    saveError.value = true
  }
  setTimeout(() => { saveMsg.value = '' }, 3000)
}

async function viewLogs() {
  logsLoading.value = true
  logsContent.value = null
  const raw = await readFile(LOG_PATH)
  if (!raw) {
    logsContent.value = '(No log file found)'
  } else {
    const lines = raw.split('\n')
    logsContent.value = lines.slice(-60).join('\n')
  }
  logsLoading.value = false
}
</script>

<template>
  <div class="page">
    <div class="page-header">
      <h2>Settings</h2>
    </div>

    <!-- ── Discord Token ── -->
    <section class="card">
      <p class="section-label">Discord Token</p>

      <div class="token-status">
        <span v-if="statusStore.hasToken" class="badge badge--green">
          <svg width="12" height="12" viewBox="0 0 24 24" fill="currentColor" aria-hidden="true"><path d="M9 16.17L4.83 12l-1.42 1.41L9 19 21 7l-1.41-1.41z"/></svg>
          Token saved
        </span>
        <span v-else class="badge badge--red">Not configured</span>
      </div>

      <div class="token-input-wrap">
        <textarea
          v-model="token"
          :type="showToken ? 'text' : 'password'"
          class="token-textarea"
          :class="{ 'token-obscured': !showToken }"
          rows="3"
          placeholder="Paste your Discord token here…"
          autocomplete="off"
          autocorrect="off"
          spellcheck="false"
        />
      </div>

      <div class="token-row">
        <label class="checkbox-label">
          <input type="checkbox" v-model="showToken" />
          Show token
        </label>
        <button
          class="btn btn--primary"
          :disabled="tokenLoading || token.trim().length < 10"
          @click="saveToken"
        >{{ tokenLoading ? 'Saving…' : 'Save Token' }}</button>
      </div>

      <!-- How to get token -->
      <button class="collapsible-btn" @click="showHowTo = !showHowTo">
        <svg
          width="14" height="14" viewBox="0 0 24 24" fill="currentColor"
          :style="{ transform: showHowTo ? 'rotate(90deg)' : 'none', transition: 'transform 0.2s' }"
        >
          <path d="M10 6L8.59 7.41 13.17 12l-4.58 4.59L10 18l6-6z"/>
        </svg>
        How to get your token
      </button>

      <div v-if="showHowTo" class="howto">
        <div class="warning-box">
          <svg width="14" height="14" viewBox="0 0 24 24" fill="currentColor" aria-hidden="true"><path d="M1 21h22L12 2 1 21zm12-3h-2v-2h2v2zm0-4h-2v-4h2v4z"/></svg>
          <strong>Never share your token.</strong> It grants full access to your Discord account.
          This app stores it locally on your device only.
        </div>

        <div class="method">
          <p class="method-title">Method 1 — Android Browser</p>
          <ol class="method-steps">
            <li>Open Discord Web (<code>discord.com/app</code>) in your browser and log in.</li>
            <li>Tap the address bar and paste the snippet below as the URL (starting with <code>javascript:</code>).</li>
            <li>Your token will appear on the page — copy and paste it here.</li>
          </ol>
          <div class="snippet-wrap">
            <pre class="snippet">{{ ANDROID_SNIPPET }}</pre>
            <button class="copy-btn" @click="copySnippet('android', ANDROID_SNIPPET)">
              {{ copiedSnippet === 'android' ? 'Copied' : 'Copy' }}
            </button>
          </div>
        </div>

        <div class="method">
          <p class="method-title">Method 2 — PC Browser Console</p>
          <ol class="method-steps">
            <li>Open Discord Web in a desktop browser.</li>
            <li>Press <kbd>F12</kbd> → Console tab.</li>
            <li>Paste the snippet and press Enter.</li>
            <li>Copy the returned token string here.</li>
          </ol>
          <div class="snippet-wrap">
            <pre class="snippet">{{ PC_SNIPPET }}</pre>
            <button class="copy-btn" @click="copySnippet('pc', PC_SNIPPET)">
              {{ copiedSnippet === 'pc' ? 'Copied' : 'Copy' }}
            </button>
          </div>
        </div>
      </div>
    </section>

    <!-- ── Presence Settings ── -->
    <section class="card">
      <p class="section-label">Presence Settings</p>

      <div class="field">
        <label class="field-label">Status</label>
        <div class="segmented">
          <button
            v-for="opt in STATUS_OPTIONS"
            :key="opt"
            class="seg-btn"
            :class="{ 'seg-btn--active': configStore.status === opt }"
            @click="configStore.status = opt"
          >{{ opt.charAt(0).toUpperCase() + opt.slice(1) }}</button>
        </div>
      </div>

      <div class="field">
        <div class="field-label-row">
          <label class="field-label">Poll Interval</label>
          <span class="field-value">{{ configStore.pollInterval }}s</span>
        </div>
        <input
          v-model.number="configStore.pollInterval"
          type="range"
          min="2"
          max="30"
          step="1"
          class="range-input"
        />
        <div class="range-hints">
          <span>2s</span><span>30s</span>
        </div>
      </div>

      <div class="field">
        <label class="field-label">
          Application ID
          <span class="advanced-tag">Advanced</span>
        </label>
        <input
          v-model="configStore.applicationId"
          class="text-input"
          type="text"
          placeholder="Discord application ID"
          inputmode="numeric"
        />
      </div>
    </section>

    <!-- ── Daemon ── -->
    <section class="card">
      <p class="section-label">Daemon</p>

      <div class="field">
        <label class="field-label">Log Level</label>
        <select v-model.number="configStore.logLevel" class="select-input">
          <option v-for="lvl in LOG_LEVELS" :key="lvl.value" :value="lvl.value">
            {{ lvl.label }}
          </option>
        </select>
      </div>

      <button class="btn btn--secondary logs-btn" :disabled="logsLoading" @click="viewLogs">
        <svg width="15" height="15" viewBox="0 0 24 24" fill="currentColor">
          <path d="M14 2H6c-1.1 0-2 .9-2 2v16c0 1.1.9 2 2 2h12c1.1 0 2-.9 2-2V8l-6-6zm-1 7V3.5L18.5 9H13zM8 13h8v1H8zm0 2h8v1H8zm0 2h5v1H8z"/>
        </svg>
        {{ logsLoading ? 'Loading…' : 'View Logs' }}
      </button>

      <div v-if="logsContent !== null" class="logs-block">
        <div class="logs-toolbar">
          <span class="logs-title">Last 60 lines</span>
          <button class="copy-btn" @click="copySnippet('logs', logsContent)">
            {{ copiedSnippet === 'logs' ? 'Copied' : 'Copy' }}
          </button>
        </div>
        <pre class="logs-pre">{{ logsContent }}</pre>
      </div>
    </section>

    <!-- Save button -->
    <button class="btn btn--primary save-btn" @click="saveSettings">Save Settings</button>
    <p v-if="saveMsg" class="save-msg" :class="{ 'save-msg--error': saveError }">
      {{ saveMsg }}
    </p>
  </div>
</template>

<style scoped>
.page { padding: 16px; display: flex; flex-direction: column; gap: 12px; }

.page-header h2 { margin: 0; font-size: 20px; font-weight: 700; color: var(--text-primary); }

.card { background: var(--bg-card); border-radius: 8px; padding: 16px; display: flex; flex-direction: column; gap: 14px; }

.section-label { margin: 0; font-size: 11px; text-transform: uppercase; letter-spacing: 0.07em; color: var(--text-secondary); font-weight: 700; }

.token-status { display: flex; align-items: center; }
.badge { font-size: 12px; font-weight: 600; padding: 3px 10px; border-radius: 12px; }
.badge--green { background: rgba(87,242,135,0.15); color: var(--green); }
.badge--red { background: rgba(237,66,69,0.15); color: var(--red); }

.token-textarea {
  width: 100%; min-height: 72px; background: var(--bg-input);
  border: 1px solid var(--border); border-radius: 6px;
  color: var(--text-primary); font-size: 13px; padding: 10px 12px;
  outline: none; resize: vertical; font-family: monospace;
}
.token-obscured { -webkit-text-security: disc; }
.token-textarea:focus { border-color: var(--accent); }

.token-row { display: flex; align-items: center; justify-content: space-between; gap: 8px; }
.checkbox-label { display: flex; align-items: center; gap: 6px; font-size: 13px; color: var(--text-secondary); cursor: pointer; }

.collapsible-btn {
  display: flex; align-items: center; gap: 6px;
  background: none; border: none; color: var(--accent);
  font-size: 13px; font-weight: 500; cursor: pointer; padding: 0;
}

.howto { display: flex; flex-direction: column; gap: 14px; }

.warning-box {
  background: rgba(254,231,92,0.1); border: 1px solid rgba(254,231,92,0.3);
  border-radius: 6px; padding: 10px 12px; font-size: 13px; color: var(--yellow);
  line-height: 1.5;
}

.method { display: flex; flex-direction: column; gap: 6px; }
.method-title { margin: 0; font-size: 13px; font-weight: 600; color: var(--text-primary); }
.method-steps { margin: 0; padding-left: 18px; font-size: 13px; color: var(--text-secondary); line-height: 1.6; }
.method-steps code { background: var(--bg-input); border-radius: 3px; padding: 1px 4px; font-size: 12px; color: var(--text-primary); }

.snippet-wrap { position: relative; }
.snippet {
  background: var(--bg-input); border: 1px solid var(--border); border-radius: 6px;
  padding: 10px 12px; padding-right: 60px; margin: 0;
  font-size: 11px; color: var(--text-primary); white-space: pre-wrap;
  word-break: break-all; line-height: 1.5; font-family: monospace;
  max-height: 100px; overflow-y: auto;
}
.copy-btn {
  position: absolute; top: 8px; right: 8px;
  background: var(--border); border: none; border-radius: 4px;
  color: var(--text-primary); font-size: 11px; font-weight: 600;
  padding: 3px 8px; cursor: pointer;
}
.copy-btn:hover { background: var(--accent); color: white; }

/* Presence fields */
.field { display: flex; flex-direction: column; gap: 6px; }
.field-label { font-size: 13px; font-weight: 500; color: var(--text-secondary); display: flex; align-items: center; gap: 6px; }
.field-label-row { display: flex; justify-content: space-between; align-items: center; }
.field-value { font-size: 13px; font-weight: 600; color: var(--accent); }
.advanced-tag {
  font-size: 10px; font-weight: 700; background: rgba(88,101,242,0.2);
  color: var(--accent); border-radius: 3px; padding: 1px 5px; letter-spacing: 0.04em;
}

.segmented { display: flex; gap: 0; border: 1px solid var(--border); border-radius: 6px; overflow: hidden; }
.seg-btn {
  flex: 1; background: transparent; border: none; color: var(--text-secondary);
  font-size: 13px; font-weight: 500; padding: 8px 0; cursor: pointer;
  transition: background 0.15s, color 0.15s;
}
.seg-btn + .seg-btn { border-left: 1px solid var(--border); }
.seg-btn--active { background: var(--accent); color: white; }

.range-input {
  width: 100%; accent-color: var(--accent); cursor: pointer;
  height: 4px;
}
.range-hints { display: flex; justify-content: space-between; font-size: 11px; color: var(--text-secondary); margin-top: 2px; }

.text-input {
  width: 100%; background: var(--bg-input); border: 1px solid var(--border);
  border-radius: 6px; color: var(--text-primary); font-size: 14px;
  padding: 10px 12px; outline: none;
}
.text-input:focus { border-color: var(--accent); }

.select-input {
  width: 100%; background: var(--bg-input); border: 1px solid var(--border);
  border-radius: 6px; color: var(--text-primary); font-size: 14px;
  padding: 10px 12px; outline: none; cursor: pointer;
  appearance: auto;
}
.select-input:focus { border-color: var(--accent); }

.logs-btn {
  display: flex; align-items: center; gap: 8px;
  align-self: flex-start;
}

.logs-block {
  background: var(--bg-input); border: 1px solid var(--border);
  border-radius: 6px; overflow: hidden;
}
.logs-toolbar {
  display: flex; align-items: center; justify-content: space-between;
  padding: 8px 12px; border-bottom: 1px solid var(--border);
}
.logs-title { font-size: 11px; color: var(--text-secondary); font-weight: 600; text-transform: uppercase; letter-spacing: 0.05em; }
.logs-toolbar .copy-btn { position: static; }

.logs-pre {
  margin: 0; padding: 12px; font-size: 11px; font-family: monospace;
  color: var(--text-secondary); line-height: 1.6;
  max-height: 280px; overflow-y: auto; white-space: pre-wrap; word-break: break-all;
}

/* Bottom section */
.btn { padding: 10px 20px; border-radius: 6px; border: none; font-size: 14px; font-weight: 600; cursor: pointer; transition: opacity 0.15s; }
.btn:disabled { opacity: 0.5; cursor: not-allowed; }
.btn--primary { background: var(--accent); color: white; }
.btn--primary:not(:disabled):hover { background: var(--accent-hover); }
.btn--secondary {
  background: var(--bg-input); color: var(--text-primary);
  border: 1px solid var(--border);
}

.save-btn { width: 100%; padding: 13px; font-size: 15px; }
.save-msg { margin: 0; text-align: center; font-size: 13px; color: var(--green); }
.save-msg--error { color: var(--red); }

kbd {
  background: var(--bg-input); border: 1px solid var(--border);
  border-radius: 3px; padding: 1px 5px; font-size: 11px;
  font-family: monospace; color: var(--text-primary);
}
</style>
