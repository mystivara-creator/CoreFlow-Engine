import React, { useCallback, useEffect, useState } from 'react';
import { Activity, CheckCircle2, LockKeyhole, ShieldAlert } from 'lucide-react';
import { execNativeKsuShell, hasNativeKernelSuBridge } from '../bridge/daemonBridge';

type SessionState = 'active' | 'idle' | 'unavailable';

const RESOLVE_BIN =
  'BIN="/data/adb/modules_update/cfae_framework/system/bin/coreflowd"; ' +
  '[ -f "$BIN" ] || BIN="/data/adb/modules/cfae_framework/system/bin/coreflowd"; ' +
  '[ -f "$BIN" ] || BIN="/data/adb/modules/coreflow_engine/system/bin/coreflowd"; ' +
  '[ -f "$BIN" ] || BIN="/data/adb/modules/cfae_framework/coreflowd"; ' +
  '[ -f "$BIN" ] || BIN="/data/adb/modules/coreflow_engine/coreflowd"; ' +
  '[ -f "$BIN" ] || BIN="/system_ext/bin/coreflowd"; ' +
  '[ -f "$BIN" ] || BIN="/system/bin/coreflowd"; ' +
  '[ -f "$BIN" ] || exit 127';

const VALIDATION_DOMAINS = [
  { title: 'Boot stability', detail: 'Daemon readiness, initial hardware discovery and recovery state' },
  { title: 'Idle equilibrium', detail: 'Baseline stability, drift detection and release of owned mutations' },
  { title: 'Interactive workload', detail: 'Foreground process membership, CPU pressure and response pacing' },
  { title: 'Sustained workload', detail: 'Long-running workload admission and thermal response' },
  { title: 'Multitasking', detail: 'Memory pressure, I/O pressure and zram activity' },
  { title: 'Thermal recovery', detail: 'Cooling trend, OEM thermal constraints and post-load stabilization' },
  { title: 'Ownership conflict', detail: 'External writer detection and safe mutation relinquishment' },
  { title: 'Journal recovery', detail: 'Write-ahead journal integrity and idempotent recovery admission' },
];

