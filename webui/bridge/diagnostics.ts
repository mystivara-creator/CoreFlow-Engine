import { execNativeKsuShell, hasNativeKernelSuBridge, ParsedEngineStatus } from './daemonBridge';

export const NATIVE_HISTORY_LIMIT = 300;
export type TelemetryRecord = Record<string, string | number | boolean | null>;

export function nativeTelemetryRecord(parsed: ParsedEngineStatus): TelemetryRecord | null {
  const d = parsed.ok ? parsed.data : undefined;
  if (!d || !d.running || d.pid <= 0) return null;
  return {
    Sample: d.sample, LifetimeSample: d.memory?.lifetimeSample ?? null, UpdatedEpoch: d.updated, PID: d.pid,
    'CPU(%)': d.eyes.cpu === null ? null : d.eyes.cpu * 100,
    Load1: d.eyes.load, MemAvailableRatio: d.eyes.mem,
    'Thermal(C)': d.eyes.thermal, 'HottestThermal(C)': d.eyes.hottest,
    Charging: d.eyes.charging, 'Battery(%)': d.eyes.battery,
    'IORead(KB/s)': d.eyes.ioRead, 'IOWrite(KB/s)': d.eyes.ioWrite,
    Confidence: d.eyes.confidence, State: d.brain.state,
    RenderingAvailable: d.intelligence?.renderingAvailable ?? false,
    PresentationMissRateProxy: d.intelligence?.renderingAvailable ? d.intelligence.frameJankRate : null,
    Foreground: d.intelligence?.foregroundProcess ?? '',
    RenderSurface: d.intelligence?.renderingLayer ?? '',
  };
}

export function appendNativeTelemetry(history: TelemetryRecord[], next: TelemetryRecord): TelemetryRecord[] {
  const last = history[history.length - 1];
  // A PID change or backwards sample counter starts a new daemon session.
  if (last && (last.PID !== next.PID || Number(next.Sample) < Number(last.Sample))) return [next];
  if (last && last.Sample === next.Sample) return [...history.slice(0, -1), next];
  return [...history.slice(-(NATIVE_HISTORY_LIMIT - 1)), next];
}

function csvCell(value: TelemetryRecord[string]): string {
  if (value === null) return '';
  let text = String(value);
  // Spreadsheet formula protection applies to strings, not measured numbers.
  if (typeof value === 'string' && /^[\s]*[=+\-@]/.test(text)) text = "'" + text;
  return /[",\r\n]/.test(text) ? '"' + text.replace(/"/g, '""') + '"' : text;
}

export function nativeHistoryToCsv(history: TelemetryRecord[]): string {
  if (!history.length) return '';
  const keys = Object.keys(history[0]);
  return '\uFEFF' + [keys.map(csvCell).join(','), ...history.map(row => keys.map(key => csvCell(row[key] ?? null)).join(','))].join('\r\n');
}

const quote = (s: string) => "'" + s.replace(/'/g, "'\"'\"'") + "'";

export async function saveDiagnosticFile(filename: string, content: string, mime: string): Promise<string> {
  if (!/^[a-zA-Z0-9][a-zA-Z0-9._-]{0,119}$/.test(filename)) throw new Error('Invalid export filename.');
  const bytes = new TextEncoder().encode(content);
  if (bytes.length > 512 * 1024) throw new Error('Export exceeds 512 KiB. Narrow the filter and try again.');
  if (!hasNativeKernelSuBridge()) {
    const url = URL.createObjectURL(new Blob([content], { type: mime }));
    const a = document.createElement('a');
    a.href = url; a.download = filename;
    document.body.appendChild(a);
    a.click(); a.remove();
    // Revoking immediately can cancel the WebView/browser's asynchronous read.
    window.setTimeout(() => URL.revokeObjectURL(url), 60000);
    return `Download requested: ${filename}`;
  }
  const run = async (cmd: string) => {
    const result = await execNativeKsuShell(cmd);
    if (result.errno !== 0) throw new Error((result.stderr || result.stdout || `Shell exit ${result.errno}`).trim());
    return result.stdout.trim();
  };
  // Stage in private daemon storage; never interpolate diagnostic text into shell code.
  const temp = await run('set -eu; umask 077; mkdir -p /data/adb/coreflow; mktemp /data/adb/coreflow/.webui-export.XXXXXX');
  if (!/^\/data\/adb\/coreflow\/\.webui-export\.[a-zA-Z0-9]+$/.test(temp)) throw new Error('Export staging failed.');
  const destination = `/sdcard/Download/${filename}`;
  try {
    let binary = '';
    for (let i = 0; i < bytes.length; i += 8192) binary += String.fromCharCode(...bytes.subarray(i, i + 8192));
    const encoded = btoa(binary);
    for (let i = 0; i < encoded.length; i += 32768) {
      await run(`set -eu; printf '%s' ${quote(encoded.slice(i, i + 32768))} | base64 -d >> ${quote(temp)}`);
    }
    const output = await run(`set -eu; mkdir -p /sdcard/Download; umask 022; set -C; cat ${quote(temp)} > ${quote(destination)}; printf '%s' ${quote(destination)}`);
    if (output !== destination) throw new Error('Export completion could not be verified.');
    return `Saved: ${destination}`;
  } finally {
    await execNativeKsuShell(`rm -f ${quote(temp)}`);
  }
}
