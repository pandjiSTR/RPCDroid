import { exec } from 'kernelsu'

// kernelsu v3: exec() returns { errno, stdout, stderr }; errno=0 = success
export async function ksuExec(cmd) {
  try {
    const result = await exec(cmd)
    return { ...result, exitCode: result.errno === 0 ? 0 : 1 }
  } catch (e) {
    return { stdout: '', stderr: String(e), errno: 1, exitCode: 1 }
  }
}

export async function readFile(path) {
  const r = await ksuExec(`cat "${path}" 2>/dev/null`)
  if (r.exitCode !== 0 || !r.stdout) return null
  return r.stdout.trim()
}

export async function writeFile(path, content) {
  const escaped = content.replace(/\\/g, '\\\\').replace(/'/g, "'\\''")
  const r = await ksuExec(`printf '%s' '${escaped}' > "${path}"`)
  return r.exitCode === 0
}

export async function isDaemonRunning() {
  const r = await ksuExec('pidof rpcdroid 2>/dev/null')
  return r.exitCode === 0 && r.stdout.trim().length > 0
}

export async function startDaemon() {
  return ksuExec('/data/adb/modules/rpcdroid/bin/rpcdroid daemon >/dev/null 2>&1 &')
}

export async function stopDaemon() {
  return ksuExec('killall rpcdroid 2>/dev/null; true')
}

export async function restartDaemon() {
  await stopDaemon()
  await new Promise(r => setTimeout(r, 1500))
  return startDaemon()
}
