<script setup>
import { ref, computed, onMounted } from 'vue'
import { useGamesStore } from '../stores/games'
import { getInstalledApps, fetchPlayStoreMetadata, getIconDisplayUrl } from '../helpers/apps'
import ToggleSwitch from '../components/ToggleSwitch.vue'

const store = useGamesStore()

const search = ref('')
const editingPkg = ref(null)
const editingName = ref('')
const showAddForm = ref(false)
const newPkg = ref('')
const newName = ref('')
const newIcon = ref('')
const newAppId = ref('')
const addError = ref('')
const fetchingIcon = ref(false)

const installedApps = ref([])
const loadingApps = ref(false)
const appSearch = ref('')
const isManualInput = ref(false)
const selectedApp = ref(null)

onMounted(() => {
  if (!store.loaded) store.load()
})

const filteredGames = computed(() => {
  const q = search.value.toLowerCase()
  return Object.entries(store.games)
    .filter(([pkg, g]) =>
      !q || g.name.toLowerCase().includes(q) || pkg.toLowerCase().includes(q)
    )
    .sort((a, b) => a[1].name.localeCompare(b[1].name))
})

const filteredInstalledApps = computed(() => {
  const q = appSearch.value.toLowerCase().trim()
  if (!q) return installedApps.value
  return installedApps.value.filter(a =>
    a.appLabel.toLowerCase().includes(q) || a.packageName.toLowerCase().includes(q)
  )
})

async function openAddModal() {
  showAddForm.value = true
  addError.value = ''
  selectedApp.value = null
  isManualInput.value = false
  appSearch.value = ''
  newPkg.value = ''
  newName.value = ''
  newIcon.value = ''
  newAppId.value = ''
  if (installedApps.value.length === 0) {
    loadingApps.value = true
    try {
      installedApps.value = await getInstalledApps()
    } finally {
      loadingApps.value = false
    }
  }
}

async function selectInstalledApp(app) {
  selectedApp.value = app
  newPkg.value = app.packageName
  newName.value = app.appLabel
  newIcon.value = ''
  newAppId.value = ''
  fetchingIcon.value = true
  addError.value = ''
  try {
    const meta = await fetchPlayStoreMetadata(app.packageName)
    if (meta) {
      if (meta.title) newName.value = meta.title
      if (meta.icon) newIcon.value = meta.icon
      if (meta.appId) newAppId.value = meta.appId
    }
  } finally {
    fetchingIcon.value = false
  }
}

async function manualFetchIcon() {
  const pkg = newPkg.value.trim()
  if (!pkg) return
  fetchingIcon.value = true
  try {
    const meta = await fetchPlayStoreMetadata(pkg)
    if (meta) {
      if (meta.title && (!newName.value || newName.value === pkg.split('.').pop())) {
        newName.value = meta.title
      }
      if (meta.icon) newIcon.value = meta.icon
      if (meta.appId) newAppId.value = meta.appId
    }
  } finally {
    fetchingIcon.value = false
  }
}

function resetSelection() {
  selectedApp.value = null
  isManualInput.value = false
  newPkg.value = ''
  newName.value = ''
  newIcon.value = ''
  newAppId.value = ''
  addError.value = ''
}

function startEdit(pkg, name) {
  editingPkg.value = pkg
  editingName.value = name
}

async function commitEdit(pkg) {
  const name = editingName.value.trim()
  if (name) await store.updateName(pkg, name)
  editingPkg.value = null
}

function cancelEdit() {
  editingPkg.value = null
}

async function doAddGame() {
  const pkg = newPkg.value.trim()
  const name = newName.value.trim()
  const icon = newIcon.value.trim()
  addError.value = ''
  if (!pkg) { addError.value = 'Package name is required.'; return }
  if (!/^[a-zA-Z][a-zA-Z0-9_.]*$/.test(pkg)) { addError.value = 'Invalid package name format.'; return }
  if (store.games[pkg]) { addError.value = 'Package already exists in game list.'; return }
  await store.addGame(pkg, name || pkg.split('.').pop(), icon, newAppId.value)
  showAddForm.value = false
  resetSelection()
}

async function doRemove(pkg) {
  await store.removeGame(pkg)
}

