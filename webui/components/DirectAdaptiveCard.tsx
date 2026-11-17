import React from 'react';
import {
  Activity,
  CheckCircle2,
  Cpu,
  FileText,
  Gauge,
  HardDrive,
  MonitorSmartphone,
  RotateCcw,
  ShieldAlert,
  ShieldCheck,
} from 'lucide-react';
import { DirectAdaptiveStatus } from '../engine/autonomousEngine';

interface DirectAdaptiveCardProps {
  status?: DirectAdaptiveStatus;
  nativeMode: boolean;
  journalOutput: string | null;
  restoringBaseline: boolean;
  onViewJournal: () => void;
  onRestoreBaseline: () => void;
}

const MODE_LABELS: Record<string, string> = {
  SCREEN_OFF: 'Screen Off / Deep Idle',
  BALANCED_SWEET_SPOT: 'Balanced Sweet-Spot',
  ACTIVE_LOAD: 'Active Load / Gaming',
  THERMAL_GUARD: 'Thermal Guard',
  READ_ONLY: 'Read-only status',
  DISCOVERY: 'Capability discovery',
};

const formatBytes = (bytes: number): string => {
  if (!Number.isFinite(bytes) || bytes <= 0) return 'Unavailable';
  const units = ['B', 'KB', 'MB', 'GB', 'TB'];
  let value = bytes;
  let index = 0;
  while (value >= 1024 && index < units.length - 1) {
    value /= 1024;
    index += 1;
  }
  return value.toFixed(index >= 3 ? 2 : 0) + ' ' + units[index];
};

const formatOpp = (raw: string): string => {
  const value = Number(raw);
  if (!Number.isFinite(value)) return raw;
  return value >= 100000 ? String(Math.round(value / 1000)) + ' MHz' : raw;
};

