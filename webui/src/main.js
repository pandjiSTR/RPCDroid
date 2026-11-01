import { createApp } from 'vue'
import { createPinia } from 'pinia'
import router from './router'
import App from './App.vue'
import { initTheme } from './theme'
import { initRipple } from './ripple'
import './style.css'

initTheme() // set colour roles before first paint
initRipple()

const app = createApp(App)
app.use(createPinia())
app.use(router)
app.mount('#app')