async function fetchGameIcon(pkg) {
  const meta = await fetchPlayStoreMetadata(pkg)
  if (meta) {
    if (meta.appId) {
      await store.updateAppId(pkg, meta.appId)
      await store.updateIcon(pkg, '')
    } else if (meta.icon) {
      await store.updateIcon(pkg, meta.icon)
    }
    if (meta.title && (store.games[pkg]?.name === 'mho' || store.games[pkg]?.name === pkg.split('.').pop())) {
      await store.updateName(pkg, meta.title)
    }
  }
}

function iconError(e) {
  e.target.style.display = 'none'
  if (e.target.nextElementSibling) {
    e.target.nextElementSibling.style.display = 'flex'
  }
}

function pickerIconError(e) {
  e.target.style.display = 'none'
  if (e.target.nextElementSibling) {
    e.target.nextElementSibling.style.display = 'flex'
  }
}
</script>

<template>
  <div class="page">
    <div class="page-header">
      <h2>Game List</h2>
      <span class="game-count">{{ Object.keys(store.games).length }} games</span>
    </div>

    <!-- Search -->
    <div class="search-wrap">
      <svg class="search-icon" width="16" height="16" viewBox="0 0 24 24" fill="currentColor">
        <path d="M15.5 14h-.79l-.28-.27A6.471 6.471 0 0 0 16 9.5 6.5 6.5 0 1 0 9.5 16c1.61 0 3.09-.59 4.23-1.57l.27.28v.79l5 4.99L20.49 19l-4.99-5zm-6 0C7.01 14 5 11.99 5 9.5S7.01 5 9.5 5 14 7.01 14 9.5 11.99 14 9.5 14z"/>
      </svg>
      <input
        v-model="search"
        class="search-input"
        type="text"
        placeholder="Search by name or package…"
      />
    </div>

    <!-- Loading -->
    <div v-if="store.loading" class="empty-state">
      <span class="spinner" aria-hidden="true"></span>
      <p>Loading games…</p>
    </div>

    <!-- Empty -->
    <div v-else-if="filteredGames.length === 0 && !store.loading" class="empty-state">
      <span style="font-size:40px" class="empty-icon" aria-hidden="true">
        <svg width="40" height="40" viewBox="0 0 24 24" fill="currentColor"><path d="M15 7.5V2H9v5.5l3 3 3-3zM7.5 9H2v6h5.5l3-3-3-3zM9 16.5V22h6v-5.5l-3-3-3 3zM16.5 9l-3 3 3 3H22V9h-5.5z"/></svg>
      </span>
      <p>{{ search ? 'No games match your search.' : 'No games configured yet.' }}</p>
    </div>

    <!-- Game list -->
    <div v-else class="game-list">
      <div v-for="[pkg, game] in filteredGames" :key="pkg" class="game-item">
        <div class="game-icon-wrap">
          <img
            :src="getIconDisplayUrl(game.icon) || `ksu://icon/${pkg}`"
            :alt="game.name"
            class="game-icon"
            @error="iconError"
          />
          <span class="game-icon-fallback" aria-hidden="true">
        <svg width="22" height="22" viewBox="0 0 24 24" fill="currentColor"><path d="M15 7.5V2H9v5.5l3 3 3-3zM7.5 9H2v6h5.5l3-3-3-3zM9 16.5V22h6v-5.5l-3-3-3 3zM16.5 9l-3 3 3 3H22V9h-5.5z"/></svg>
      </span>
        </div>
        <div class="game-info">
          <!-- Inline name editor -->
          <div v-if="editingPkg === pkg" class="name-edit-row">
            <input
              v-model="editingName"
              class="name-input"
              type="text"
              @keyup.enter="commitEdit(pkg)"
              @keyup.escape="cancelEdit"
              @blur="commitEdit(pkg)"
              autofocus
            />
          </div>
          <div v-else class="game-name-row">
            <span class="game-name" @click="startEdit(pkg, game.name)">{{ game.name }}</span>
            <button class="edit-btn" @click="startEdit(pkg, game.name)" title="Rename">
              <svg width="13" height="13" viewBox="0 0 24 24" fill="currentColor">
                <path d="M3 17.25V21h3.75L17.81 9.94l-3.75-3.75L3 17.25zM20.71 7.04c.39-.39.39-1.02 0-1.41l-2.34-2.34c-.39-.39-1.02-.39-1.41 0l-1.83 1.83 3.75 3.75 1.83-1.83z"/>
              </svg>
            </button>
            <span v-if="game.icon || game.app_id" class="badge-icon" title="Discord Icon Linked" aria-hidden="true">
              <svg width="12" height="12" viewBox="0 0 24 24" fill="currentColor"><path d="M21 19V5c0-1.1-.9-2-2-2H5c-1.1 0-2 .9-2 2v14c0 1.1.9 2 2 2h14c1.1 0 2-.9 2-2zM8.5 13.5l2.5 3.01L14.5 12l4.5 6H5l3.5-4.5z"/></svg>
            </span>
            <button v-else class="btn-fetch-icon" @click="fetchGameIcon(pkg)" title="Fetch icon from Play Store">
              + Icon
            </button>
          </div>
          <span class="game-pkg">{{ pkg }}</span>
        </div>
        <div class="game-actions">
          <ToggleSwitch
            :model-value="game.enabled"
            @update:model-value="store.toggleGame(pkg)"
          />
          <button class="remove-btn" @click="doRemove(pkg)" title="Remove">
            <svg width="16" height="16" viewBox="0 0 24 24" fill="currentColor">
              <path d="M6 19c0 1.1.9 2 2 2h8c1.1 0 2-.9 2-2V7H6v12zM19 4h-3.5l-1-1h-5l-1 1H5v2h14V4z"/>
            </svg>
          </button>
        </div>
      </div>
    </div>

    <!-- Add game form / modal -->
    <div v-if="showAddForm" class="card add-form">
      <div class="add-form-header">
        <p class="section-label">Add Game</p>
        <button class="close-btn" @click="showAddForm = false; resetSelection()" aria-label="Close" title="Close">
        <svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round" aria-hidden="true">
          <line x1="18" y1="6" x2="6" y2="18" /><line x1="6" y1="6" x2="18" y2="18" />
        </svg>
      </button>
      </div>

      <!-- Mode 1: Searchable App Picker -->
      <div v-if="!selectedApp && !isManualInput" class="app-picker-section">
        <label class="field-label">Select Installed App</label>
        <div class="search-wrap" style="margin-bottom: 8px;">
          <svg class="search-icon" width="14" height="14" viewBox="0 0 24 24" fill="currentColor">
            <path d="M15.5 14h-.79l-.28-.27A6.471 6.471 0 0 0 16 9.5 6.5 6.5 0 1 0 9.5 16c1.61 0 3.09-.59 4.23-1.57l.27.28v.79l5 4.99L20.49 19l-4.99-5zm-6 0C7.01 14 5 11.99 5 9.5S7.01 5 9.5 5 14 7.01 14 9.5 11.99 14 9.5 14z"/>
          </svg>
          <input
            v-model="appSearch"
            class="search-input"
            type="text"
            placeholder="Search installed apps..."
          />
        </div>

        <div v-if="loadingApps" class="picker-loading">
          <span>Scanning installed apps…</span>
        </div>

        <div v-else-if="filteredInstalledApps.length === 0" class="picker-empty">
          <p>No non-system apps found.</p>
        </div>

        <div v-else class="app-picker-list">
          <div
            v-for="app in filteredInstalledApps"
            :key="app.packageName"
            class="app-picker-item"
            :class="{ 'app-picker-item--exists': store.games[app.packageName] }"
            @click="selectInstalledApp(app)"
          >
            <div class="picker-icon-wrap">
              <img
                :src="`ksu://icon/${app.packageName}`"
                :alt="app.appLabel"
                class="picker-icon"
                @error="pickerIconError"
              />
              <span class="picker-icon-fallback" aria-hidden="true">
              <svg width="18" height="18" viewBox="0 0 24 24" fill="currentColor"><path d="M17 1.01L7 1c-1.1 0-2 .9-2 2v18c0 1.1.9 2 2 2h10c1.1 0 2-.9 2-2V3c0-1.1-.9-1.99-2-1.99zM17 19H7V5h10v14z"/></svg>
            </span>
            </div>
            <div class="picker-info">
              <span class="picker-label">{{ app.appLabel }}</span>
              <span class="picker-pkg">{{ app.packageName }}</span>
            </div>
            <span v-if="store.games[app.packageName]" class="picker-added-badge">Added</span>
          </div>
        </div>

        <button class="manual-toggle-btn" @click="isManualInput = true">
          Enter package name manually instead
        </button>
      </div>

      <!-- Mode 2: App Selected or Manual Entry Configuration -->
      <div v-else class="selected-app-section">
        <div v-if="selectedApp" class="selected-banner">
          <div class="picker-icon-wrap">
            <img
              :src="getIconDisplayUrl(newIcon) || `ksu://icon/${newPkg}`"
              :alt="newName"
              class="picker-icon"
              @error="pickerIconError"
            />
            <span class="picker-icon-fallback" aria-hidden="true">
              <svg width="18" height="18" viewBox="0 0 24 24" fill="currentColor"><path d="M17 1.01L7 1c-1.1 0-2 .9-2 2v18c0 1.1.9 2 2 2h10c1.1 0 2-.9 2-2V3c0-1.1-.9-1.99-2-1.99zM17 19H7V5h10v14z"/></svg>
            </span>
          </div>
          <div class="selected-details">
            <span class="selected-name">{{ newName }}</span>
            <span class="selected-pkg">{{ newPkg }}</span>
          </div>
          <button class="change-app-btn" @click="resetSelection">Change</button>
        </div>

        <div v-if="isManualInput" class="manual-fields">
          <label class="field-label">Package Name</label>
          <div class="pkg-input-row">
            <input
              v-model="newPkg"
              class="text-input"
              type="text"
              placeholder="com.example.game"
              @blur="manualFetchIcon"
            />
            <button class="btn btn--small" @click="manualFetchIcon" :disabled="fetchingIcon">
              Fetch Icon
            </button>
          </div>
        </div>

        <label class="field-label" style="margin-top: 10px;">Display Name</label>
        <input
          v-model="newName"
          class="text-input"
          type="text"
          placeholder="Display Name in Discord"
          @keyup.enter="doAddGame"
        />

        <label class="field-label" style="margin-top: 10px;">
          Discord Icon URL
          <span v-if="fetchingIcon" class="fetching-label">Resolving Google Play icon…</span>
        </label>
        <input
          v-model="newIcon"
          class="text-input"
          type="text"
          placeholder="https://play-lh.googleusercontent.com/... (optional)"
          @keyup.enter="doAddGame"
        />

        <div v-if="newIcon" class="icon-preview-row">
          <img :src="getIconDisplayUrl(newIcon)" class="icon-preview" alt="Icon Preview" />
          <span class="icon-preview-hint">Icon preview from Discord CDN / Google Play</span>
        </div>

        <p v-if="addError" class="add-error">{{ addError }}</p>

        <div class="add-btn-row">
          <button class="btn btn--primary" @click="doAddGame">Add Game</button>
          <button class="btn btn--secondary" @click="resetSelection">
            {{ isManualInput ? 'Back to App List' : 'Cancel' }}
          </button>
        </div>
      </div>
    </div>

    <!-- Floating Action Button -->
    <button v-if="!showAddForm" class="fab" @click="openAddModal">
      <svg width="22" height="22" viewBox="0 0 24 24" fill="currentColor">
        <path d="M19 13h-6v6h-2v-6H5v-2h6V5h2v6h6v2z"/>
      </svg>
      Add Game
    </button>
  </div>