export const DirectAdaptiveCard: React.FC<DirectAdaptiveCardProps> = ({
  status,
  nativeMode,
  journalOutput,
  restoringBaseline,
  onViewJournal,
  onRestoreBaseline,
}) => {
  if (!nativeMode) {
    return (
      <section className="m3-card space-y-3 p-5 sm:p-7">
        <div className="flex items-center gap-2">
          <Activity className="h-4 w-4 text-[#0b57d0] dark:text-[#a8c7fa]" />
          <h2 className="font-semibold">Direct Adaptive runtime</h2>
        </div>
        <p className="text-sm text-slate-600 dark:text-slate-400">
          Live topology, actuator ownership and OEM baseline values are available only from the native daemon.
          Browser preview does not synthesize kernel audit evidence.
        </p>
      </section>
    );
  }

  if (!status || !status.active) {
    return (
      <section className="m3-card space-y-3 p-5 sm:p-7">
        <div className="flex items-center gap-2">
          <ShieldAlert className="h-4 w-4 text-amber-600 dark:text-amber-300" />
          <h2 className="font-semibold">Direct Adaptive runtime</h2>
        </div>
        <p className="text-sm text-slate-600 dark:text-slate-400">
          {status?.mode === 'READ_ONLY'
            ? 'Daemon is stopped. The last OEM baseline remains available for recovery.'
            : 'Runtime discovery or durable baseline preparation is unavailable. Independent daemon status remains accessible.'}
        </p>
      </section>
    );
  }

  const ownedCount = status.appliedValues.filter((item) => item.owned).length;
  const modifiedCount = status.appliedValues.filter((item) => item.modified).length;
  const hasOwnershipIssue = status.ownershipLosses > 0 ||
    status.appliedValues.some((item) => !item.owned && item.modified);

  return (
    <section className="m3-card space-y-5 p-5 sm:p-7" aria-label="Direct Adaptive runtime and actuator audit">
      <div className="flex flex-col gap-3 sm:flex-row sm:items-start sm:justify-between">
        <div>
          <div className="flex items-center gap-2">
            <Activity className="h-4 w-4 text-[#0b57d0] dark:text-[#a8c7fa]" />
            <h2 className="font-semibold">Direct Adaptive · Current runtime</h2>
          </div>
          <p className="mt-1 text-sm text-slate-600 dark:text-slate-400">
            Topology-derived policy and readback-verified sysfs ownership. Optional PSI/FrameTimeline evidence does not gate this runtime.
          </p>
        </div>
        <span className={
          'inline-flex w-fit items-center gap-2 rounded-full px-3 py-1.5 text-xs font-semibold ' +
          (status.journalFault
            ? 'bg-rose-100 text-rose-800 dark:bg-rose-950/60 dark:text-rose-200'
            : status.thermalGuard
              ? 'bg-amber-100 text-amber-900 dark:bg-amber-950/60 dark:text-amber-200'
              : 'bg-emerald-100 text-emerald-800 dark:bg-emerald-950/60 dark:text-emerald-200')
        }>
          {status.journalFault ? <ShieldAlert className="h-3.5 w-3.5" /> :
            status.thermalGuard ? <Gauge className="h-3.5 w-3.5" /> :
              <ShieldCheck className="h-3.5 w-3.5" />}
          {status.journalFault ? 'Journal fault · mutations halted' :
            MODE_LABELS[status.mode] ?? status.mode}
        </span>
      </div>

      <div className="grid grid-cols-2 gap-3 sm:grid-cols-4">
        <div className="m3-tonal-block p-3">
          <div className="flex items-center gap-1.5 text-[11px] text-slate-500 dark:text-slate-400"><Cpu className="h-3.5 w-3.5" /> CPU utilization</div>
          <div className="mt-1 font-mono text-lg font-semibold tabular-nums">{status.cpuUtilizationPct.toFixed(1)}%</div>
        </div>
        <div className="m3-tonal-block p-3">
          <div className="flex items-center gap-1.5 text-[11px] text-slate-500 dark:text-slate-400"><HardDrive className="h-3.5 w-3.5" /> RAM / ZRAM</div>
          <div className="mt-1 font-mono text-sm font-semibold">{formatBytes(status.ramBytes)}</div>
          <div className="text-[11px] text-slate-500 dark:text-slate-400">ZRAM {formatBytes(status.zramBytes)}</div>
        </div>
        <div className="m3-tonal-block p-3">
          <div className="flex items-center gap-1.5 text-[11px] text-slate-500 dark:text-slate-400"><MonitorSmartphone className="h-3.5 w-3.5" /> Display</div>
          <div className="mt-1 font-mono text-lg font-semibold">{status.refreshHz > 0 ? String(status.refreshHz) + ' Hz' : 'Auto'}</div>
          <div className="text-[11px] text-slate-500 dark:text-slate-400">Screen {status.screenState === 'OFF' ? 'off' : 'on / unknown'}</div>
        </div>
        <div className="m3-tonal-block p-3">
          <div className="flex items-center gap-1.5 text-[11px] text-slate-500 dark:text-slate-400"><Gauge className="h-3.5 w-3.5" /> Actuator audit</div>
          <div className="mt-1 font-mono text-lg font-semibold">{status.verifiedMutations}</div>
          <div className="text-[11px] text-slate-500 dark:text-slate-400">{status.skippedNodes} skipped · {status.ownershipLosses} ownership losses</div>
        </div>
      </div>

      <div className="grid grid-cols-1 gap-3 sm:grid-cols-2">
        <div className="m3-tonal-block p-4">
          <div className="text-[11px] font-semibold uppercase tracking-wide text-slate-500 dark:text-slate-400">GPU discovered</div>
          <div className="mt-1 break-words font-mono text-sm font-semibold">{status.gpuModel}</div>
        </div>
        <div className="m3-tonal-block p-4">
          <div className="text-[11px] font-semibold uppercase tracking-wide text-slate-500 dark:text-slate-400">Storage topology</div>
          <div className="mt-1 break-words font-mono text-sm font-semibold">{status.storageType}</div>
        </div>
      </div>

      <div className="space-y-3">
        <div className="flex items-center justify-between gap-2">
          <h3 className="text-sm font-semibold">CPU topology & discovered OPP</h3>
          <span className="text-[11px] text-slate-500 dark:text-slate-400">{status.cpuClusters.length} clusters · {status.discoveredNodes} nodes</span>
        </div>
        {status.cpuClusters.length === 0 ? (
          <p className="m3-tonal-block p-3 text-xs text-slate-500 dark:text-slate-400">CPUFreq OPP table unavailable; CPU actuation is skipped independently.</p>
        ) : (
          <div className="grid grid-cols-1 gap-3 md:grid-cols-2">
            {status.cpuClusters.map((cluster) => (
              <div key={cluster.id} className="m3-tonal-block min-w-0 p-3">
                <div className="text-xs font-semibold">{cluster.id}</div>
                <div className="mt-2 flex flex-wrap gap-1.5">
                  {cluster.opps.map((opp) => (
                    <span key={cluster.id + '-' + opp} className="rounded-md bg-white/70 px-2 py-1 font-mono text-[10px] text-slate-700 dark:bg-white/[0.06] dark:text-slate-200">
                      {formatOpp(opp)}
                    </span>
                  ))}
                </div>
              </div>
            ))}
          </div>
        )}
      </div>

      <div className="space-y-3">
        <div className="flex flex-col gap-1 sm:flex-row sm:items-end sm:justify-between">
          <h3 className="text-sm font-semibold">Current applied values vs OEM baseline</h3>
          <span className="text-[11px] text-slate-500 dark:text-slate-400">
            {modifiedCount} modified · {ownedCount}/{status.appliedValues.length} owned
          </span>
        </div>
        {status.appliedValues.length === 0 ? (
          <p className="m3-tonal-block p-3 text-xs text-slate-500 dark:text-slate-400">No writable semantic control nodes were captured.</p>
        ) : (
          <div className="space-y-2">
            {status.appliedValues.slice(0, 24).map((item) => (
              <div key={item.path} className="m3-tonal-block space-y-2 p-3">
                <div className="flex flex-wrap items-center justify-between gap-2">
                  <div className="min-w-0">
                    <div className="break-all font-mono text-xs font-semibold">{item.semantic}</div>
                    <div className="break-all text-[10px] text-slate-500 dark:text-slate-400">{item.domain}</div>
                  </div>
                  <span className={
                    'inline-flex items-center gap-1 rounded-full px-2 py-1 text-[10px] font-semibold ' +
                    (item.owned
                      ? 'bg-emerald-100 text-emerald-800 dark:bg-emerald-950/60 dark:text-emerald-200'
                      : 'bg-amber-100 text-amber-900 dark:bg-amber-950/60 dark:text-amber-200')
                  }>
                    {item.owned ? <CheckCircle2 className="h-3 w-3" /> : <ShieldAlert className="h-3 w-3" />}
                    {item.owned ? 'Owned / readback OK' : item.health}
                  </span>
                </div>
                <div className="grid grid-cols-3 gap-2 text-[10px]">
                  <div><div className="text-slate-500 dark:text-slate-400">Current</div><div className="mt-0.5 break-all font-mono font-semibold">{item.current}</div></div>
                  <div><div className="text-slate-500 dark:text-slate-400">OEM</div><div className="mt-0.5 break-all font-mono font-semibold">{item.oemBaseline}</div></div>
                  <div><div className="text-slate-500 dark:text-slate-400">Policy target</div><div className="mt-0.5 break-all font-mono font-semibold">{item.target}</div></div>
                </div>
              </div>
            ))}
          </div>
        )}
      </div>

      {hasOwnershipIssue && (
        <div role="status" className="rounded-xl border border-amber-300/70 bg-amber-50 px-3 py-2 text-xs text-amber-900 dark:border-amber-800 dark:bg-amber-950/40 dark:text-amber-200">
          One or more nodes lost mutation ownership or readback agreement. Those actuators are isolated; unrelated domains continue.
        </div>
      )}

      <div className="flex flex-wrap gap-2">
        <button type="button" onClick={onViewJournal} className="inline-flex items-center gap-2 rounded-full bg-[#d3e3fd] px-4 py-2.5 text-xs font-semibold text-[#041e49] hover:bg-[#b8d3f7] dark:bg-[#0842a0] dark:text-[#d3e3fd]">
          <FileText className="h-3.5 w-3.5" /> View mutation journal
        </button>
        <button type="button" onClick={onRestoreBaseline} disabled={restoringBaseline} className="inline-flex items-center gap-2 rounded-full border border-rose-300/70 px-4 py-2.5 text-xs font-semibold text-rose-800 hover:bg-rose-50 disabled:opacity-50 dark:border-rose-800 dark:text-rose-200 dark:hover:bg-rose-950/30">
          <RotateCcw className="h-3.5 w-3.5" /> {restoringBaseline ? 'Restoring OEM baseline…' : 'Restore OEM baseline & stop'}
        </button>
      </div>

      {journalOutput !== null && (
        <div className="space-y-2">
          <div className="text-xs font-semibold">Baseline journal · read-only</div>
          <pre className="max-h-64 overflow-auto rounded-xl bg-slate-950 p-3 text-[10px] leading-relaxed text-emerald-200">{journalOutput}</pre>
        </div>
      )}

      <div className="text-[10px] leading-relaxed text-slate-500 dark:text-slate-400">
        OEM snapshot entries: {status.baselineNodes} · thermal pressure: {status.thermalPressure.toFixed(1)} · journal state: {status.journalFault ? 'fault-latched' : 'healthy'}
      </div>
    </section>
  );
};
