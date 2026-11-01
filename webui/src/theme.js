// Material You (M3) theming. Generates a full tonal scheme from a seed colour
// using Google's material-color-utilities and exposes it as CSS variables.
import { ref, watch } from 'vue'
import { argbFromHex, hexFromArgb, Hct, SchemeTonalSpot, MaterialDynamicColors as M } from '@material/material-color-utilities'

export const SEEDS = [
  { name: 'Blurple', hex: '#5865F2' },
  { name: 'Violet', hex: '#6750A4' },
  { name: 'Teal', hex: '#00897B' },
  { name: 'Green', hex: '#4C9A52' },
  { name: 'Orange', hex: '#E8710A' },
  { name: 'Rose', hex: '#D81B60' }
]
export const MODES = ['auto', 'light', 'dark']

const KEY_SEED = 'rpcdroid.seed'
const KEY_MODE = 'rpcdroid.mode'

function load(key, fallback) {
  try { return localStorage.getItem(key) || fallback } catch { return fallback }
}
function save(key, v) {
  try { localStorage.setItem(key, v) } catch { /* storage unavailable */ }
}

export const seed = ref(load(KEY_SEED, SEEDS[0].hex))
export const mode = ref(load(KEY_MODE, 'auto'))

const mql = window.matchMedia('(prefers-color-scheme: dark)')
const kebab = s => s.replace(/[A-Z]/g, c => '-' + c.toLowerCase())

// Roles exported as --md-<kebab-case> CSS variables
const ROLES = [
  'primary', 'onPrimary', 'primaryContainer', 'onPrimaryContainer',
  'secondary', 'onSecondary', 'secondaryContainer', 'onSecondaryContainer',
  'tertiary', 'onTertiary', 'tertiaryContainer', 'onTertiaryContainer',
  'error', 'onError', 'errorContainer', 'onErrorContainer',
  'surface', 'onSurface', 'surfaceVariant', 'onSurfaceVariant',
  'surfaceContainerLowest', 'surfaceContainerLow', 'surfaceContainer', 'surfaceContainerHigh', 'surfaceContainerHighest',
  'outline', 'outlineVariant', 'inverseSurface', 'inverseOnSurface', 'inversePrimary'
]

export function apply() {
  const dark = mode.value === 'dark' || (mode.value === 'auto' && mql.matches)
  // Tonal Spot = the default Android 12+ dynamic colour scheme (muted, controlled chroma)
  const scheme = new SchemeTonalSpot(Hct.fromInt(argbFromHex(seed.value)), dark, 0)
  const root = document.documentElement.style

  for (const role of ROLES) {
    const c = M[role]
    if (c?.getArgb) root.setProperty(`--md-${kebab(role)}`, hexFromArgb(c.getArgb(scheme)))
  }

  root.colorScheme = dark ? 'dark' : 'light'
  const meta = document.querySelector('meta[name="theme-color"]')
  if (meta) meta.setAttribute('content', hexFromArgb(M.surface.getArgb(scheme)))
}

export function setSeed(hex) { seed.value = hex; save(KEY_SEED, hex) }
export function setMode(m) { mode.value = m; save(KEY_MODE, m) }

export function initTheme() {
  apply()
  watch([seed, mode], apply)
  mql.addEventListener?.('change', () => mode.value === 'auto' && apply())
}