</template>

<style scoped>
.page { padding: 16px; display: flex; flex-direction: column; gap: 12px; }

.page-header { display: flex; align-items: center; justify-content: space-between; }
.page-header h2 { margin: 0; font-size: 20px; font-weight: 700; color: var(--text-primary); }
.game-count { font-size: 12px; color: var(--text-secondary); background: var(--bg-card); padding: 3px 10px; border-radius: 12px; }

.search-wrap {
  display: flex; align-items: center; gap: 8px;
  background: var(--bg-card); border-radius: 8px; padding: 0 12px;
  border: 1px solid var(--border);
}
.search-icon { color: var(--text-secondary); flex-shrink: 0; }
.search-input {
  flex: 1; background: transparent; border: none; outline: none;
  color: var(--text-primary); font-size: 14px; padding: 10px 0;
}
.search-input::placeholder { color: var(--text-secondary); }

.empty-state {
  display: flex; flex-direction: column; align-items: center;
  gap: 8px; padding: 48px 16px; color: var(--text-secondary); font-size: 14px;
}

.game-list { display: flex; flex-direction: column; gap: 2px; }

.game-item {
  display: flex; align-items: center; gap: 12px;
  background: var(--bg-card); border-radius: 8px; padding: 12px;
}

.game-icon-wrap { position: relative; width: 40px; height: 40px; flex-shrink: 0; }
.game-icon {
  width: 40px; height: 40px; border-radius: 8px; object-fit: cover;
  display: block;
}
.game-icon-fallback {
  display: none; position: absolute; inset: 0;
  align-items: center; justify-content: center;
  font-size: 22px; background: var(--bg-input); border-radius: 8px;
}

