import React, { useCallback, useEffect, useMemo, useState } from 'react';
import { Activity, Archive, CheckCircle2, FileCheck2, LockKeyhole, Play, RefreshCw, ShieldAlert, Square } from 'lucide-react';
import { execNativeKsuShell, hasNativeKernelSuBridge } from '../bridge/daemonBridge';

type Profile =
  | 'BOOT_STABILITY'
  | 'IDLE_EQUILIBRIUM'
  | 'INTERACTIVE_CPU'
  | 'GAMING_LOAD'
  | 'MULTITASKING'
  | 'THERMAL_RECOVERY'
  | 'OWNERSHIP_CONFLICT'
  | 'ROLLBACK_RECOVERY';

type PayloadName = 'events.jsonl' | 'metrics.csv' | 'report.json' | 'summary.txt' | 'manifest.json';
type Payload = { name: PayloadName; bytes: Uint8Array };

const PROFILES: { id: Profile; title: string; detail: string }[] = [
  { id: 'BOOT_STABILITY', title: 'Boot stability', detail: 'Startup, daemon readiness, and recovery state' },
  { id: 'IDLE_EQUILIBRIUM', title: 'Idle equilibrium', detail: 'Idle stability and baseline drift' },
  { id: 'INTERACTIVE_CPU', title: 'Interactive CPU', detail: 'Interactive pacing, CPU pressure, and ownership' },
  { id: 'GAMING_LOAD', title: 'Gaming load', detail: 'Sustained workload and thermal response' },
  { id: 'MULTITASKING', title: 'Multitasking', detail: 'Memory and I/O pressure under concurrent use' },
  { id: 'THERMAL_RECOVERY', title: 'Thermal recovery', detail: 'Cooling trend and post-load stabilization' },
  { id: 'OWNERSHIP_CONFLICT', title: 'Ownership conflict', detail: 'External writer arbitration and safe relinquishment' },
  { id: 'ROLLBACK_RECOVERY', title: 'Rollback recovery', detail: 'Journal durability and recovery admission' },
];

const RESOLVE_BIN =
  'BIN="/data/adb/modules_update/cfae_framework/system/bin/coreflowd"; ' +
  '[ -f "$BIN" ] || BIN="/data/adb/modules/cfae_framework/system/bin/coreflowd"; ' +
  '[ -f "$BIN" ] || BIN="/data/adb/modules/coreflow_engine/system/bin/coreflowd"; ' +
  '[ -f "$BIN" ] || BIN="/data/adb/modules/cfae_framework/coreflowd"; ' +
  '[ -f "$BIN" ] || BIN="/data/adb/modules/coreflow_engine/coreflowd"; ' +
  '[ -f "$BIN" ] || BIN="/system_ext/bin/coreflowd"; ' +
  '[ -f "$BIN" ] || BIN="/system/bin/coreflowd"; ' +
  '[ -f "$BIN" ] || exit 127';

const UUID_PATTERN = /^[0-9a-f]{8}-[0-9a-f]{4}-4[0-9a-f]{3}-[89ab][0-9a-f]{3}-[0-9a-f]{12}$/i;
const PAYLOAD_NAMES: PayloadName[] = [
  'events.jsonl',
  'metrics.csv',
  'report.json',
  'summary.txt',
  'manifest.json',
];
const MAX_BUNDLE_BYTES = 9 * 1024 * 1024;

function decodeBase64(value: string): Uint8Array {
  const normalized = value.replace(/\\s+/g, '');
  if (!normalized || !/^[A-Za-z0-9+/]*={0,2}$/.test(normalized)) {
    throw new Error('Payload returned invalid Base64 data');
  }
  const binary = window.atob(normalized);
  const bytes = new Uint8Array(binary.length);
  for (let i = 0; i < binary.length; i += 1) bytes[i] = binary.charCodeAt(i);
  return bytes;
}

function crc32(bytes: Uint8Array): number {
  let crc = 0xffffffff;
  for (const byte of bytes) {
    crc ^= byte;
    for (let bit = 0; bit < 8; bit += 1) {
      crc = (crc >>> 1) ^ ((crc & 1) ? 0xedb88320 : 0);
    }
  }
  return (crc ^ 0xffffffff) >>> 0;
}

function u16(view: DataView, offset: number, value: number) {
  view.setUint16(offset, value, true);
}
function u32(view: DataView, offset: number, value: number) {
  view.setUint32(offset, value >>> 0, true);
}

