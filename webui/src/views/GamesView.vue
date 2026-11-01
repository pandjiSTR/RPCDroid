<script setup>
import { ref, computed, onMounted } from 'vue'
import { useGamesStore } from '../stores/games'
import { getInstalledApps, fetchPlayStoreMetadata, getIconDisplayUrl } from '../helpers/apps'
import ToggleSwitch from '../components/ToggleSwitch.vue'
import Icon from '../components/Icon.vue'
import GameIcon from '../components/GameIcon.vue'

const store = useGamesStore()

const search = ref('')
const filter = ref('all') // all | on | off
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
    .filter(([pkg, g]) => {
      if (filter.value === 'on' && !g.enabled) return false
      if (filter.value === 'off' && g.enabled) return false
      return !q || g.name.toLowerCase().includes(q) || pkg.toLowerCase().includes(q)
    })
    .sort((a, b) => a[1].name.localeCompare(b[1].name))
})

const filteredInstalledApps = computed(() => {
  const q = appSearch.value.toLowerCase().trim()
  if (!q) return installedApps.value
  return installedApps.value.filter(a =>
    a.appLabel.toLowerCase().includes(q) || a.packageName.toLowerCase().includes(q)
  )
})

const enabledCount = computed(() => Object.values(store.games).filter(g => g.enabled).length)

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
      <div class="searchbar">
        <Icon name="search" />
        <input v-model="search" type="text" placeholder="Search games" />
        <button v-if="search" class="icon-btn" aria-label="Clear" @click="search = ''"><Icon name="close" /></button>
      </div>
      <div class="chips">
        <button class="chip" :class="{ on: filter === 'all' }" @click="filter = 'all'">
          <Icon v-if="filter === 'all'" name="check" :size="18" />All {{ Object.keys(store.games).length }}
        </button>
        <button class="chip" :class="{ on: filter === 'on' }" @click="filter = 'on'">
          <Icon v-if="filter === 'on'" name="check" :size="18" />Enabled {{ enabledCount }}
        </button>
        <button class="chip" :class="{ on: filter === 'off' }" @click="filter = 'off'">
          <Icon v-if="filter === 'off'" name="check" :size="18" />Disabled
        </button>
      </div>
    </div>

    <div v-if="store.loading" class="empty"><span class="spinner" /><p class="t-body-m">Loading games…</p></div>

    <div v-else-if="filteredGames.length === 0" class="empty">
      <span class="empty-ico"><Icon name="sports_esports" :size="32" /></span>
      <p class="t-title-m">{{ search ? 'No results' : 'No games yet' }}</p>
      <p class="t-body-m variant">{{ search ? 'Try a different name or package.' : 'Tap Add game to get started.' }}</p>
    </div>

    <div v-else class="list">
      <article v-for="[pkg, game] in filteredGames" :key="pkg" class="game" :class="{ off: !game.enabled }">
        <GameIcon :pkg="pkg" :icon="game.icon" :name="game.name" :size="48" />

        <div class="main">
          <div v-if="editingPkg === pkg" class="tf rename">
            <input v-model="editingName" v-focus type="text" @keyup.enter="commitEdit(pkg)" @keyup.escape="cancelEdit" @blur="commitEdit(pkg)" />
          </div>
          <button v-else class="name t-title-m no-ripple" @click="startEdit(pkg, game.name)" title="Tap to rename">
            <span>{{ game.name }}</span>
          </button>
          <span class="pkg t-body-s variant">{{ pkg }}</span>
          <span v-if="game.icon || game.app_id" class="tag tag--ok"><Icon name="image" :size="14" /> Icon linked</span>
          <button v-else class="tag tag--action" @click="fetchGameIcon(pkg)"><Icon name="add" :size="14" /> Fetch icon</button>
        </div>

        <div class="side">
          <ToggleSwitch :model-value="game.enabled" @update:model-value="store.toggleGame(pkg)" />
          <button class="icon-btn" :class="{ 'icon-btn--tonal': confirmPkg === pkg }"
                  :aria-label="confirmPkg === pkg ? 'Tap again to remove' : 'Remove'" @click="askRemove(pkg)">
            <Icon name="delete" :size="22" />
          </button>
        </div>
      </article>
    </div>

    <Transition name="fab">
      <button v-if="!showAddForm" class="fab" @click="openAddModal">
        <Icon name="add" /> <span class="t-label-l">Add game</span>
      </button>
    </Transition>

    <!-- Modal bottom sheet -->
    <Transition name="sheet">
      <div v-if="showAddForm" class="sheet-root">
        <div class="scrim" @click="closeSheet" />
        <div class="sheet" role="dialog" aria-modal="true" aria-label="Add game">
          <div class="grab" />
          <h3 class="t-title-l sheet-title">Add game</h3>

          <!-- Step 1: pick an installed app -->
          <div v-if="!selectedApp && !isManualInput" class="stack">
            <div class="searchbar searchbar--sheet">
              <Icon name="search" />
              <input v-model="appSearch" type="text" placeholder="Search installed apps" />
            </div>

            <div v-if="loadingApps" class="empty sm"><span class="spinner" /><p class="t-body-m">Scanning installed apps…</p></div>
            <div v-else-if="filteredInstalledApps.length === 0" class="empty sm"><p class="t-body-m">No non-system apps found.</p></div>
            <div v-else class="picker">
              <button v-for="app in filteredInstalledApps" :key="app.packageName" class="pick"
                      :class="{ exists: store.games[app.packageName] }" @click="selectInstalledApp(app)">
                <GameIcon :pkg="app.packageName" :name="app.appLabel" :size="40" />
                <span class="pick-info"><b class="t-body-l">{{ app.appLabel }}</b><small class="t-body-s variant">{{ app.packageName }}</small></span>
                <span v-if="store.games[app.packageName]" class="tag">Added</span>
                <Icon v-else name="chevron_right" />
              </button>
            </div>

            <button class="btn" @click="isManualInput = true">Enter package name manually</button>
          </div>

          <!-- Step 2: details -->
          <div v-else class="stack">
            <div v-if="selectedApp" class="chosen">
              <GameIcon :pkg="newPkg" :icon="newIcon" :name="newName" :size="44" />
              <span class="pick-info"><b class="t-title-m">{{ newName }}</b><small class="t-body-s variant">{{ newPkg }}</small></span>
              <button class="btn" @click="resetSelection">Change</button>
            </div>

            <div v-if="isManualInput" class="row">
              <div class="tf grow">
                <label>Package name</label>
                <input v-model="newPkg" type="text" placeholder="com.example.game" @blur="manualFetchIcon" />
              </div>
              <button class="btn btn--tonal" :disabled="fetchingIcon" @click="manualFetchIcon">Fetch</button>
            </div>

            <div class="tf">
              <label>Display name</label>
              <input v-model="newName" type="text" placeholder="Name shown in Discord" @keyup.enter="doAddGame" />
            </div>

            <div class="tf">
              <label>Icon URL <span v-if="fetchingIcon" class="fetching">Resolving Play Store icon…</span></label>
              <input v-model="newIcon" type="text" placeholder="https://play-lh.googleusercontent.com/… (optional)" @keyup.enter="doAddGame" />
            </div>
            <div v-if="newIcon" class="preview">
              <img :src="getIconDisplayUrl(newIcon)" alt="Icon preview" />
              <span class="t-body-s variant">Preview from Discord CDN / Google Play</span>
            </div>

            <p v-if="addError" class="error t-body-s">{{ addError }}</p>

            <div class="row end">
              <button class="btn" @click="resetSelection">{{ isManualInput ? 'Back' : 'Cancel' }}</button>
              <button class="btn btn--filled" @click="doAddGame">Add game</button>
            </div>
          </div>
        </div>
      </div>
    </Transition>
  </div>