.game-info { flex: 1; min-width: 0; }
.game-name-row { display: flex; align-items: center; gap: 6px; }
.game-name {
  font-size: 15px; font-weight: 500; color: var(--text-primary);
  cursor: pointer; white-space: nowrap; overflow: hidden; text-overflow: ellipsis;
}
.game-name:hover { color: var(--accent); }
.edit-btn {
  background: none; border: none; color: var(--text-secondary);
  cursor: pointer; padding: 2px; display: flex; align-items: center;
  opacity: 0.5; flex-shrink: 0;
}
.edit-btn:hover { opacity: 1; color: var(--accent); }
.badge-icon { font-size: 12px; }
.btn-fetch-icon {
  background: var(--bg-input); border: 1px solid var(--border);
  color: var(--text-secondary); font-size: 10px; padding: 2px 6px;
  border-radius: 4px; cursor: pointer;
}
.btn-fetch-icon:hover { color: var(--accent); border-color: var(--accent); }
.game-pkg {
  display: block; font-size: 11px; color: var(--text-secondary);
  white-space: nowrap; overflow: hidden; text-overflow: ellipsis; margin-top: 2px;
}

.name-edit-row { display: flex; }
.name-input {
  width: 100%; background: var(--bg-input); border: 1px solid var(--accent);
  border-radius: 4px; color: var(--text-primary); font-size: 14px;
  padding: 4px 8px; outline: none;
}

