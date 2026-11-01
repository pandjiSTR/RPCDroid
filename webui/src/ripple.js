// Material-style touch ripple, delegated globally so every button gets one.
const HOSTS = 'button:not(.no-ripple), .ripple, a.nav-item'

export function initRipple() {
  document.addEventListener('pointerdown', e => {
    let el = e.target.closest?.(HOSTS)
    if (!el || el.disabled) return
    el = el.querySelector(':scope > .nav-ind') || el // nav bar ripples only the indicator
    const r = el.getBoundingClientRect()
    const size = Math.max(r.width, r.height) * 2
    const wave = document.createElement('span')
    wave.className = 'wave'
    wave.style.cssText = `width:${size}px;height:${size}px;left:${e.clientX - r.left - size / 2}px;top:${e.clientY - r.top - size / 2}px`
    el.appendChild(wave)
    wave.addEventListener('animationend', () => wave.remove())
    setTimeout(() => wave.remove(), 900)
  }, { passive: true })
}