</template>

<style scoped>
.toolbar {
  position: sticky; z-index: 40; top: calc(64px + env(safe-area-inset-top, 0px));
  display: flex; flex-direction: column; gap: 12px; padding: 4px 0 8px; margin-top: -4px;
  background: var(--md-surface);
}
.searchbar svg { flex-shrink: 0; }

.list { display: flex; flex-direction: column; gap: 8px; }
.game {
  display: flex; align-items: center; gap: 16px; padding: 12px 16px; border-radius: var(--shape-l);
  background: var(--md-surface-container-low);
  content-visibility: auto; contain-intrinsic-size: auto 96px; transition: opacity .2s;
}
.game.off { opacity: .6; }
.main { flex: 1; min-width: 0; display: flex; flex-direction: column; align-items: flex-start; gap: 2px; }
.name { padding: 0; border: 0; background: none; color: var(--md-on-surface); text-align: left; cursor: pointer; max-width: 100%; }
.name span { display: -webkit-box; -webkit-line-clamp: 2; -webkit-box-orient: vertical; overflow: hidden; }
.rename { width: 100%; padding: 4px 12px 3px; }
.rename input { margin-top: 0; font-size: 16px; }
.pkg { max-width: 100%; overflow: hidden; text-overflow: ellipsis; white-space: nowrap; }
.main .tag { margin-top: 6px; }
.side { display: flex; flex-direction: column; align-items: center; gap: 4px; flex-shrink: 0; }