function createStoredZip(files: Payload[]): Blob {
  const encoder = new TextEncoder();
  const localParts: Uint8Array[] = [];
  const centralParts: Uint8Array[] = [];
  let offset = 0;

  for (const file of files) {
    const name = encoder.encode(file.name);
    const data = file.bytes;
    const crc = crc32(data);
    const local = new Uint8Array(30 + name.length);
    const localView = new DataView(local.buffer);
    u32(localView, 0, 0x04034b50);
    u16(localView, 4, 20);
    u16(localView, 6, 0);
    u16(localView, 8, 0);
    u16(localView, 10, 0);
    u16(localView, 12, 0);
    u32(localView, 14, crc);
    u32(localView, 18, data.length);
    u32(localView, 22, data.length);
    u16(localView, 26, name.length);
    u16(localView, 28, 0);
    local.set(name, 30);
    localParts.push(local, data);

    const central = new Uint8Array(46 + name.length);
    const centralView = new DataView(central.buffer);
    u32(centralView, 0, 0x02014b50);
    u16(centralView, 4, 20);
    u16(centralView, 6, 20);
    u16(centralView, 8, 0);
    u16(centralView, 10, 0);
    u16(centralView, 12, 0);
    u16(centralView, 14, 0);
    u32(centralView, 16, crc);
    u32(centralView, 20, data.length);
    u32(centralView, 24, data.length);
    u16(centralView, 28, name.length);
    u16(centralView, 30, 0);
    u16(centralView, 32, 0);
    u16(centralView, 34, 0);
    u16(centralView, 36, 0);
    u32(centralView, 38, 0);
    u32(centralView, 42, offset);
    central.set(name, 46);
    centralParts.push(central);
    offset += local.length + data.length;
  }

  const centralSize = centralParts.reduce((sum, part) => sum + part.length, 0);
  const end = new Uint8Array(22);
  const endView = new DataView(end.buffer);
  u32(endView, 0, 0x06054b50);
  u16(endView, 4, 0);
  u16(endView, 6, 0);
  u16(endView, 8, files.length);
  u16(endView, 10, files.length);
  u32(endView, 12, centralSize);
  u32(endView, 16, offset);
  u16(endView, 20, 0);
  return new Blob([...localParts, ...centralParts, end].map((part) => part.slice().buffer as ArrayBuffer), { type: 'application/zip' });
}

async function sha256(bytes: Uint8Array): Promise<string> {
  if (!window.crypto?.subtle) throw new Error('Secure SHA-256 verification is unavailable in this WebView');
  const digest = await window.crypto.subtle.digest('SHA-256', bytes.slice().buffer as ArrayBuffer);
  return Array.from(new Uint8Array(digest), (value) => value.toString(16).padStart(2, '0')).join('');
}

