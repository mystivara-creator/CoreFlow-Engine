import React from 'react';
import {
  Activity,
  Cpu,
  Flame,
  Gauge,
  GitCommit,
  HardDrive,
  Play,
  ShieldCheck,
  Sliders,
  Sparkles,
  Zap,
} from 'lucide-react';
import {
  AutonomousTelemetry,
  computeSystemHealthSummary,
  getModeMeta,
  TransactionStage,
} from '../engine/autonomousEngine';

interface Props {
  telemetry: AutonomousTelemetry;
  nativeMode?: boolean;
  thermalTripCelsius: number;
  onTriggerSimulation?: (forced?: 'cpu' | 'memory' | 'io_trial') => void;
}

const STAGES: { key: TransactionStage; label: string }[] = [
  { key: 'PLANNED', label: 'Planned' },
  { key: 'APPLIED', label: 'Applied' },
  { key: 'VERIFIED', label: 'Readback Verified' },
  { key: 'OBSERVING', label: 'Observing' },
  { key: 'RETAINED', label: 'Retained' },
];

export const AutonomousStateHero: React.FC<Props> = ({
  telemetry,
  nativeMode = false,
  thermalTripCelsius,
  onTriggerSimulation,
}) => {
  const meta = getModeMeta(telemetry.mode);
  const health = computeSystemHealthSummary(telemetry, thermalTripCelsius);
  const temp = telemetry.socTempCelsius;
  const tempText = nativeMode && temp < 0 ? 'N/A' : temp.toFixed(1);
  const rawStage = telemetry.tunerStage ?? telemetry.transactionStage ?? meta.stage;
  const currentStage: TransactionStage = telemetry.engineActive
    ? STAGES.some((s) => s.key === rawStage)
      ? (rawStage as TransactionStage)
      : 'RETAINED'
    : 'PLANNED';

  const primaryQueue = telemetry.blockQueues?.[0];
  const candidates = telemetry.tunerCandidates ?? [];
  const evaluatedCandidates = telemetry.tunerEvaluatedCandidates ?? [];
  const activeInterventions = telemetry.tunerActiveInterventions ?? [];
  const rankedEvaluations = [...evaluatedCandidates].sort(
    (a, b) => b.utilityScore - a.utilityScore || b.confidence - a.confidence
  );
  const rankedArms = [...candidates].sort((a, b) => b.ucbScore - a.ucbScore);
  const utilityDelta = telemetry.tunerUtilityDelta ?? 0;
  const configuredUclampMin = telemetry.uclampPolicyMin ?? telemetry.uclampMin;
  const configuredUclampMax = telemetry.uclampPolicyMax ?? telemetry.uclampMax;

  const thermalToneBadge =
    health.thermalTone === 'guard'
      ? 'md3-badge md3-badge-error'
      : health.thermalTone === 'elevated'
        ? 'md3-badge md3-badge-warning'
        : 'md3-badge md3-badge-success';

  const pressureToneBadge =
    health.pressureTone === 'critical'
      ? 'md3-badge md3-badge-warning'
      : health.pressureTone === 'contended'
        ? 'md3-badge md3-badge-secondary'
        : 'md3-badge md3-badge-success';

  return (
    <div className="md3-card-elevated p-6 sm:p-8 space-y-6">
      {/* Top Expressive Hero Row */}
      <div
        className="flex flex-col lg:flex-row lg:items-start justify-between gap-6 pb-6"
        style={{ borderBottom: '1px solid var(--md-sys-color-outline-variant)' }}
      >
        <div className="space-y-2.5 max-w-2xl">
          <div className="flex flex-wrap items-center gap-2">
            <span className="md3-badge md3-badge-primary">
              {nativeMode
                ? telemetry.engineActive
                  ? 'NATIVE RUNTIME ACTIVE'
                  : 'NATIVE STANDBY'
                : `${meta.label.toUpperCase()} (SIM)`}
            </span>
            <span className={thermalToneBadge}>
              THERMAL: {health.thermalTone.toUpperCase()}
            </span>
            <span className={pressureToneBadge}>
              PSI: {health.pressureTone.toUpperCase()}
            </span>
            <span className="md3-badge md3-badge-neutral font-mono">
              RISK: {telemetry.riskClass ?? meta.risk}
            </span>
          </div>

          <h2 className="text-xl sm:text-2xl font-bold tracking-tight leading-snug">
            {nativeMode ? telemetry.modeDescription : meta.description}
          </h2>

          <p
            className="text-xs sm:text-sm leading-relaxed"
            style={{ color: 'var(--md-sys-color-on-surface-variant)' }}
          >
            {health.headline} Every kernel mutation is gated by capability discovery, write-ahead CFJ4 journaling, synchronous readback verification, and empirical before/after outcome scoring.
          </p>

          {health.activeDomains.length > 0 && (
            <div className="flex flex-wrap items-center gap-1.5 pt-1">
              <span
                className="text-[11px] font-semibold mr-1"
                style={{ color: 'var(--md-sys-color-on-surface-variant)' }}
              >
                Active Domains:
              </span>
              {health.activeDomains.map((dom) => (
                <span key={dom} className="md3-badge md3-badge-secondary font-mono">
                  {dom}
                </span>
              ))}
            </div>
          )}
        </div>

        {/* M3 Equilibrium & Safety Hierarchy Panel */}
        <div className="md3-card-filled p-4 sm:p-5 shrink-0 lg:w-72 space-y-3">
          <div className="flex items-center justify-between">
            <span
              className="text-xs font-bold uppercase tracking-wider"
              style={{ color: 'var(--md-sys-color-on-surface-variant)' }}
            >
              Equilibrium Index
            </span>
            <span
              className="font-mono text-2xl font-bold tabular-nums"
              style={{ color: 'var(--md-sys-color-primary)' }}
            >
              {health.equilibriumScore}%
            </span>
          </div>
          <div className="md3-progress-track">
            <div
              className="md3-progress-bar"
              style={{ width: `${health.equilibriumScore}%` }}
            />
          </div>
          <div
            className="pt-2 space-y-1 text-xs"
            style={{ borderTop: '1px solid var(--md-sys-color-outline-variant)' }}
          >
            <div className="font-semibold">
              OEM Thermal → Pressure → Load → Balanced
            </div>
            <div
              className="font-mono text-[11px]"
              style={{ color: 'var(--md-sys-color-on-surface-variant)' }}
            >
              Verdict: {telemetry.tunerVerdict ?? telemetry.lastOutcome ?? meta.outcome} · Ownership:{' '}
              {telemetry.ownershipIntact !== false ? 'Intact' : 'Contended'}
            </div>
          </div>
        </div>
      </div>

      {/* Interactive Simulation Workload Trigger Bar (Browser Preview Mode Only) */}
      {!nativeMode && onTriggerSimulation && (
        <div className="md3-card-filled px-4 py-3.5 flex flex-col sm:flex-row sm:items-center justify-between gap-3">
          <div className="flex items-center gap-2.5 text-xs">
            <Sparkles className="w-4 h-4 shrink-0" style={{ color: 'var(--md-sys-color-primary)' }} />
            <div>
              <span className="font-bold">Interactive Preview Workload Simulator</span>
              <span
                className="hidden md:inline"
                style={{ color: 'var(--md-sys-color-on-surface-variant)' }}
              >
                {' '}· Inject synthetic pressure spikes to observe M3 transaction stages and bandit decisions
              </span>
            </div>
          </div>
          <div className="flex flex-wrap items-center gap-1.5">
            <button
              type="button"
              id="btn-sim-cpu"
              onClick={() => onTriggerSimulation('cpu')}
              className="md3-chip"
            >
              <Cpu className="w-3.5 h-3.5" />
              <span>CPU Burst</span>
            </button>
            <button
              type="button"
              id="btn-sim-memory"
              onClick={() => onTriggerSimulation('memory')}
              className="md3-chip"
            >
              <Activity className="w-3.5 h-3.5" />
              <span>Memory Stall</span>
            </button>
            <button
              type="button"
              id="btn-sim-io"
              onClick={() => onTriggerSimulation('io_trial')}
              className="md3-chip"
            >
              <HardDrive className="w-3.5 h-3.5" />
              <span>Block I/O Trial</span>
            </button>
            <button
              type="button"
              id="btn-sim-step"
              onClick={() => onTriggerSimulation(undefined)}
              className="md3-btn-tonal !min-h-[2rem] !px-3 !text-xs"
            >
              <Play className="w-3.5 h-3.5" />
              <span>Step Tick</span>
            </button>
          </div>
        </div>
      )}

      {/* Material Design 3 5-Stage Transaction Pipeline Stepper */}
      <div
        className="space-y-3 pb-6"
        style={{ borderBottom: '1px solid var(--md-sys-color-outline-variant)' }}
      >
        <div className="flex flex-wrap items-center justify-between gap-2">
          <span className="text-xs font-bold flex items-center gap-1.5">
            <GitCommit className="w-4 h-4" style={{ color: 'var(--md-sys-color-primary)' }} />
            <span>Transaction Lifecycle Pipeline{!nativeMode ? ' (sim)' : ''}</span>
          </span>
          <div className="flex flex-wrap items-center gap-2 text-xs font-mono">
            {telemetry.tunerWorkload && (
              <span className="md3-badge md3-badge-neutral">
                Workload: {telemetry.tunerWorkload}
              </span>
            )}
            {telemetry.tunerObjective && (
              <span className="md3-badge md3-badge-neutral">
                Objective: {telemetry.tunerObjective}
              </span>
            )}
            <span className="md3-badge md3-badge-primary">
              Stage: {telemetry.tunerStage ?? currentStage}
            </span>
          </div>
        </div>

        <div className="grid grid-cols-2 sm:grid-cols-5 gap-2">
          {STAGES.map((item, idx) => {
            const activeIdx = STAGES.findIndex((s) => s.key === currentStage);
            const isCurrent = item.key === currentStage;
            const isPassed = idx < activeIdx;
            return (
              <div
                key={item.key}
                className="px-3.5 py-2.5 rounded-2xl text-xs flex items-center justify-between transition-all"
                style={{
                  backgroundColor: isCurrent
                    ? 'var(--md-sys-color-primary-container)'
                    : isPassed
                      ? 'var(--md-sys-color-surface-container-high)'
                      : 'var(--md-sys-color-surface-container)',
                  color: isCurrent
                    ? 'var(--md-sys-color-on-primary-container)'
                    : isPassed
                      ? 'var(--md-sys-color-on-surface)'
                      : 'var(--md-sys-color-on-surface-variant)',
                  fontWeight: isCurrent ? 700 : 600,
                }}
              >
                <span className="truncate">
                  {idx + 1}. {item.label}
                </span>
                {isCurrent && (
                  <span
                    className="h-2 w-2 rounded-full shrink-0"
                    style={{ backgroundColor: 'var(--md-sys-color-primary)' }}
                  />
                )}
              </div>
            );
          })}
        </div>
      </div>

      {/* 4-Column Material 3 Metric Grid */}
      <div className="grid grid-cols-1 sm:grid-cols-2 lg:grid-cols-4 gap-4">
        {/* 1. Foreground UClamp Readback */}
        <div className="md3-card-filled p-4 flex flex-col justify-between">
          <div>
            <div
              className="flex items-center justify-between text-xs font-semibold mb-2"
              style={{ color: 'var(--md-sys-color-on-surface-variant)' }}
            >
              <span>Foreground UClamp · Readback</span>
              <Zap className="w-4 h-4" style={{ color: 'var(--md-sys-color-primary)' }} />
            </div>
            <div className="text-2xl font-bold font-mono tabular-nums">
              {telemetry.uclampSupported ? telemetry.uclampMin : '—'}
              <span
                className="text-xs font-normal"
                style={{ color: 'var(--md-sys-color-on-surface-variant)' }}
              >
                {' '}
                / {telemetry.uclampSupported ? telemetry.uclampMax : '—'}
              </span>
            </div>
          </div>

          <div
            className="mt-3 grid grid-cols-2 gap-2 pt-3 text-[11px]"
            style={{ borderTop: '1px solid var(--md-sys-color-outline-variant)' }}
          >
            <div>
              <div style={{ color: 'var(--md-sys-color-on-surface-variant)' }}>Policy base</div>
              <div className="mt-0.5 font-mono font-bold">
                {configuredUclampMin} / {configuredUclampMax}
              </div>
            </div>
            <div>
              <div style={{ color: 'var(--md-sys-color-on-surface-variant)' }}>ABI encoding</div>
              <div className="mt-0.5 font-mono font-bold">
                {telemetry.uclampEncoding ?? 'percentage'}
              </div>
            </div>
          </div>

          <div
            className="mt-2 truncate text-[10px] font-mono"
            style={{ color: 'var(--md-sys-color-on-surface-variant)' }}
            title={telemetry.uclampTargetPath ?? ''}
          >
            {telemetry.uclampSupported
              ? telemetry.uclampTargetPath || 'Verified foreground cgroup'
              : 'Foreground cgroup capability unavailable'}
          </div>
        </div>

        {/* 2. SoC Thermal Composite */}
        <div className="md3-card-filled p-4 flex flex-col justify-between">
          <div>
            <div
              className="flex items-center justify-between text-xs font-semibold mb-2"
              style={{ color: 'var(--md-sys-color-on-surface-variant)' }}
            >
              <span>SoC Thermal Composite{!nativeMode ? ' (sim)' : ''}</span>
              <Flame className="w-4 h-4" style={{ color: 'var(--md-sys-color-primary)' }} />
            </div>
            <div className="text-2xl font-bold font-mono tabular-nums">
              {tempText}
              <span
                className="text-xs font-normal"
                style={{ color: 'var(--md-sys-color-on-surface-variant)' }}
              >
                {' '}
                °C
              </span>
            </div>
          </div>

          <div className="my-2.5">
            <div className="md3-progress-track">
              <div
                className="md3-progress-bar"
                style={{
                  width: `${Math.max(8, Math.min(100, temp > 0 ? ((temp - 28) / 28) * 100 : 12))}%`,
                }}
              />
            </div>
          </div>

          <div
            className="text-[11px] leading-normal"
            style={{ color: 'var(--md-sys-color-on-surface-variant)' }}
          >
            {nativeMode
              ? `Source: ${telemetry.thermalSource ?? 'OEM policy & sensor fusion'}`
              : `Trip gate ${thermalTripCelsius.toFixed(1)} °C · CPU peak ${(telemetry.cpuPeakCelsius ?? temp).toFixed(1)} °C`}
          </div>
        </div>

        {/* 3. PSI Resource Pressure */}
        <div className="md3-card-filled p-4 flex flex-col justify-between">
          <div
            className="flex items-center justify-between text-xs font-semibold mb-2"
            style={{ color: 'var(--md-sys-color-on-surface-variant)' }}
          >
            <span>PSI Resource Pressure{!nativeMode ? ' (sim)' : ''}</span>
            <Gauge className="w-4 h-4" style={{ color: 'var(--md-sys-color-primary)' }} />
          </div>

          <div className="grid grid-cols-3 gap-2 font-mono tabular-nums text-xs my-1">
            <div>
              <div
                className="text-[10px] font-sans"
                style={{ color: 'var(--md-sys-color-on-surface-variant)' }}
              >
                CPU
              </div>
              <div className="text-sm font-bold">
                {telemetry.cpuPressureValid === false
                  ? 'N/A'
                  : `${telemetry.cpuPressurePct.toFixed(1)}%`}
              </div>
            </div>
            <div>
              <div
                className="text-[10px] font-sans"
                style={{ color: 'var(--md-sys-color-on-surface-variant)' }}
              >
                MEM
              </div>
              <div className="text-sm font-bold">
                {telemetry.memoryPressureValid === false
                  ? 'N/A'
                  : `${telemetry.memoryPressurePct.toFixed(1)}%`}
              </div>
            </div>
            <div>
              <div
                className="text-[10px] font-sans"
                style={{ color: 'var(--md-sys-color-on-surface-variant)' }}
              >
                I/O
              </div>
              <div className="text-sm font-bold">
                {telemetry.ioPressureValid === false
                  ? 'N/A'
                  : `${telemetry.ioPressurePct.toFixed(1)}%`}
              </div>
            </div>
          </div>

          <div
            className="text-[11px] mt-2 leading-normal"
            style={{ color: 'var(--md-sys-color-on-surface-variant)' }}
          >
            Linux <code className="font-mono">/proc/pressure</code> avg10 window
          </div>
        </div>

        {/* 4. Durable Journal */}
        <div className="md3-card-filled p-4 flex flex-col justify-between">
          <div>
            <div
              className="flex items-center justify-between text-xs font-semibold mb-2"
              style={{ color: 'var(--md-sys-color-on-surface-variant)' }}
            >
              <span>CFJ4 Durable Journal{!nativeMode ? ' (sim)' : ''}</span>
              <ShieldCheck className="w-4 h-4" style={{ color: 'var(--md-sys-color-primary)' }} />
            </div>
            <div className="text-lg font-bold">
              {telemetry.baselineStatus === 'protected'
                ? 'Journal Armed'
                : 'Baseline Restored'}
            </div>
          </div>

          <div
            className="text-[11px] mt-2 leading-normal"
            style={{ color: 'var(--md-sys-color-on-surface-variant)' }}
          >
            {telemetry.baselineNodesSnapshotted > 0
              ? `${telemetry.baselineNodesSnapshotted} owned kernel nodes verified`
              : 'Write-ahead & readback verified'}
          </div>
        </div>
      </div>

      {/* Contract-Gated Autonomous Tuning Evaluator & Multi-Domain Co-Execution */}
      <div className="md3-card-filled p-5 space-y-4">
        <div
          className="flex flex-col sm:flex-row sm:items-start justify-between gap-3 pb-3.5"
          style={{ borderBottom: '1px solid var(--md-sys-color-outline-variant)' }}
        >
          <div className="space-y-1 min-w-0">
            <div className="flex flex-wrap items-center gap-2 text-xs">
              <Sliders className="w-4 h-4" style={{ color: 'var(--md-sys-color-primary)' }} />
              <span className="font-bold text-sm">
                Autonomous Contract-Gated Tuning Evaluator{!nativeMode ? ' (sim)' : ''}
              </span>
              <span className="md3-badge md3-badge-primary font-mono">
                Intent: {telemetry.tunerIntent ?? 'HoldEquilibrium'}
              </span>
            </div>
            <p
              className="text-xs leading-relaxed"
              style={{ color: 'var(--md-sys-color-on-surface-variant)' }}
            >
              {telemetry.tunerRationale ??
                'Evaluates workload-specific candidates only after the kernel contract, access path, valid range, and baseline have been verified.'}
            </p>
          </div>

          <div className="grid grid-cols-3 gap-4 shrink-0 font-mono tabular-nums text-xs">
            <div>
              <div
                className="text-[10px] font-sans"
                style={{ color: 'var(--md-sys-color-on-surface-variant)' }}
              >
                Empirical ΔU
              </div>
              <div className="font-bold text-sm mt-0.5">
                {utilityDelta >= 0
                  ? `+${utilityDelta.toFixed(2)}`
                  : utilityDelta.toFixed(2)}
              </div>
            </div>
            <div>
              <div
                className="text-[10px] font-sans"
                style={{ color: 'var(--md-sys-color-on-surface-variant)' }}
              >
                Retained
              </div>
              <div className="font-bold text-sm mt-0.5">
                {telemetry.tunerImproved ?? 0} / {telemetry.tunerTrials ?? 0}
              </div>
            </div>
            <div>
              <div
                className="text-[10px] font-sans"
                style={{ color: 'var(--md-sys-color-on-surface-variant)' }}
              >
                Rolled Back
              </div>
              <div className="font-bold text-sm mt-0.5">
                {telemetry.tunerRolledBack ?? 0}
              </div>
            </div>
          </div>
        </div>

        {/* Active Co-Executing Interventions (if any reported by daemon) */}
        {activeInterventions.length > 0 && (
          <div className="space-y-2">
            <div className="text-xs font-bold">
              Active Co-Executing Interventions ({activeInterventions.length})
            </div>
            <div className="grid grid-cols-1 md:grid-cols-2 gap-2.5">
              {activeInterventions.map((iv) => (
                <div
                  key={`${iv.domain}-${iv.candidateId}`}
                  className="md3-card-elevated p-3.5 text-xs space-y-1.5"
                >
                  <div className="flex items-center justify-between gap-2">
                    <span className="font-mono font-bold truncate">{iv.candidateId}</span>
                    <span className="md3-badge md3-badge-primary font-mono">{iv.stage}</span>
                  </div>
                  <div style={{ color: 'var(--md-sys-color-on-surface-variant)' }}>
                    {iv.candidateLabel}
                  </div>
                  <div
                    className="flex flex-wrap items-center gap-3 font-mono text-[10px]"
                    style={{ color: 'var(--md-sys-color-on-surface-variant)' }}
                  >
                    <span>Domain: {iv.domain}</span>
                    <span>Verdict: {iv.verdict}</span>
                    <span>
                      Window: {iv.observationTick}/{iv.observationWindow}
                    </span>
                  </div>
                </div>
              ))}
            </div>
          </div>
        )}

        {/* Block Device Queue Surface & Global Candidate Ranking */}
        <div className="grid grid-cols-1 lg:grid-cols-2 gap-4">
          <div className="md3-card-elevated p-4 space-y-3">
            <div className="flex flex-wrap items-center justify-between gap-2 text-xs">
              <span className="font-bold flex items-center gap-1.5">
                <HardDrive className="w-3.5 h-3.5" style={{ color: 'var(--md-sys-color-primary)' }} />
                <span>
                  Block Queue Surface{!nativeMode ? ' (sim)' : ''} (
                  {primaryQueue ? `/sys/block/${primaryQueue.device}/queue` : 'Awaiting probe'})
                </span>
              </span>
              <span
                className="font-mono text-[11px]"
                style={{ color: 'var(--md-sys-color-on-surface-variant)' }}
              >
                OEM: {primaryQueue?.baselineScheduler ?? 'N/A'} → Active:{' '}
                <strong style={{ color: 'var(--md-sys-color-primary)' }}>
                  {primaryQueue?.activeScheduler ?? 'N/A'}
                </strong>
              </span>
            </div>

            <div className="grid grid-cols-3 sm:grid-cols-6 gap-2 pt-1 font-mono tabular-nums text-xs">
              <div>
                <div
                  className="text-[10px] font-sans"
                  style={{ color: 'var(--md-sys-color-on-surface-variant)' }}
                >
                  read_expire
                </div>
                <div className="font-bold">
                  {primaryQueue && primaryQueue.readExpireMs >= 0
                    ? `${primaryQueue.readExpireMs}ms`
                    : 'N/A'}
                </div>
              </div>
              <div>
                <div
                  className="text-[10px] font-sans"
                  style={{ color: 'var(--md-sys-color-on-surface-variant)' }}
                >
                  writes_starved
                </div>
                <div className="font-bold">
                  {primaryQueue && primaryQueue.writesStarved >= 0
                    ? primaryQueue.writesStarved
                    : 'N/A'}
                </div>
              </div>
              <div>
                <div
                  className="text-[10px] font-sans"
                  style={{ color: 'var(--md-sys-color-on-surface-variant)' }}
                >
                  fifo_batch
                </div>
                <div className="font-bold">
                  {primaryQueue && primaryQueue.fifoBatch >= 0
                    ? primaryQueue.fifoBatch
                    : 'N/A'}
                </div>
              </div>
              <div>
                <div
                  className="text-[10px] font-sans"
                  style={{ color: 'var(--md-sys-color-on-surface-variant)' }}
                >
                  read_ahead
                </div>
                <div className="font-bold">
                  {primaryQueue && primaryQueue.readAheadKb >= 0
                    ? `${primaryQueue.readAheadKb}KB`
                    : 'N/A'}
                </div>
              </div>
              <div>
                <div
                  className="text-[10px] font-sans"
                  style={{ color: 'var(--md-sys-color-on-surface-variant)' }}
                >
                  nr_requests
                </div>
                <div className="font-bold">
                  {primaryQueue && primaryQueue.nrRequests >= 0
                    ? primaryQueue.nrRequests
                    : 'N/A'}
                </div>
              </div>
              <div>
                <div
                  className="text-[10px] font-sans"
                  style={{ color: 'var(--md-sys-color-on-surface-variant)' }}
                >
                  rq_affinity
                </div>
                <div className="font-bold">
                  {primaryQueue && primaryQueue.rqAffinity >= 0
                    ? primaryQueue.rqAffinity
                    : 'N/A'}
                </div>
              </div>
            </div>
          </div>

          <div className="md3-card-elevated p-4 space-y-3">
            <div className="flex flex-wrap items-center justify-between gap-2 text-xs">
              <span className="font-bold">Global Adaptive Candidate Ranking</span>
              <span
                className="font-mono text-[11px]"
                style={{ color: 'var(--md-sys-color-on-surface-variant)' }}
              >
                Active: {telemetry.tunerCandidateId ?? 'none'}
              </span>
            </div>
            <div className="max-h-60 space-y-2 overflow-y-auto pr-1">
              {rankedEvaluations.length > 0
                ? rankedEvaluations.map((candidate, index) => {
                    const arm = candidates.find((item) => item.id === candidate.id);
                    const isActive =
                      candidate.selected || candidate.id === telemetry.tunerCandidateId;
                    return (
                      <div
                        key={candidate.id}
                        className="rounded-xl p-3 text-xs space-y-1.5"
                        style={{
                          backgroundColor: isActive
                            ? 'var(--md-sys-color-primary-container)'
                            : 'var(--md-sys-color-surface-container)',
                          color: isActive
                            ? 'var(--md-sys-color-on-primary-container)'
                            : 'var(--md-sys-color-on-surface)',
                        }}
                      >
                        <div className="flex items-start justify-between gap-3">
                          <div className="min-w-0">
                            <div className="flex flex-wrap items-center gap-1.5 text-[10px] font-mono">
                              <span>#{index + 1}</span>
                              <span>· {candidate.domain}</span>
                              {isActive && (
                                <span className="md3-badge md3-badge-primary">ACTIVE</span>
                              )}
                              {!candidate.eligible && (
                                <span className="md3-badge md3-badge-warning">INELIGIBLE</span>
                              )}
                            </div>
                            <div className="mt-0.5 break-all font-bold">{candidate.id}</div>
                          </div>
                          <div className="shrink-0 text-right font-mono tabular-nums">
                            <div className="text-sm font-bold">
                              {candidate.utilityScore.toFixed(1)}
                            </div>
                            <div className="text-[9px] opacity-75">utility</div>
                          </div>
                        </div>
                        <div className="flex flex-wrap gap-x-3 gap-y-1 font-mono text-[10px] opacity-80">
                          <span>UCB {candidate.empiricalScore.toFixed(3)}</span>
                          <span>conf {Math.round(candidate.confidence * 100)}%</span>
                          {arm && <span>retained {arm.successes}</span>}
                          {arm && <span>rollback {arm.rollbacks}</span>}
                        </div>
                      </div>
                    );
                  })
                : rankedArms.map((arm, index) => (
                    <div
                      key={arm.id}
                      className="flex items-center justify-between gap-3 rounded-xl p-2.5"
                      style={{ backgroundColor: 'var(--md-sys-color-surface-container)' }}
                    >
                      <div className="min-w-0">
                        <div
                          className="text-[10px] font-mono"
                          style={{ color: 'var(--md-sys-color-on-surface-variant)' }}
                        >
                          #{index + 1} · {arm.domain}
                        </div>
                        <div className="truncate text-xs font-bold">{arm.id}</div>
                        <div
                          className="text-[10px]"
                          style={{ color: 'var(--md-sys-color-on-surface-variant)' }}
                        >
                          retained {arm.successes} · rollback {arm.rollbacks} · trials {arm.trials}
                        </div>
                      </div>
                      <div className="shrink-0 text-right font-mono">
                        <div className="text-sm font-bold">{arm.ucbScore.toFixed(3)}</div>
                        <div
                          className="text-[9px]"
                          style={{ color: 'var(--md-sys-color-on-surface-variant)' }}
                        >
                          UCB1
                        </div>
                      </div>
                    </div>
                  ))}
            </div>
          </div>
        </div>
      </div>

      {/* Hybrid ONNX / Online RLS-MLP Neural Horizon Predictor Strip */}
      <div className="md3-card-filled p-4 sm:p-5 flex flex-col sm:flex-row sm:items-center justify-between gap-4">
        <div className="space-y-1 min-w-0">
          <div className="flex flex-wrap items-center gap-2 text-xs">
            <span className="font-bold text-sm">
              Neural Horizon Predictor (ONNX / Online RLS-MLP){!nativeMode ? ' (sim)' : ''}
            </span>
            <span className="md3-badge md3-badge-primary font-mono">
              {telemetry.aiBackend ?? 'embedded-rls-mlp'}
            </span>
          </div>
          <p
            className="text-xs leading-relaxed"
            style={{ color: 'var(--md-sys-color-on-surface-variant)' }}
          >
            Forecasts 10-second thermal trajectory via Online Recursive Least Squares + 6×8×2 Micro-MLP with NLMS output-head adaptation, pre-pacing capacity strictly within Safety Admission invariants.
          </p>
        </div>

        <div className="grid grid-cols-4 gap-4 shrink-0 font-mono tabular-nums text-xs">
          <div>
            <div
              className="text-[10px] font-sans"
              style={{ color: 'var(--md-sys-color-on-surface-variant)' }}
            >
              10s Forecast
            </div>
            <div className="font-bold text-sm mt-0.5">
              {(telemetry.aiPredictedPressure10s ?? 45.0).toFixed(1)}
            </div>
          </div>
          <div>
            <div
              className="text-[10px] font-sans"
              style={{ color: 'var(--md-sys-color-on-surface-variant)' }}
            >
              Velocity
            </div>
            <div className="font-bold text-sm mt-0.5">
              {(telemetry.aiPredictedVelocity ?? 0) >= 0
                ? `+${(telemetry.aiPredictedVelocity ?? 0).toFixed(2)}/s`
                : `${(telemetry.aiPredictedVelocity ?? 0).toFixed(2)}/s`}
            </div>
          </div>
          <div>
            <div
              className="text-[10px] font-sans"
              style={{ color: 'var(--md-sys-color-on-surface-variant)' }}
            >
              Scale Hint
            </div>
            <div className="font-bold text-sm mt-0.5">
              {Math.round((telemetry.aiRecommendedScale ?? 1.0) * 100)}%
            </div>
          </div>
          <div>
            <div
              className="text-[10px] font-sans"
              style={{ color: 'var(--md-sys-color-on-surface-variant)' }}
            >
              Confidence
            </div>
            <div
              className="font-bold text-sm mt-0.5"
              style={{ color: 'var(--md-sys-color-primary)' }}
            >
              {Math.round((telemetry.aiConfidence ?? 0.85) * 100)}%
            </div>
          </div>
        </div>
      </div>
    </div>
  );
};