.empty { display: flex; flex-direction: column; align-items: center; gap: 8px; padding: 56px 16px; color: var(--md-on-surface-variant); text-align: center; }
.empty.sm { padding: 24px 16px; }
.empty-ico { width: 72px; height: 72px; border-radius: 24px; display: grid; place-items: center; margin-bottom: 8px; background: var(--md-secondary-container); color: var(--md-on-secondary-container); }
.empty .t-title-m { color: var(--md-on-surface); }

/* Extended FAB */
.fab {
  position: fixed; z-index: 90; right: 16px; bottom: calc(96px + env(safe-area-inset-bottom, 0px));
  height: 56px; padding: 0 20px 0 16px; border: 0; border-radius: var(--shape-l); cursor: pointer;
  display: flex; align-items: center; gap: 12px;
  background: var(--md-primary-container); color: var(--md-on-primary-container); box-shadow: var(--elev-3);
}
.fab-enter-active, .fab-leave-active { transition: all .2s var(--ease); }
.fab-enter-from, .fab-leave-to { opacity: 0; transform: scale(.8); }

/* Modal bottom sheet */
.sheet-root { position: fixed; inset: 0; z-index: 200; display: flex; align-items: flex-end; }
.scrim { position: absolute; inset: 0; background: rgba(0, 0, 0, .32); }
.sheet {
  position: relative; width: 100%; max-height: 90vh; overflow-y: auto;
  padding: 12px 24px calc(24px + env(safe-area-inset-bottom, 0px)); border-radius: var(--shape-xl) var(--shape-xl) 0 0;
  background: var(--md-surface-container-low); display: flex; flex-direction: column; gap: 16px;
}
.grab { width: 32px; height: 4px; border-radius: 2px; margin: 0 auto; background: var(--md-on-surface-variant); opacity: .4; }
.sheet-enter-active, .sheet-leave-active { transition: opacity .3s var(--ease); }
.sheet-enter-active .sheet, .sheet-leave-active .sheet { transition: transform .3s var(--ease); }
.sheet-enter-from, .sheet-leave-to { opacity: 0; }
.sheet-enter-from .sheet, .sheet-leave-to .sheet { transform: translateY(100%); }
.searchbar--sheet { background: var(--md-surface-container-high); }

.picker { max-height: 320px; overflow-y: auto; display: flex; flex-direction: column; margin: 0 -12px; }
.pick {
  display: flex; align-items: center; gap: 16px; padding: 8px 12px; border: 0; border-radius: var(--shape-m);
  background: transparent; color: var(--md-on-surface-variant); text-align: left; cursor: pointer;
}
.pick.exists { opacity: .5; }
.pick-info { flex: 1; min-width: 0; display: flex; flex-direction: column; }
.pick-info b { font-weight: 400; color: var(--md-on-surface); overflow: hidden; text-overflow: ellipsis; white-space: nowrap; }
.pick-info small { overflow: hidden; text-overflow: ellipsis; white-space: nowrap; }

.chosen { display: flex; align-items: center; gap: 12px; padding: 12px; border-radius: var(--shape-l); background: var(--md-surface-container-highest); }
.fetching { color: var(--md-primary); }
.preview { display: flex; align-items: center; gap: 12px; }
.preview img { width: 40px; height: 40px; border-radius: 12px; object-fit: cover; }
.error { color: var(--md-error); }
.row.end { justify-content: flex-end; }
</style>