export const DeviceValidationSuiteCard: React.FC<{ nativeMode: boolean }> = ({ nativeMode }) => {
  const [profile, setProfile] = useState<Profile>('INTERACTIVE_CPU');
  const [sessionId, setSessionId] = useState<string | null>(null);
  const [busy, setBusy] = useState(false);
  const [message, setMessage] = useState('No validation session is active.');
  const [error, setError] = useState<string | null>(null);
  const [bundleSummary, setBundleSummary] = useState<string | null>(null);
  const [bundleVerdict, setBundleVerdict] = useState<string | null>(null);
  const [bundleMetadata, setBundleMetadata] = useState<string | null>(null);
  const [checkResults, setCheckResults] = useState<{ id: string; status: string; detail: string }[]>([]);
  const bridgeAvailable = nativeMode && hasNativeKernelSuBridge();

  const runDaemon = useCallback(async (args: string, timeout = 10000) => {
    if (!bridgeAvailable) throw new Error('Native KernelSU bridge is unavailable');
    const result = await execNativeKsuShell(`${RESOLVE_BIN}; "$BIN" ${args}`, timeout);
    if (result.errno !== 0) throw new Error(result.stderr || result.stdout || 'coreflowd command failed');
    return result.stdout.trim();
  }, [bridgeAvailable]);

  const refresh = useCallback(async () => {
    if (!bridgeAvailable) return;
    setError(null);
    try {
      const response = await runDaemon('--dvs-status');
      const active = response.match(/^OK ACTIVE ([0-9a-f-]{36})$/i);
      if (active && UUID_PATTERN.test(active[1])) {
        setSessionId(active[1]);
        setMessage(`Capture active · ${active[1]}`);
      } else if (response === 'OK INACTIVE') {
        setSessionId(null);
        setMessage('No validation session is active.');
      } else {
        throw new Error(response || 'Unexpected CF-DVS status response');
      }
    } catch (reason) {
      setError(reason instanceof Error ? reason.message : String(reason));
    }
  }, [bridgeAvailable, runDaemon]);

  useEffect(() => {
    void refresh();
  }, [refresh]);

  const selectedProfile = useMemo(
    () => PROFILES.find((item) => item.id === profile) ?? PROFILES[2],
    [profile],
  );

  const startSession = async () => {
    setBusy(true);
    setError(null);
    setBundleSummary(null);
    setBundleVerdict(null);
    try {
      const response = await runDaemon(`--dvs-start ${profile}`);
      const match = response.match(/^OK ACTIVE ([0-9a-f-]{36})$/i);
      if (!match || !UUID_PATTERN.test(match[1])) throw new Error(response || 'Session start was not acknowledged');
      setSessionId(match[1]);
      setMessage(`Capture active · ${match[1]}`);
    } catch (reason) {
      setError(reason instanceof Error ? reason.message : String(reason));
    } finally {
      setBusy(false);
    }
  };

  const readPayload = async (id: string, name: PayloadName): Promise<Payload> => {
    const command =
      `${RESOLVE_BIN}; DIR="/data/adb/coreflow/cf-dvs/${id}"; ` +
      '[ -d "$DIR" ] && [ ! -L "$DIR" ] || exit 1; ' +
      `FILE="$DIR/${name}"; [ -f "$FILE" ] && [ ! -L "$FILE" ] || exit 1; ` +
      'base64 "$FILE" 2>/dev/null | tr -d "\\r\\n"';
    const result = await execNativeKsuShell(command, 30000);
    if (result.errno !== 0) throw new Error(result.stderr || `Unable to read ${name}`);
    return { name, bytes: decodeBase64(result.stdout) };
  };

  const exportBundle = async (id: string) => {
    setMessage('Reading local evidence and verifying SHA-256 manifest…');
    const files: Payload[] = [];
    let totalBytes = 0;
    for (const name of PAYLOAD_NAMES) {
      const file = await readPayload(id, name);
      totalBytes += file.bytes.length;
      if (totalBytes > MAX_BUNDLE_BYTES) throw new Error('Bundle exceeds the WebUI export safety limit');
      files.push(file);
    }

    const manifestFile = files.find((file) => file.name === 'manifest.json');
    if (!manifestFile) throw new Error('Bundle manifest is missing');
    const manifest = JSON.parse(new TextDecoder().decode(manifestFile.bytes)) as {
      manifest_version?: string;
      files?: { path: string; size_bytes: number; sha256: string }[];
    };
    if (manifest.manifest_version !== '1.0' || !Array.isArray(manifest.files)) {
      throw new Error('Unsupported CF-DVS manifest version');
    }
    const expectedPayloads = PAYLOAD_NAMES.filter((name) => name !== 'manifest.json');
    if (manifest.files.length !== expectedPayloads.length) {
      throw new Error('Manifest payload inventory is incomplete');
    }
    const seen = new Set<string>();

    for (const entry of manifest.files) {
      if (
        !expectedPayloads.includes(entry.path as PayloadName) ||
        seen.has(entry.path) ||
        !Number.isSafeInteger(entry.size_bytes) ||
        entry.size_bytes < 0 ||
        typeof entry.sha256 !== 'string' ||
        !/^[0-9a-f]{64}$/i.test(entry.sha256)
      ) {
        throw new Error('Manifest contains an invalid or duplicate payload entry');
      }
      seen.add(entry.path);
      const payload = files.find((file) => file.name === entry.path);
      if (!payload || payload.bytes.length !== entry.size_bytes) {
        throw new Error(`Payload size mismatch: ${entry.path}`);
      }
      if ((await sha256(payload.bytes)) !== entry.sha256.toLowerCase()) {
        throw new Error(`SHA-256 mismatch: ${entry.path}`);
      }
    }
    if (expectedPayloads.some((name) => !seen.has(name))) {
      throw new Error('Manifest omitted a required payload');
    }

    const reportFile = files.find((file) => file.name === 'report.json');
    const summaryFile = files.find((file) => file.name === 'summary.txt');
    if (!reportFile || !summaryFile) throw new Error('Required report files are missing');
    const report = JSON.parse(new TextDecoder().decode(reportFile.bytes)) as {
      verdict?: string;
      checks?: { id?: string; status?: string; detail?: string }[];
      session?: { device?: { manufacturer?: string; model?: string; android_api?: number } };
      build?: { commit?: string };
    };
    const summary = new TextDecoder().decode(summaryFile.bytes);
    const verdict = typeof report.verdict === 'string' ? report.verdict : 'UNKNOWN';
    setBundleVerdict(verdict);
    const device = report.session?.device;
    setBundleMetadata([
      `Manufacturer: ${device?.manufacturer ?? 'not reported'}`,
      `Model: ${device?.model ?? 'not reported'}`,
      `Android API: ${device?.android_api ?? 'not reported'}`,
      `Build commit: ${report.build?.commit ?? 'unknown'}`,
    ].join(' · '));
    setCheckResults(Array.isArray(report.checks) ? report.checks.map((check) => ({
      id: typeof check.id === 'string' ? check.id : 'unknown',
      status: typeof check.status === 'string' ? check.status : 'inconclusive',
      detail: typeof check.detail === 'string' ? check.detail : 'No detail supplied',
    })) : []);
    setBundleSummary(summary);

    const zip = createStoredZip(files);
    const url = URL.createObjectURL(zip);
    const anchor = document.createElement('a');
    anchor.href = url;
    anchor.download = `cf-dvs-${id}.zip`;
    document.body.appendChild(anchor);
    anchor.click();
    anchor.remove();
    window.setTimeout(() => URL.revokeObjectURL(url), 30000);
    setMessage(`Verified local bundle ready · ${(totalBytes / 1024).toFixed(1)} KiB`);
  };

  const stopAndExport = async () => {
    if (!sessionId) return;
    setBusy(true);
    setError(null);
    try {
      const response = await runDaemon('--dvs-stop', 15000);
      const match = response.match(/^OK STOPPED \/data\/adb\/coreflow\/cf-dvs\/([0-9a-f-]{36})$/i);
      if (!match || !UUID_PATTERN.test(match[1]) || match[1] !== sessionId) {
        throw new Error(response || 'Session finalization was not acknowledged');
      }
      setSessionId(null);
      await exportBundle(match[1]);
    } catch (reason) {
      setError(reason instanceof Error ? reason.message : String(reason));
      await refresh();
    } finally {
      setBusy(false);
    }
  };

  return (
    <div className="m3-card p-6 sm:p-8 space-y-6">
      <div className="flex flex-col sm:flex-row sm:items-start justify-between gap-4 border-b border-slate-900/[0.06] dark:border-white/[0.06] pb-5">
        <div className="space-y-1.5">
          <div className="flex items-center gap-2 text-sm font-semibold">
            <Activity className="w-4 h-4 text-[#0b57d0] dark:text-[#a8c7fa]" />
            Device Validation Suite
          </div>
          <p className="text-xs sm:text-sm leading-relaxed text-slate-600 dark:text-slate-400 max-w-2xl">
            Capture a bounded, read-only device session. The suite records evidence; it does not tune the device or certify performance by itself.
          </p>
        </div>
        <button
          type="button"
          onClick={() => void refresh()}
          disabled={!bridgeAvailable || busy}
          className="px-4 py-2 rounded-full text-xs font-medium inline-flex items-center gap-2 bg-slate-900/[0.05] dark:bg-white/[0.06] disabled:opacity-40"
        >
          <RefreshCw className="w-3.5 h-3.5" /> Refresh
        </button>
      </div>

      {!bridgeAvailable && (
        <div className="rounded-2xl p-4 bg-amber-50 dark:bg-amber-950/30 text-amber-900 dark:text-amber-200 text-xs flex gap-3">
          <ShieldAlert className="w-4 h-4 shrink-0 mt-0.5" />
          CF-DVS controls require the installed native module and a KernelSU-compatible WebView bridge. Browser preview never simulates a real capture.
        </div>
      )}

      <div className="grid grid-cols-1 sm:grid-cols-[minmax(0,1fr)_minmax(240px,0.85fr)] gap-5">
        <div className="space-y-3">
          <label className="block text-xs font-semibold text-slate-700 dark:text-slate-200" htmlFor="cf-dvs-profile">Validation profile</label>
          <select
            id="cf-dvs-profile"
            value={profile}
            onChange={(event) => setProfile(event.target.value as Profile)}
            disabled={!bridgeAvailable || busy || Boolean(sessionId)}
            className="w-full rounded-xl border border-slate-900/10 dark:border-white/10 bg-white dark:bg-[#181c24] px-4 py-3 text-sm"
          >
            {PROFILES.map((item) => <option key={item.id} value={item.id}>{item.title}</option>)}
          </select>
          <p className="text-xs text-slate-500 dark:text-slate-400">{selectedProfile.detail}</p>
          <div className="flex flex-wrap gap-2 pt-2">
            {!sessionId ? (
              <button
                type="button"
                onClick={() => void startSession()}
                disabled={!bridgeAvailable || busy}
                className="px-5 py-2.5 rounded-full bg-[#0b57d0] text-white text-xs font-semibold inline-flex items-center gap-2 disabled:opacity-40"
              >
                <Play className="w-3.5 h-3.5" /> Start capture
              </button>
            ) : (
              <button
                type="button"
                onClick={() => void stopAndExport()}
                disabled={busy}
                className="px-5 py-2.5 rounded-full bg-rose-600 text-white text-xs font-semibold inline-flex items-center gap-2 disabled:opacity-40"
              >
                <Square className="w-3.5 h-3.5" /> Stop & export ZIP
              </button>
            )}
          </div>
        </div>

        <div className="rounded-2xl p-4 bg-slate-900/[0.035] dark:bg-white/[0.035] space-y-3">
          <div className="text-[11px] uppercase tracking-wider text-slate-500 dark:text-slate-400 font-semibold">Session state</div>
          <div className="flex items-center gap-2 text-sm font-semibold">
            <span className={`w-2 h-2 rounded-full ${sessionId ? 'bg-emerald-500 animate-pulse' : 'bg-slate-400'}`} />
            {sessionId ? 'Capturing evidence' : 'Idle'}
          </div>
          <p className="text-xs text-slate-600 dark:text-slate-400 break-all">{message}</p>
          {sessionId && <p className="text-[11px] font-mono text-slate-500">Session ID: {sessionId}</p>}
          {busy && <p className="text-xs text-[#0b57d0] dark:text-[#a8c7fa]">Working… keep this WebUI open.</p>}
        </div>
      </div>

      {error && (
        <div role="alert" className="rounded-xl p-3 bg-rose-50 dark:bg-rose-950/40 text-rose-800 dark:text-rose-200 text-xs">
          {error}
        </div>
      )}

      <div className="grid grid-cols-1 md:grid-cols-2 gap-4">
        <div className="rounded-2xl border border-slate-900/[0.07] dark:border-white/[0.08] p-4 space-y-3">
          <div className="flex items-center gap-2 text-xs font-semibold"><LockKeyhole className="w-4 h-4" /> Privacy review</div>
          <ul className="space-y-2 text-xs text-slate-600 dark:text-slate-400">
            <li className="flex gap-2"><CheckCircle2 className="w-3.5 h-3.5 shrink-0 text-emerald-600" /> Local files only; no upload or network transfer.</li>
            <li className="flex gap-2"><CheckCircle2 className="w-3.5 h-3.5 shrink-0 text-emerald-600" /> Export allowlist contains five fixed payload filenames.</li>
            <li className="flex gap-2"><CheckCircle2 className="w-3.5 h-3.5 shrink-0 text-emerald-600" /> No serial, Android ID, account, SSID, or precise-location field is requested by this UI.</li>
            <li className="flex gap-2"><ShieldAlert className="w-3.5 h-3.5 shrink-0 text-amber-600" /> Review report contents before manually sharing the ZIP.</li>
          </ul>
        </div>
        <div className="rounded-2xl border border-slate-900/[0.07] dark:border-white/[0.08] p-4 space-y-3">
          <div className="flex items-center gap-2 text-xs font-semibold"><FileCheck2 className="w-4 h-4" /> Export integrity</div>
          <p className="text-xs text-slate-600 dark:text-slate-400">
            The WebUI verifies each payload size and SHA-256 against the native manifest before building a ZIP archive. A mismatch blocks export.
          </p>
          {bundleVerdict && <div className="text-xs font-semibold">Captured verdict: {bundleVerdict}</div>}
          {bundleMetadata && <p className="text-[11px] text-slate-500 dark:text-slate-400">{bundleMetadata}</p>}
          {checkResults.length > 0 && <ul className="space-y-2 text-xs">{checkResults.map((check) => (
            <li key={check.id} className="flex items-start justify-between gap-3">
              <span className="min-w-0"><span className="font-medium">{check.id}</span><span className="block text-slate-500 dark:text-slate-400">{check.detail}</span></span>
              <span className="shrink-0 font-semibold uppercase">{check.status}</span>
            </li>
          ))}</ul>}
          {bundleSummary && <pre className="max-h-40 overflow-auto whitespace-pre-wrap rounded-xl bg-slate-900/[0.04] dark:bg-white/[0.04] p-3 text-[11px]">{bundleSummary}</pre>}
          {bundleSummary && <div className="text-[11px] text-slate-500 flex items-center gap-2"><Archive className="w-3.5 h-3.5" /> ZIP assembled locally; no automatic sharing occurred.</div>}
          {!bundleSummary && <p className="text-xs text-slate-500">No bundle exported in this WebUI session.</p>}
        </div>
      </div>
    </div>
  );
};
