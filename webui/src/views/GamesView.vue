<script setup>
import { ref, computed, onMounted } from 'vue'
import { useGamesStore } from '../stores/games'
import { getInstalledApps, fetchPlayStoreMetadata, getIconDisplayUrl } from '../helpers/apps'
import ToggleSwitch from '../components/ToggleSwitch.vue'
import Icon from '../components/Icon.vue'
import GameIcon from '../components/GameIcon.vue'

const store = useGamesStore()

const search = ref('')
const editingPkg = ref(null)
const editingName = ref('')
const confirmPkg = ref(null)
let confirmTimer = null

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

const vFocus = { mounted: el => el.focus() }

onMounted(() => { if (!store.loaded) store.load() })

const filteredGames = computed(() => {
  const q = search.value.toLowerCase()
  return Object.entries(store.games)
    .filter(([pkg, g]) => !q || g.name.toLowerCase().includes(q) || pkg.toLowerCase().includes(q))
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
  resetSelection()
  appSearch.value = ''
  if (installedApps.value.length === 0) {
    loadingApps.value = true
    try { installedApps.value = await getInstalledApps() } finally { loadingApps.value = false }
  }
}

function closeSheet() {
  showAddForm.value = false
  resetSelection()
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
      if (meta.title && (!newName.value || newName.value === pkg.split('.').pop())) newName.value = meta.title
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
  if (editingPkg.value !== pkg) return // Enter + blur both fire; only commit once
  editingPkg.value = null
  const name = editingName.value.trim()
  if (name) await store.updateName(pkg, name)
}

function cancelEdit() { editingPkg.value = null }

async function doAddGame() {
  const pkg = newPkg.value.trim()
  const name = newName.value.trim()
  const icon = newIcon.value.trim()
  addError.value = ''
  if (!pkg) { addError.value = 'Package name is required.'; return }
  if (!/^[a-zA-Z][a-zA-Z0-9_.]*$/.test(pkg)) { addError.value = 'Invalid package name format.'; return }
  if (store.games[pkg]) { addError.value = 'Package already exists in game list.'; return }
  await store.addGame(pkg, name || pkg.split('.').pop(), icon, newAppId.value)
  closeSheet()
}

// Two-tap delete: first tap arms the button, second tap removes
async function askRemove(pkg) {
  if (confirmPkg.value === pkg) {
    clearTimeout(confirmTimer)
    confirmPkg.value = null
    await store.removeGame(pkg)
    return
  }
  confirmPkg.value = pkg
  clearTimeout(confirmTimer)
  confirmTimer = setTimeout(() => { confirmPkg.value = null }, 3000)
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
</script>

<template>
  <div class="page">
    <div class="toolbar">
      <div class="search">
        <Icon name="search" :size="17" />
        <input v-model="search" type="text" placeholder="Search name or package…" />
        <button v-if="search" class="icon-btn clear" aria-label="Clear" @click="search = ''"><Icon name="x" :size="15" /></button>
      </div>

    </div>

    <div v-if="store.loading" class="empty"><span class="spinner" /><p>Loading games…</p></div>

    <div v-else-if="filteredGames.length === 0" class="empty">
      <span class="empty-ico"><Icon name="gamepad" :size="28" /></span>
      <p>{{ search ? 'No games match your search.' : 'No games here yet.' }}</p>
    </div>

    <div v-else class="list">
      <article v-for="[pkg, game] in filteredGames" :key="pkg" class="game" :class="{ off: !game.enabled }">
        <GameIcon :pkg="pkg" :icon="game.icon" :name="game.name" :size="46" />

        <div class="main">
          <input
            v-if="editingPkg === pkg" v-model="editingName" v-focus class="input rename" type="text"
            @keyup.enter="commitEdit(pkg)" @keyup.escape="cancelEdit" @blur="commitEdit(pkg)"
          />
          <button v-else class="name" @click="startEdit(pkg, game.name)" title="Tap to rename">
            <span>{{ game.name }}</span><Icon name="pencil" :size="12" />
          </button>
          <span class="pkg">{{ pkg }}</span>
          <span v-if="game.icon || game.app_id" class="tag tag--ok"><Icon name="image" :size="11" /> Icon linked</span>
          <button v-else class="tag tag--action" @click="fetchGameIcon(pkg)"><Icon name="plus" :size="11" :stroke="2.4" /> Fetch icon</button>
        </div>

        <div class="side">
          <ToggleSwitch :model-value="game.enabled" @update:model-value="store.toggleGame(pkg)" />
          <button class="icon-btn" :class="{ 'icon-btn--danger': confirmPkg === pkg }"
                  :title="confirmPkg === pkg ? 'Tap again to remove' : 'Remove'" @click="askRemove(pkg)">
            <Icon name="trash" :size="16" />
          </button>
        </div>
      </article>
    </div>

    <button v-if="!showAddForm" class="fab" @click="openAddModal">
      <Icon name="plus" :size="20" :stroke="2.4" /> Add game
    </button>

    <!-- Add game bottom sheet -->
    <Transition name="sheet">
      <div v-if="showAddForm" class="sheet-root">
        <div class="backdrop" @click="closeSheet" />
        <div class="sheet" role="dialog" aria-modal="true" aria-label="Add game">
          <div class="grab" />
          <div class="row sheet-head">
            <h3>Add game</h3>
            <button class="icon-btn" aria-label="Close" @click="closeSheet"><Icon name="x" :size="20" /></button>
          </div>

          <!-- Step 1: pick an installed app -->
          <div v-if="!selectedApp && !isManualInput" class="stack">
            <div class="search">
              <Icon name="search" :size="16" />
              <input v-model="appSearch" type="text" placeholder="Search installed apps…" />
            </div>

            <div v-if="loadingApps" class="empty sm"><span class="spinner" /><p>Scanning installed apps…</p></div>
            <div v-else-if="filteredInstalledApps.length === 0" class="empty sm"><p>No non-system apps found.</p></div>
            <div v-else class="picker">
              <button v-for="app in filteredInstalledApps" :key="app.packageName" class="pick"
                      :class="{ exists: store.games[app.packageName] }" @click="selectInstalledApp(app)">
                <GameIcon :pkg="app.packageName" :name="app.appLabel" :size="38" />
                <span class="pick-info"><b>{{ app.appLabel }}</b><small>{{ app.packageName }}</small></span>
                <span v-if="store.games[app.packageName]" class="tag">Added</span>
                <Icon v-else name="chevron" :size="16" />
              </button>
            </div>

            <button class="link" @click="isManualInput = true">Enter package name manually</button>
          </div>

          <!-- Step 2: details -->
          <div v-else class="stack">
            <div v-if="selectedApp" class="chosen">
              <GameIcon :pkg="newPkg" :icon="newIcon" :name="newName" :size="42" />
              <span class="pick-info"><b>{{ newName }}</b><small>{{ newPkg }}</small></span>
              <button class="btn btn--ghost btn--sm" @click="resetSelection">Change</button>
            </div>

            <div v-if="isManualInput" class="field">
              <label class="field-label">Package name</label>
              <div class="row">
                <input v-model="newPkg" class="input" type="text" placeholder="com.example.game" @blur="manualFetchIcon" />
                <button class="btn btn--ghost btn--sm" :disabled="fetchingIcon" @click="manualFetchIcon">Fetch</button>
              </div>
            </div>

            <div class="field">
              <label class="field-label">Display name</label>
              <input v-model="newName" class="input" type="text" placeholder="Name shown in Discord" @keyup.enter="doAddGame" />
            </div>

            <div class="field">
              <label class="field-label">
                Icon URL <span v-if="fetchingIcon" class="fetching">Resolving Play Store icon…</span>
              </label>
              <input v-model="newIcon" class="input" type="text" placeholder="https://play-lh.googleusercontent.com/… (optional)" @keyup.enter="doAddGame" />
              <div v-if="newIcon" class="preview">
                <img :src="getIconDisplayUrl(newIcon)" alt="Icon preview" />
                <span>Preview from Discord CDN / Google Play</span>
              </div>
            </div>

            <p v-if="addError" class="error">{{ addError }}</p>

            <div class="row">
              <button class="btn btn--ghost" @click="resetSelection">{{ isManualInput ? 'Back' : 'Cancel' }}</button>
              <button class="btn btn--primary grow" @click="doAddGame">Add game</button>
            </div>
          </div>
        </div>
      </div>
    </Transition>
  </div>
</template>

<style scoped>
.toolbar {
  display: flex; flex-direction: column; gap: 10px;
  position: sticky; z-index: 40; top: calc(env(safe-area-inset-top, 0px) + 60px);
  padding-bottom: 8px; background: linear-gradient(var(--bg) 80%, transparent);
}

.search {
  display: flex; align-items: center; gap: 10px; height: 46px; padding: 0 12px;
  background: var(--surface); border: 1px solid var(--border); border-radius: 14px; color: var(--muted);
}
.search:focus-within { border-color: var(--accent); box-shadow: 0 0 0 3px var(--accent-soft); }
.search input { flex: 1; min-width: 0; background: transparent; border: 0; outline: 0; color: var(--text); font-size: 14px; }
.search input::placeholder { color: var(--muted); }
.clear { width: 28px; height: 28px; }

.list { display: flex; flex-direction: column; gap: 10px; }
.game {
  display: flex; align-items: center; gap: 12px; padding: 12px;
  background: var(--surface); border: 1px solid var(--border); border-radius: 18px;
  content-visibility: auto; contain-intrinsic-size: auto 92px;
  transition: opacity 0.2s;
}
.game.off { opacity: 0.55; }
.main { flex: 1; min-width: 0; display: flex; flex-direction: column; align-items: flex-start; gap: 3px; }
.name {
  display: inline-flex; align-items: center; gap: 6px; max-width: 100%; padding: 0; border: 0; background: none;
  color: var(--text); font-size: 15px; font-weight: 650; cursor: pointer; text-align: left;
}
.name span { display: -webkit-box; -webkit-line-clamp: 2; -webkit-box-orient: vertical; overflow: hidden; line-height: 1.25; }
.name svg { color: var(--muted); opacity: 0.6; flex-shrink: 0; }
.rename { height: 34px; font-size: 14px; border-color: var(--accent); }
.pkg { max-width: 100%; font-size: 11px; color: var(--muted); overflow: hidden; text-overflow: ellipsis; white-space: nowrap; }
.side { display: flex; flex-direction: column; align-items: center; gap: 6px; flex-shrink: 0; }

.tag {
  display: inline-flex; align-items: center; gap: 4px; margin-top: 3px; height: 22px; padding: 0 8px;
  border: 0; border-radius: 99px; font-size: 10.5px; font-weight: 700; background: var(--surface-2); color: var(--muted);
}
.tag--ok { background: var(--green-soft); color: var(--green); }
.tag--action { background: var(--accent-soft); color: var(--accent); cursor: pointer; }

.empty { display: flex; flex-direction: column; align-items: center; gap: 12px; padding: 56px 16px; color: var(--muted); font-size: 14px; }
.empty.sm { padding: 28px 16px; }
.empty-ico { width: 60px; height: 60px; border-radius: 20px; display: grid; place-items: center; background: var(--surface-2); }

.fab {
  position: fixed; z-index: 90; right: 16px; bottom: calc(92px + env(safe-area-inset-bottom, 0px));
  height: 50px; padding: 0 20px 0 16px; border: 0; border-radius: 99px;
  display: flex; align-items: center; gap: 8px; color: #fff; font-size: 14px; font-weight: 700; cursor: pointer;
  background: var(--grad); box-shadow: 0 14px 30px -10px rgba(88, 101, 242, 0.95);
  transition: transform 0.12s;
}
.fab:active { transform: scale(0.95); }

/* Bottom sheet */
.sheet-root { position: fixed; inset: 0; z-index: 200; display: flex; align-items: flex-end; }
.backdrop { position: absolute; inset: 0; background: rgba(0, 0, 0, 0.6); }
.sheet {
  position: relative; width: 100%; max-height: 88vh; overflow-y: auto;
  padding: 10px 18px calc(20px + env(safe-area-inset-bottom, 0px));
  background: var(--bg-2); border: 1px solid var(--border-strong); border-bottom: 0;
  border-radius: 26px 26px 0 0; display: flex; flex-direction: column; gap: 14px;
}
.grab { width: 38px; height: 4px; border-radius: 4px; background: var(--border-strong); margin: 0 auto; }
.sheet-head h3 { font-size: 17px; font-weight: 750; }
.sheet-enter-active, .sheet-leave-active { transition: opacity 0.28s ease; }
.sheet-enter-active .sheet, .sheet-leave-active .sheet { transition: transform 0.28s cubic-bezier(0.2, 0.8, 0.2, 1); }
.sheet-enter-from, .sheet-leave-to { opacity: 0; }
.sheet-enter-from .sheet, .sheet-leave-to .sheet { transform: translateY(100%); }

.picker { max-height: 320px; overflow-y: auto; display: flex; flex-direction: column; gap: 4px; margin: 0 -6px; padding: 0 6px; }
.pick {
  display: flex; align-items: center; gap: 12px; padding: 8px; border: 0; border-radius: 14px;
  background: transparent; color: var(--muted); text-align: left; cursor: pointer;
}
.pick:active { background: var(--accent-soft); }
.pick.exists { opacity: 0.5; }
.pick-info { flex: 1; min-width: 0; display: flex; flex-direction: column; gap: 2px; }
.pick-info b { font-size: 14px; font-weight: 600; color: var(--text); overflow: hidden; text-overflow: ellipsis; white-space: nowrap; }
.pick-info small { font-size: 11px; color: var(--muted); overflow: hidden; text-overflow: ellipsis; white-space: nowrap; }
.link { align-self: flex-start; padding: 4px 0; border: 0; background: none; color: var(--accent); font-size: 13px; font-weight: 600; cursor: pointer; }

.chosen { display: flex; align-items: center; gap: 12px; padding: 10px; border-radius: 16px; background: var(--surface-2); border: 1px solid var(--border); }
.fetching { color: var(--accent); font-weight: 500; }
.preview { display: flex; align-items: center; gap: 10px; font-size: 11px; color: var(--muted); }
.preview img { width: 38px; height: 38px; border-radius: 10px; object-fit: cover; border: 1px solid var(--border); }
.error { font-size: 12px; color: var(--red); }
.grow { flex: 1; }
</style>