.game-actions { display: flex; align-items: center; gap: 8px; flex-shrink: 0; }
.remove-btn {
  background: none; border: none; color: var(--text-secondary);
  cursor: pointer; padding: 4px; display: flex; align-items: center;
  border-radius: 4px;
}
.remove-btn:hover { color: var(--red); background: rgba(237,66,69,0.1); }

.card { background: var(--bg-card); border-radius: 8px; padding: 16px; border: 1px solid var(--border); }
.add-form { display: flex; flex-direction: column; }
.add-form-header { display: flex; justify-content: space-between; align-items: center; margin-bottom: 12px; }
.section-label { margin: 0; font-size: 12px; text-transform: uppercase; letter-spacing: 0.06em; color: var(--text-secondary); font-weight: 600; }
.close-btn { background: none; border: none; color: var(--text-secondary); font-size: 16px; cursor: pointer; }
.close-btn:hover { color: var(--text-primary); }

.field-label { font-size: 12px; color: var(--text-secondary); margin-bottom: 4px; font-weight: 500; display: flex; justify-content: space-between; }
.fetching-label { font-size: 11px; color: var(--accent); font-weight: normal; }

.text-input {
  width: 100%; background: var(--bg-input); border: 1px solid var(--border);
  border-radius: 6px; color: var(--text-primary); font-size: 14px;
  padding: 10px 12px; outline: none;
}
.text-input:focus { border-color: var(--accent); }

.pkg-input-row { display: flex; gap: 8px; }

