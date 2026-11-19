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
  return value >= 100000 ? `${Math.round(value / 1000)} MHz` : raw;
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
      <section className="md3-card-elevated space-y-3 p-6 sm:p-7">
        <div className="flex items-center gap-2.5">
          <Activity className="h-4 w-4" style={{ color: 'var(--md-sys-color-primary)' }} />
          <h2 className="font-bold text-base">Direct Adaptive Runtime</h2>
          <span className="md3-badge md3-badge-neutral">NATIVE ONLY</span>
        </div>
        <p
          className="text-xs sm:text-sm leading-relaxed"
          style={{ color: 'var(--md-sys-color-on-surface-variant)' }}
        >
          Live topology discovery, actuator ownership, and OEM baseline values are queried directly from <code className="font-mono">coreflowd</code>. Browser preview does not synthesize kernel audit evidence.
        </p>
      </section>
    );
  }

  if (!status || !status.active) {
    return (
      <section className="md3-card-elevated space-y-3 p-6 sm:p-7">
        <div className="flex items-center gap-2.5">
          <ShieldAlert className="h-4 w-4" style={{ color: 'var(--md-sys-color-warning)' }} />
          <h2 className="font-bold text-base">Direct Adaptive Runtime</h2>
        </div>
        <p
          className="text-xs sm:text-sm leading-relaxed"
          style={{ color: 'var(--md-sys-color-on-surface-variant)' }}
        >
          {status?.mode === 'READ_ONLY'
            ? 'Daemon is stopped. The last OEM baseline remains available for recovery.'
            : 'Runtime discovery or durable baseline preparation is unavailable. Independent daemon status remains accessible.'}
        </p>
      </section>
    );
  }

  const ownedCount = status.appliedValues.filter((item) => item.owned).length;
  const modifiedCount = status.appliedValues.filter((item) => item.modified).length;
  const hasOwnershipIssue =
    status.ownershipLosses > 0 ||
    status.appliedValues.some((item) => !item.owned && item.modified);

  const badgeClass = status.journalFault
    ? 'md3-badge md3-badge-error'
    : status.thermalGuard
      ? 'md3-badge md3-badge-warning'
      : 'md3-badge md3-badge-success';

  return (
    <section
      className="md3-card-elevated space-y-5 p-6 sm:p-7"
      aria-label="Direct Adaptive runtime and actuator audit"
    >
      <div className="flex flex-col gap-3 sm:flex-row sm:items-start sm:justify-between">
        <div>
          <div className="flex items-center gap-2">
            <Activity className="h-4 w-4" style={{ color: 'var(--md-sys-color-primary)' }} />
            <h2 className="font-bold text-base sm:text-lg">
              Direct Adaptive · Production Composition Root
            </h2>
          </div>
          <p
            className="mt-1 text-xs sm:text-sm leading-relaxed"
            style={{ color: 'var(--md-sys-color-on-surface-variant)' }}
          >
            Topology-derived Sweet-Spot policy and readback-verified sysfs ownership. Optional PSI/FrameTimeline evidence never blocks independently discovered domains.
          </p>
        </div>

        <span className={badgeClass}>
          {status.journalFault ? (
            <ShieldAlert className="h-3.5 w-3.5" />
          ) : status.thermalGuard ? (
            <Gauge className="h-3.5 w-3.5" />
          ) : (
            <ShieldCheck className="h-3.5 w-3.5" />
          )}
          {status.journalFault
            ? 'Journal fault · mutations halted'
            : MODE_LABELS[status.mode] ?? status.mode}
        </span>
      </div>

      <div className="grid grid-cols-2 gap-3 sm:grid-cols-4">
        <div className="md3-card-filled p-3.5">
          <div
            className="flex items-center gap-1.5 text-[11px]"
            style={{ color: 'var(--md-sys-color-on-surface-variant)' }}
          >
            <Cpu className="h-3.5 w-3.5" /> CPU utilization
          </div>
          <div className="mt-1 font-mono text-lg font-bold tabular-nums">
            {status.cpuUtilizationPct.toFixed(1)}%
          </div>
        </div>
        <div className="md3-card-filled p-3.5">
          <div
            className="flex items-center gap-1.5 text-[11px]"
            style={{ color: 'var(--md-sys-color-on-surface-variant)' }}
          >
            <HardDrive className="h-3.5 w-3.5" /> RAM / ZRAM
          </div>
          <div className="mt-1 font-mono text-sm font-bold">{formatBytes(status.ramBytes)}</div>
          <div className="text-[11px]" style={{ color: 'var(--md-sys-color-on-surface-variant)' }}>
            ZRAM {formatBytes(status.zramBytes)}
          </div>
        </div>
        <div className="md3-card-filled p-3.5">
          <div
            className="flex items-center gap-1.5 text-[11px]"
            style={{ color: 'var(--md-sys-color-on-surface-variant)' }}
          >
            <MonitorSmartphone className="h-3.5 w-3.5" /> Display
          </div>
          <div className="mt-1 font-mono text-lg font-bold">
            {status.refreshHz > 0 ? `${status.refreshHz} Hz` : 'Auto'}
          </div>
          <div className="text-[11px]" style={{ color: 'var(--md-sys-color-on-surface-variant)' }}>
            Screen {status.screenState === 'OFF' ? 'off' : 'on / active'}
          </div>
        </div>
        <div className="md3-card-filled p-3.5">
          <div
            className="flex items-center gap-1.5 text-[11px]"
            style={{ color: 'var(--md-sys-color-on-surface-variant)' }}
          >
            <Gauge className="h-3.5 w-3.5" /> Actuator audit
          </div>
          <div className="mt-1 font-mono text-lg font-bold">{status.verifiedMutations}</div>
          <div className="text-[11px]" style={{ color: 'var(--md-sys-color-on-surface-variant)' }}>
            {status.skippedNodes} skipped · {status.ownershipLosses} ownership losses
          </div>
        </div>
      </div>

      <div className="grid grid-cols-1 gap-3 sm:grid-cols-2">
        <div className="md3-card-filled p-4">
          <div
            className="text-[11px] font-bold uppercase tracking-wide"
            style={{ color: 'var(--md-sys-color-on-surface-variant)' }}
          >
            GPU Discovered
          </div>
          <div className="mt-1 break-words font-mono text-sm font-bold">{status.gpuModel}</div>
        </div>
        <div className="md3-card-filled p-4">
          <div
            className="text-[11px] font-bold uppercase tracking-wide"
            style={{ color: 'var(--md-sys-color-on-surface-variant)' }}
          >
            Storage Topology
          </div>
          <div className="mt-1 break-words font-mono text-sm font-bold">{status.storageType}</div>
        </div>
      </div>

      <div className="space-y-3">
        <div className="flex items-center justify-between gap-2">
          <h3 className="text-sm font-bold">CPU Topology &amp; Discovered OPP</h3>
          <span className="text-xs font-mono" style={{ color: 'var(--md-sys-color-on-surface-variant)' }}>
            {status.cpuClusters.length} clusters · {status.discoveredNodes} nodes
          </span>
        </div>
        {status.cpuClusters.length === 0 ? (
          <p className="md3-card-filled p-3.5 text-xs" style={{ color: 'var(--md-sys-color-on-surface-variant)' }}>
            CPUFreq OPP table unavailable; CPU actuation is skipped independently.
          </p>
        ) : (
          <div className="grid grid-cols-1 gap-3 md:grid-cols-2">
            {status.cpuClusters.map((cluster) => (
              <div key={cluster.id} className="md3-card-filled min-w-0 p-3.5">
                <div className="text-xs font-bold">{cluster.id}</div>
                <div className="mt-2 flex flex-wrap gap-1.5">
                  {cluster.opps.map((opp) => (
                    <span
                      key={`${cluster.id}-${opp}`}
                      className="md3-badge md3-badge-neutral font-mono"
                    >
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
          <h3 className="text-sm font-bold">Current Applied Values vs OEM Baseline</h3>
          <span className="text-xs font-mono" style={{ color: 'var(--md-sys-color-on-surface-variant)' }}>
            {modifiedCount} modified · {ownedCount}/{status.appliedValues.length} owned
          </span>
        </div>
        {status.appliedValues.length === 0 ? (
          <p className="md3-card-filled p-3.5 text-xs" style={{ color: 'var(--md-sys-color-on-surface-variant)' }}>
            No writable semantic control nodes were captured.
          </p>
        ) : (
          <div className="space-y-2">
            {status.appliedValues.slice(0, 24).map((item) => (
              <div key={item.path} className="md3-card-filled space-y-2 p-3.5">
                <div className="flex flex-wrap items-center justify-between gap-2">
                  <div className="min-w-0">
                    <div className="break-all font-mono text-xs font-bold">{item.semantic}</div>
                    <div
                      className="break-all text-[10px]"
                      style={{ color: 'var(--md-sys-color-on-surface-variant)' }}
                    >
                      {item.domain}
                    </div>
                  </div>
                  <span
                    className={
                      item.owned
                        ? 'md3-badge md3-badge-success'
                        : 'md3-badge md3-badge-warning'
                    }
                  >
                    {item.owned ? (
                      <CheckCircle2 className="h-3 w-3" />
                    ) : (
                      <ShieldAlert className="h-3 w-3" />
                    )}
                    {item.owned ? 'Owned / readback OK' : item.health}
                  </span>
                </div>
                <div className="grid grid-cols-3 gap-2 text-[11px]">
                  <div>
                    <div style={{ color: 'var(--md-sys-color-on-surface-variant)' }}>Current</div>
                    <div className="mt-0.5 break-all font-mono font-bold">{item.current}</div>
                  </div>
                  <div>
                    <div style={{ color: 'var(--md-sys-color-on-surface-variant)' }}>OEM</div>
                    <div className="mt-0.5 break-all font-mono font-bold">{item.oemBaseline}</div>
                  </div>
                  <div>
                    <div style={{ color: 'var(--md-sys-color-on-surface-variant)' }}>Policy target</div>
                    <div className="mt-0.5 break-all font-mono font-bold">{item.target}</div>
                  </div>
                </div>
              </div>
            ))}
          </div>
        )}
      </div>

      {hasOwnershipIssue && (
        <div
          role="status"
          className="rounded-2xl px-4 py-3 text-xs"
          style={{
            backgroundColor: 'var(--md-sys-color-warning-container)',
            color: 'var(--md-sys-color-on-warning-container)',
          }}
        >
          One or more nodes lost mutation ownership or readback agreement. Those actuators are isolated; unrelated domains continue.
        </div>
      )}

      <div className="flex flex-wrap gap-2.5">
        <button type="button" onClick={onViewJournal} className="md3-btn-tonal">
          <FileText className="h-3.5 w-3.5" /> View Mutation Journal
        </button>
        <button
          type="button"
          onClick={onRestoreBaseline}
          disabled={restoringBaseline}
          className="md3-btn-outlined"
          style={{
            borderColor: 'var(--md-sys-color-error)',
            color: 'var(--md-sys-color-error)',
          }}
        >
          <RotateCcw className="h-3.5 w-3.5" />
          {restoringBaseline ? 'Restoring OEM baseline…' : 'Restore OEM Baseline & Stop'}
        </button>
      </div>

      {journalOutput !== null && (
        <div className="space-y-2">
          <div className="text-xs font-bold">Baseline Journal · Read-Only</div>
          <pre
            className="max-h-64 overflow-auto rounded-2xl p-4 text-[11px] font-mono leading-relaxed"
            style={{
              backgroundColor: 'var(--md-sys-color-surface-container-highest)',
              color: 'var(--md-sys-color-on-surface)',
            }}
          >
            {journalOutput}
          </pre>
        </div>
      )}

      <div
        className="text-[11px] leading-relaxed font-mono"
        style={{ color: 'var(--md-sys-color-on-surface-variant)' }}
      >
        OEM snapshot entries: {status.baselineNodes} · thermal pressure: {status.thermalPressure.toFixed(1)} · journal state: {status.journalFault ? 'fault-latched' : 'healthy'}
      </div>
    </section>
  );
};