export const DeviceValidationSuiteCard: React.FC<{ nativeMode: boolean }> = ({ nativeMode }) => {
  const [sessionState, setSessionState] = useState<SessionState>('unavailable');
  const [sessionId, setSessionId] = useState<string | null>(null);
  const [message, setMessage] = useState('Waiting for native validation status.');
  const [lastChecked, setLastChecked] = useState<string | null>(null);
  const [error, setError] = useState<string | null>(null);

  const refresh = useCallback(async () => {
    if (!nativeMode || !hasNativeKernelSuBridge()) {
      setSessionState('unavailable');
      setSessionId(null);
      setMessage('Native validation status is unavailable in browser preview.');
      return;
    }

    try {
      const result = await execNativeKsuShell(`${RESOLVE_BIN}; "$BIN" --dvs-status`);
      if (result.errno !== 0) throw new Error(result.stderr || result.stdout || 'Validation status query failed');
      const response = result.stdout.trim();
      const active = response.match(/^OK ACTIVE ([0-9a-f-]{36})$/i);
      if (active) {
        setSessionState('active');
        setSessionId(active[1]);
        setMessage('A native validation capture is active. Its lifecycle remains daemon-owned.');
      } else if (response === 'OK INACTIVE') {
        setSessionState('idle');
        setSessionId(null);
        setMessage('No validation capture is currently active.');
      } else {
        throw new Error(response || 'Unexpected CF-DVS status response');
      }
      setError(null);
      setLastChecked(new Date().toLocaleTimeString());
    } catch (reason) {
      setSessionState('unavailable');
      setSessionId(null);
      setError(reason instanceof Error ? reason.message : String(reason));
      setMessage('Unable to read the native validation session state.');
    }
  }, [nativeMode]);

  useEffect(() => {
    let disposed = false;
    const check = async () => {
      if (disposed) return;
      await refresh();
    };
    void check();
    const timer = window.setInterval(() => void check(), 15000);
    return () => {
      disposed = true;
      window.clearInterval(timer);
    };
  }, [refresh]);

  const statusLabel = sessionState === 'active'
    ? 'Capture active'
    : sessionState === 'idle'
      ? 'Idle'
      : 'Unavailable';

  return (
    <div className="m3-card space-y-6 p-6 sm:p-8">
      <div className="flex items-start gap-3 border-b border-slate-900/[0.06] pb-5 dark:border-white/[0.06]">
        <div className="flex h-11 w-11 shrink-0 items-center justify-center rounded-2xl bg-[#d3e3fd] text-[#041e49] dark:bg-[#0842a0] dark:text-[#d3e3fd]">
          <Activity className="h-5 w-5" />
        </div>
        <div className="min-w-0 flex-1">
          <div className="flex flex-wrap items-center gap-2">
            <h2 className="text-base font-semibold text-slate-900 dark:text-slate-100">Device validation coverage</h2>
            <span className="rounded-full bg-slate-900/[0.05] px-2.5 py-1 text-[10px] font-bold text-slate-600 dark:bg-white/[0.07] dark:text-slate-300">READ ONLY</span>
          </div>
          <p className="mt-1 text-xs leading-relaxed text-slate-600 dark:text-slate-400">
            CoreFlow validates its own runtime invariants. This surface explains coverage and reports the native capture state; it does not start, stop, or configure a device test session.
          </p>
        </div>
      </div>

      <section className="m3-tonal-block flex flex-col gap-3 p-4 sm:flex-row sm:items-center sm:justify-between">
        <div>
          <div className="text-[10px] font-semibold uppercase tracking-[0.16em] text-slate-500 dark:text-slate-400">Native CF-DVS session</div>
          <div className="mt-2 flex items-center gap-2 text-sm font-semibold text-slate-900 dark:text-slate-100">
            <span className={`h-2.5 w-2.5 rounded-full ${sessionState === 'active' ? 'bg-emerald-500' : sessionState === 'idle' ? 'bg-slate-400' : 'bg-amber-500'}`} />
            {statusLabel}
          </div>
          <p className="mt-1 break-all text-xs text-slate-600 dark:text-slate-400">{sessionId ? `Session ${sessionId}` : message}</p>
          {lastChecked && <p className="mt-1 text-[10px] text-slate-500 dark:text-slate-400">Last checked {lastChecked} · automatic refresh every 15 seconds</p>}
        </div>
        <div className="flex shrink-0 items-center gap-2 text-xs text-slate-500 dark:text-slate-400">
          <LockKeyhole className="h-4 w-4" /> Daemon-owned lifecycle
        </div>
      </section>

      {error && (
        <div role="status" className="flex items-start gap-2 rounded-xl bg-amber-50 p-3 text-xs text-amber-900 dark:bg-amber-950/30 dark:text-amber-200">
          <ShieldAlert className="mt-0.5 h-4 w-4 shrink-0" />
          <span>{error}</span>
        </div>
      )}

      <section className="space-y-3">
        <div>
          <h3 className="text-sm font-semibold text-slate-900 dark:text-slate-100">Validation domains</h3>
          <p className="mt-1 text-xs text-slate-600 dark:text-slate-400">These are evidence domains, not user-selectable performance profiles.</p>
        </div>
        <div className="grid grid-cols-1 gap-2 sm:grid-cols-2">
          {VALIDATION_DOMAINS.map((domain) => (
            <div key={domain.title} className="flex items-start gap-3 rounded-2xl border border-slate-900/[0.06] p-3 dark:border-white/[0.07]">
              <CheckCircle2 className="mt-0.5 h-4 w-4 shrink-0 text-emerald-600 dark:text-emerald-400" />
              <div className="min-w-0">
                <div className="text-xs font-semibold text-slate-800 dark:text-slate-200">{domain.title}</div>
                <p className="mt-1 text-[11px] leading-relaxed text-slate-500 dark:text-slate-400">{domain.detail}</p>
              </div>
            </div>
          ))}
        </div>
      </section>

      <p className="border-t border-slate-900/[0.06] pt-4 text-[11px] leading-relaxed text-slate-500 dark:border-white/[0.07] dark:text-slate-400">
        Diagnostic capture is not an optimization command. Evidence collection and session finalization are orchestrated by the native runtime lifecycle; the WebUI cannot manually invoke them.
      </p>
    </div>
  );
};