/* App Picker */
.app-picker-section { display: flex; flex-direction: column; }
.picker-loading, .picker-empty {
  padding: 24px; text-align: center; color: var(--text-secondary); font-size: 13px;
}
.app-picker-list {
  max-height: 240px; overflow-y: auto; border: 1px solid var(--border);
  border-radius: 6px; background: var(--bg-input); display: flex; flex-direction: column;
}
.app-picker-item {
  display: flex; align-items: center; gap: 10px; padding: 8px 10px;
  cursor: pointer; border-bottom: 1px solid rgba(255,255,255,0.05);
  transition: background 0.15s;
}
.app-picker-item:last-child { border-bottom: none; }
.app-picker-item:hover { background: rgba(88, 101, 242, 0.12); }
.app-picker-item--exists { opacity: 0.6; }
.picker-icon-wrap { position: relative; width: 32px; height: 32px; flex-shrink: 0; }
.picker-icon { width: 32px; height: 32px; border-radius: 6px; object-fit: cover; display: block; }
.picker-icon-fallback {
  display: none; position: absolute; inset: 0; align-items: center; justify-content: center;
  font-size: 18px; background: var(--bg-card); border-radius: 6px;
}
.picker-info { flex: 1; min-width: 0; }
.picker-label { font-size: 13px; font-weight: 500; color: var(--text-primary); display: block; white-space: nowrap; overflow: hidden; text-overflow: ellipsis; }
.picker-pkg { font-size: 11px; color: var(--text-secondary); display: block; white-space: nowrap; overflow: hidden; text-overflow: ellipsis; }
.picker-added-badge { font-size: 10px; background: rgba(255,255,255,0.1); color: var(--text-secondary); padding: 2px 6px; border-radius: 4px; }

.manual-toggle-btn {
  margin-top: 10px; background: none; border: none; color: var(--accent);
  font-size: 12px; cursor: pointer; text-align: left; padding: 4px 0;
}
.manual-toggle-btn:hover { text-decoration: underline; }

/* Selected App Banner */
.selected-app-section { display: flex; flex-direction: column; }
.selected-banner {
  display: flex; align-items: center; gap: 12px; background: var(--bg-input);
  padding: 10px; border-radius: 6px; border: 1px solid var(--border); margin-bottom: 12px;
}
.selected-details { flex: 1; min-width: 0; }
.selected-name { font-size: 14px; font-weight: 600; color: var(--text-primary); display: block; white-space: nowrap; overflow: hidden; text-overflow: ellipsis; }
.selected-pkg { font-size: 11px; color: var(--text-secondary); display: block; white-space: nowrap; overflow: hidden; text-overflow: ellipsis; }
.change-app-btn {
  background: var(--bg-card); border: 1px solid var(--border); color: var(--text-secondary);
  font-size: 12px; padding: 4px 10px; border-radius: 4px; cursor: pointer;
}
.change-app-btn:hover { color: var(--text-primary); border-color: var(--accent); }

.icon-preview-row { display: flex; align-items: center; gap: 8px; margin-top: 8px; }
.icon-preview { width: 36px; height: 36px; border-radius: 6px; object-fit: cover; border: 1px solid var(--border); }
.icon-preview-hint { font-size: 11px; color: var(--text-secondary); }

.add-error { margin: 8px 0 0; font-size: 12px; color: var(--red); }
.add-btn-row { display: flex; gap: 8px; margin-top: 14px; }

.btn { padding: 10px 20px; border-radius: 6px; border: none; font-size: 14px; font-weight: 600; cursor: pointer; transition: opacity 0.15s; }
.btn--primary { background: var(--accent); color: white; }
.btn--primary:hover { background: var(--accent-hover); }
.btn--secondary { background: var(--bg-input); color: var(--text-primary); border: 1px solid var(--border); }
.btn--small { padding: 8px 14px; font-size: 12px; background: var(--bg-input); color: var(--text-primary); border: 1px solid var(--border); white-space: nowrap; }
.btn--small:hover { border-color: var(--accent); }

.fab {
  display: flex; align-items: center; justify-content: center; gap: 8px;
  background: var(--accent); color: white; border: none; border-radius: 8px;
  padding: 12px 20px; font-size: 14px; font-weight: 600; cursor: pointer;
  transition: background 0.15s;
}
.fab:hover { background: var(--accent-hover); }

/* Loading spinner (no emoji) */
.spinner {
  width: 24px; height: 24px;
  border: 3px solid var(--border);
  border-top-color: var(--accent);
  border-radius: 50%;
  display: inline-block;
  animation: spin 0.8s linear infinite;
}
@keyframes spin { to { transform: rotate(360deg); } }
</style>
