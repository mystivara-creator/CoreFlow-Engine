import React from 'react';
import { Flame, Gauge, ShieldCheck, Zap, GitCommit, HardDrive, Sliders } from 'lucide-react';
import { AutonomousTelemetry, getModeMeta, TransactionStage } from '../engine/autonomousEngine';

interface Props {
  telemetry: AutonomousTelemetry;
  nativeMode?: boolean;
  thermalTripCelsius: number;
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
}) => {
  const meta = getModeMeta(telemetry.mode);
  const temp = telemetry.socTempCelsius;
  const tempText = nativeMode && temp < 0 ? 'N/A' : temp.toFixed(1);
  const rawStage = telemetry.tunerStage ?? telemetry.transactionStage ?? meta.stage;
  const currentStage: TransactionStage = telemetry.engineActive
    ? (STAGES.some((s) => s.key === rawStage) ? (rawStage as TransactionStage) : 'RETAINED')
    : 'PLANNED';

  const primaryQueue = telemetry.blockQueues?.[0];
  const candidates = telemetry.tunerCandidates ?? [];
  const evaluatedCandidates = telemetry.tunerEvaluatedCandidates ?? [];
  const rankedEvaluations = [...evaluatedCandidates].sort(
    (a, b) => b.utilityScore - a.utilityScore || b.confidence - a.confidence
  );
  const rankedArms = [...candidates].sort((a, b) => b.ucbScore - a.ucbScore);
  const utilityDelta = telemetry.tunerUtilityDelta ?? 0;
  const configuredUclampMin = telemetry.uclampPolicyMin ?? telemetry.uclampMin;
  const configuredUclampMax = telemetry.uclampPolicyMax ?? telemetry.uclampMax;

  return (
    <div className="m3-card p-6 sm:p-8 relative overflow-hidden">
      {/* Subtle Material 3 Tonal Ambient Blur */}
      <div
        aria-hidden="true"
        className="pointer-events-none absolute -top-28 -right-24 w-96 h-96 rounded-full bg-[#0b57d0]/10 dark:bg-[#a8c7fa]/10 blur-3xl"
      />

      {/* Block-Aligned Header Row */}
      <div className="relative z-10 flex flex-col lg:flex-row lg:items-start justify-between gap-6 pb-6 border-b border-slate-900/[0.06] dark:border-white/[0.06]">
        <div className="space-y-2 max-w-2xl">
          <div className="flex flex-wrap items-center gap-2 text-xs text-slate-500 dark:text-slate-400">
            <span className="font-semibold text-[#0b57d0] dark:text-[#a8c7fa]">
              Coordinated Mutation Transaction{!nativeMode ? ' (sim)' : ''}
            </span>
            <span aria-hidden="true">·</span>
            <span>{nativeMode ? (telemetry.engineActive ? 'Native Runtime' : 'Native Standby') : meta.label}</span>
            <span aria-hidden="true">·</span>
            <span className="font-mono">Risk Class: {telemetry.riskClass ?? meta.risk}</span>
          </div>
          <h2 className="text-xl sm:text-2xl font-semibold tracking-tight text-slate-900 dark:text-slate-100 leading-snug">
            {nativeMode ? telemetry.modeDescription : meta.description}
          </h2>
          <p className="text-xs sm:text-sm text-slate-600 dark:text-slate-400 leading-relaxed">
            Mutations are coordinated by optimization intent, dependency provenance, and risk budget. Every kernel write requires a valid baseline, immediate readback verification, and empirical Before/After outcome validation.
          </p>
        </div>

        <div className="m3-tonal-block px-4 py-3.5 shrink-0 lg:min-w-[250px]">
          <div className="text-[11px] font-medium text-slate-500 dark:text-slate-400 mb-1">
            Safety Admission Hierarchy
          </div>
          <div className="text-xs font-semibold text-slate-800 dark:text-slate-200 tracking-tight">
            OEM Thermal → Pressure → Load → Balanced
          </div>
          <div className="text-[11px] font-mono text-slate-500 dark:text-slate-400 mt-1">
            Verdict: {telemetry.tunerVerdict ?? telemetry.lastOutcome ?? meta.outcome} · Ownership: {telemetry.ownershipIntact !== false ? 'Valid' : 'Released'}
          </div>
        </div>
      </div>

      {/* Transaction State Machine Pipeline Bar */}
      <div className="relative z-10 py-5 border-b border-slate-900/[0.06] dark:border-white/[0.06]">
        <div className="flex items-center justify-between gap-2 mb-3">
          <span className="text-xs font-medium text-slate-600 dark:text-slate-300 flex items-center gap-1.5">
            <GitCommit className="w-3.5 h-3.5 text-[#0b57d0] dark:text-[#a8c7fa]" />
            Transaction Lifecycle State{!nativeMode ? ' (sim)' : ''}
          </span>
          <span className="text-xs font-mono text-slate-500 dark:text-slate-400">
            Stage{!nativeMode ? ' (sim)' : ''}: {telemetry.tunerStage ?? currentStage}
          </span>
          {nativeMode && telemetry.tunerRuntimeStage && (
            <span className="text-xs font-mono text-slate-500 dark:text-slate-400">
              Runtime: {telemetry.tunerRuntimeStage.replace('STAGE_', '')}
            </span>
          )}
        </div>
        <div className="grid grid-cols-2 sm:grid-cols-5 gap-2">
          {STAGES.map((item, idx) => {
            const activeIdx = STAGES.findIndex((s) => s.key === currentStage);
            const isCurrent = item.key === currentStage;
            const isPassed = idx < activeIdx;
            return (
              <div
                key={item.key}
                className={
                  'px-3 py-2 rounded-2xl text-xs transition-all flex items-center justify-between ' +
                  (isCurrent
                    ? 'bg-[#d3e3fd] text-[#041e49] dark:bg-[#0842a0] dark:text-[#d3e3fd] font-semibold'
                    : isPassed
                    ? 'bg-slate-900/[0.04] dark:bg-white/[0.05] text-slate-700 dark:text-slate-300 font-medium'
                    : 'bg-slate-900/[0.02] dark:bg-white/[0.02] text-slate-400 dark:text-slate-500')
                }
              >
                <span className="truncate">{idx + 1}. {item.label}</span>
              </div>
            );
          })}
        </div>
      </div>

      {/* 4-Column Metric Grid with Block Alignment */}
      <div className="relative z-10 grid grid-cols-1 sm:grid-cols-2 lg:grid-cols-4 gap-4 pt-6">
        <div className="m3-tonal-block p-4 flex flex-col justify-between pixel-uclamp-card">
          <div className="flex items-center justify-between text-xs text-slate-500 dark:text-slate-400 mb-2">
            <span className="font-medium">Foreground UClamp · Kernel Readback</span>
            <Zap className="w-4 h-4 text-[#0b57d0] dark:text-[#a8c7fa]" />
          </div>
          <div className="text-2xl font-semibold font-mono tabular-nums text-slate-900 dark:text-slate-100">
            {telemetry.uclampSupported ? telemetry.uclampMin : '—'}
            <span className="text-xs font-normal text-slate-500 dark:text-slate-400"> / {telemetry.uclampSupported ? telemetry.uclampMax : '—'}</span>
          </div>
          <div className="mt-3 grid grid-cols-2 gap-2 border-t border-slate-900/[0.06] dark:border-white/[0.07] pt-3 text-[11px]">
            <div>
              <div className="text-slate-500 dark:text-slate-400">Configured base</div>
              <div className="mt-0.5 font-mono font-semibold text-slate-800 dark:text-slate-200">
                {configuredUclampMin} / {configuredUclampMax}
              </div>
            </div>
            <div>
              <div className="text-slate-500 dark:text-slate-400">ABI encoding</div>
              <div className="mt-0.5 font-mono font-semibold text-slate-800 dark:text-slate-200">
                {telemetry.uclampEncoding ?? 'unknown'}
              </div>
            </div>
          </div>
          <div className="mt-2 truncate text-[10px] font-mono text-slate-500 dark:text-slate-400" title={telemetry.uclampTargetPath ?? ''}>
            {telemetry.uclampSupported
              ? (telemetry.uclampTargetPath || 'Verified foreground cgroup')
              : 'Foreground cgroup capability unavailable'}
          </div>
          <div className="mt-1 text-[10px] leading-relaxed text-slate-500 dark:text-slate-400">
            {telemetry.uclampSupported
              ? 'Live values are read back from the resolved cgroup. Workload, pressure, and thermal budgets may change them from the configured base.'
              : 'No verified writable UClamp target; no mutation is claimed.'}
          </div>
        </div>

        <div className="m3-tonal-block p-4 flex flex-col justify-between">
          <div className="flex items-center justify-between text-xs text-slate-500 dark:text-slate-400 mb-2">
            <span className="font-medium">SoC Thermal Composite{!nativeMode ? ' (sim)' : ''}</span>
            <Flame className="w-4 h-4 text-[#0b57d0] dark:text-[#a8c7fa]" />
          </div>
          <div className="text-2xl font-semibold font-mono tabular-nums text-slate-900 dark:text-slate-100">
            {tempText}
            <span className="text-xs font-normal text-slate-500 dark:text-slate-400"> °C</span>
          </div>
          <div className="text-xs text-slate-500 dark:text-slate-400 mt-2 leading-normal">
            {nativeMode ? 'OEM policy & sensor fusion' : `Preview trip threshold ${thermalTripCelsius.toFixed(1)}°C`}
          </div>
        </div>

        <div className="m3-tonal-block p-4 flex flex-col justify-between">
          <div className="flex items-center justify-between text-xs text-slate-500 dark:text-slate-400 mb-2">
            <span className="font-medium">PSI Resource Pressure{!nativeMode ? ' (sim)' : ''}</span>
            <Gauge className="w-4 h-4 text-[#0b57d0] dark:text-[#a8c7fa]" />
          </div>
          <div className="grid grid-cols-3 gap-2 font-mono tabular-nums text-xs text-slate-800 dark:text-slate-200 my-1">
            <div>
              <div className="text-[11px] text-slate-400">CPU</div>
              <div className="font-semibold">{telemetry.cpuPressureValid === false ? 'N/A' : `${telemetry.cpuPressurePct.toFixed(1)}%`}</div>
            </div>
            <div>
              <div className="text-[11px] text-slate-400">MEM</div>
              <div className="font-semibold">{telemetry.memoryPressureValid === false ? 'N/A' : `${telemetry.memoryPressurePct.toFixed(1)}%`}</div>
            </div>
            <div>
              <div className="text-[11px] text-slate-400">I/O</div>
              <div className="font-semibold">{telemetry.ioPressureValid === false ? 'N/A' : `${telemetry.ioPressurePct.toFixed(1)}%`}</div>
            </div>
          </div>
          <div className="text-xs text-slate-500 dark:text-slate-400 mt-1 leading-normal">
            Linux `/proc/pressure` avg10
          </div>
        </div>

        <div className="m3-tonal-block p-4 flex flex-col justify-between">
          <div className="flex items-center justify-between text-xs text-slate-500 dark:text-slate-400 mb-2">
            <span className="font-medium">Durable Journal{!nativeMode ? ' (sim)' : ''}</span>
            <ShieldCheck className="w-4 h-4 text-[#0b57d0] dark:text-[#a8c7fa]" />
          </div>
          <div className="text-lg font-semibold text-slate-900 dark:text-slate-100">
            {telemetry.baselineStatus === 'protected' ? 'Journal Armed' : 'Baseline Restored'}
          </div>
          <div className="text-xs text-slate-500 dark:text-slate-400 mt-2 leading-normal">
            {telemetry.baselineNodesSnapshotted > 0
              ? `${telemetry.baselineNodesSnapshotted} owned kernel nodes verified`
              : 'Write-ahead & readback verified'}
          </div>
        </div>
      </div>

      {/* Contract-gated autonomous tuning and outcome evaluator. */}
      <div className="relative z-10 mt-4 m3-tonal-block p-4 sm:p-5 space-y-4">
        <div className="flex flex-col sm:flex-row sm:items-start justify-between gap-3 border-b border-slate-900/[0.06] dark:border-white/[0.06] pb-3.5">
          <div className="space-y-1 min-w-0">
            <div className="flex flex-wrap items-center gap-2 text-xs">
              <Sliders className="w-3.5 h-3.5 text-[#0b57d0] dark:text-[#a8c7fa]" />
              <span className="font-semibold text-slate-900 dark:text-slate-100">
                Autonomous Contract-Gated Tuning Evaluator{!nativeMode ? ' (sim)' : ''}
              </span>
              <span className="px-2 py-0.5 rounded-full text-[11px] font-mono bg-[#d3e3fd] text-[#041e49] dark:bg-[#0842a0] dark:text-[#d3e3fd]">
                Intent{!nativeMode ? ' (sim)' : ''}: {telemetry.tunerIntent ?? 'HoldEquilibrium'}
              </span>
            </div>
            <p className="text-xs text-slate-600 dark:text-slate-400 leading-relaxed">
              {telemetry.tunerRationale ??
                'Evaluates workload-specific candidates only after the kernel contract, access path, valid range, and baseline have been verified. Observation-only, test-only, protected, virtual, and unknown storage nodes remain outside automatic mutation.'}
            </p>
          </div>

          <div className="grid grid-cols-3 gap-3 sm:gap-4 shrink-0 font-mono tabular-nums text-xs">
            <div>
              <div className="text-[11px] text-slate-400 font-sans">Empirical ΔU{!nativeMode ? ' (sim)' : ''}</div>
              <div className="font-semibold text-sm text-slate-900 dark:text-slate-100 mt-0.5">
                {utilityDelta >= 0 ? `+${utilityDelta.toFixed(2)}` : utilityDelta.toFixed(2)}
              </div>
            </div>
            <div>
              <div className="text-[11px] text-slate-400 font-sans">Retained{!nativeMode ? " (sim)" : ""}</div>
              <div className="font-semibold text-sm text-slate-900 dark:text-slate-100 mt-0.5">
                {telemetry.tunerImproved ?? 0} / {telemetry.tunerTrials ?? 0}
              </div>
            </div>
            <div>
              <div className="text-[11px] text-slate-400 font-sans">Rolled Back{!nativeMode ? ' (sim)' : ''}</div>
              <div className="font-semibold text-sm text-slate-900 dark:text-slate-100 mt-0.5">
                {telemetry.tunerRolledBack ?? 0}
              </div>
            </div>
          </div>
        </div>

        {/* Active Block Device Queue & iosched Tunables + Bandit Candidates */}
        <div className="grid grid-cols-1 lg:grid-cols-2 gap-4">
          <div className="rounded-2xl bg-white/70 dark:bg-slate-900/40 p-3.5 border border-slate-900/[0.05] dark:border-white/[0.05] space-y-2.5">
            <div className="flex items-center justify-between text-xs">
              <span className="font-medium text-slate-700 dark:text-slate-200 flex items-center gap-1.5">
                <HardDrive className="w-3.5 h-3.5 text-[#0b57d0] dark:text-[#a8c7fa]" />
                Block Queue Surface{!nativeMode ? ' (sim)' : ''} ({primaryQueue ? `/sys/block/${primaryQueue.device}/queue` : 'Awaiting block queue probe'})
              </span>
              <span className="font-mono text-[11px] text-slate-500 dark:text-slate-400">
                Baseline: {primaryQueue?.baselineScheduler ?? 'N/A'} → Active:{' '}
                <strong className="text-[#0b57d0] dark:text-[#a8c7fa]">
                  {primaryQueue?.activeScheduler ?? 'N/A'}
                </strong>
              </span>
            </div>

            <div className="grid grid-cols-3 sm:grid-cols-6 gap-2 pt-1 font-mono tabular-nums text-xs">
              <div>
                <div className="text-[10px] text-slate-400 font-sans">read_expire</div>
                <div className="font-semibold text-slate-800 dark:text-slate-200">
                  {primaryQueue && primaryQueue.readExpireMs >= 0 ? `${primaryQueue.readExpireMs}ms` : 'N/A'}
                </div>
              </div>
              <div>
                <div className="text-[10px] text-slate-400 font-sans">writes_starved</div>
                <div className="font-semibold text-slate-800 dark:text-slate-200">
                  {primaryQueue && primaryQueue.writesStarved >= 0 ? primaryQueue.writesStarved : 'N/A'}
                </div>
              </div>
              <div>
                <div className="text-[10px] text-slate-400 font-sans">fifo_batch</div>
                <div className="font-semibold text-slate-800 dark:text-slate-200">
                  {primaryQueue && primaryQueue.fifoBatch >= 0 ? primaryQueue.fifoBatch : 'N/A'}
                </div>
              </div>
              <div>
                <div className="text-[10px] text-slate-400 font-sans">read_ahead</div>
                <div className="font-semibold text-slate-800 dark:text-slate-200">
                  {primaryQueue && primaryQueue.readAheadKb >= 0 ? `${primaryQueue.readAheadKb}KB` : 'N/A'}
                </div>
              </div>
              <div>
                <div className="text-[10px] text-slate-400 font-sans">nr_requests</div>
                <div className="font-semibold text-slate-800 dark:text-slate-200">
                  {primaryQueue && primaryQueue.nrRequests >= 0 ? primaryQueue.nrRequests : 'N/A'}
                </div>
              </div>
              <div>
                <div className="text-[10px] text-slate-400 font-sans">rq_affinity</div>
                <div className="font-semibold text-slate-800 dark:text-slate-200">
                  {primaryQueue && primaryQueue.rqAffinity >= 0 ? primaryQueue.rqAffinity : 'N/A'}
                </div>
              </div>
            </div>
          </div>

          <div className="rounded-2xl bg-white/70 dark:bg-slate-900/40 p-3.5 border border-slate-900/[0.05] dark:border-white/[0.05] space-y-3 pixel-ranking-card">
            <div className="flex flex-wrap items-center justify-between gap-2 text-xs">
              <span className="font-semibold text-slate-800 dark:text-slate-100">Global Adaptive Candidate Ranking</span>
              <span className="font-mono text-[11px] text-slate-500 dark:text-slate-400">Active: {telemetry.tunerCandidateId ?? 'none'}</span>
            </div>
            <p className="text-[10px] leading-relaxed text-slate-500 dark:text-slate-400">
              One pooled UCB population across all candidates. Domain is metadata only; workload relevance, empirical evidence, eligibility, and safety determine the decision.
            </p>
            <div className="max-h-[360px] space-y-1.5 overflow-y-auto pr-1">
              {rankedEvaluations.length > 0 ? rankedEvaluations.map((candidate, index) => {
                const arm = candidates.find((item) => item.id === candidate.id);
                const isActive = candidate.selected || candidate.id === telemetry.tunerCandidateId;
                return (
                  <div
                    key={candidate.id}
                    className={'rounded-xl border px-3 py-2.5 ' +
                      (isActive
                        ? 'border-[#0b57d0]/30 bg-[#d3e3fd]/55 dark:border-[#a8c7fa]/30 dark:bg-[#0842a0]/20'
                        : 'border-slate-900/[0.05] bg-white/55 dark:border-white/[0.06] dark:bg-white/[0.025]')}
                  >
                    <div className="flex items-start justify-between gap-3">
                      <div className="min-w-0">
                        <div className="flex flex-wrap items-center gap-1.5 text-[10px] font-mono">
                          <span className="text-slate-400">#{index + 1}</span>
                          <span className="text-slate-500 dark:text-slate-400">{candidate.domain}</span>
                          {isActive && <span className="rounded-full bg-[#0b57d0] px-1.5 py-0.5 text-white dark:bg-[#a8c7fa] dark:text-[#062e6f]">ACTIVE</span>}
                          {!candidate.eligible && <span className="text-amber-700 dark:text-amber-300">INELIGIBLE</span>}
                        </div>
                        <div className="mt-1 break-all text-[11px] font-semibold text-slate-800 dark:text-slate-100">{candidate.id}</div>
                      </div>
                      <div className="shrink-0 text-right font-mono tabular-nums">
                        <div className="text-sm font-semibold text-slate-900 dark:text-slate-100">{candidate.utilityScore.toFixed(1)}</div>
                        <div className="text-[9px] text-slate-500 dark:text-slate-400">global utility</div>
                      </div>
                    </div>
                    <div className="mt-2 flex flex-wrap gap-x-3 gap-y-1 font-mono text-[10px] text-slate-500 dark:text-slate-400">
                      <span>UCB {candidate.empiricalScore.toFixed(3)}</span>
                      <span>confidence {Math.round(candidate.confidence * 100)}%</span>
                      {arm && <span>evidence {arm.rewardSamples ?? 0}</span>}
                      {arm && <span>retained {arm.successes}</span>}
                      {arm && <span>rollback {arm.rollbacks}</span>}
                      {arm && <span>attempts {arm.trials}</span>}
                    </div>
                    {!candidate.eligible && candidate.rejectionReason && (
                      <div className="mt-1 break-words text-[10px] text-amber-700 dark:text-amber-300">{candidate.rejectionReason}</div>
                    )}
                  </div>
                );
              }) : rankedArms.map((arm, index) => (
                <div key={arm.id} className="flex items-center justify-between gap-3 rounded-xl border border-slate-900/[0.05] px-3 py-2 dark:border-white/[0.06]">
                  <div className="min-w-0">
                    <div className="text-[10px] font-mono text-slate-500 dark:text-slate-400">#{index + 1} · {arm.domain}</div>
                    <div className="truncate text-[11px] font-semibold text-slate-800 dark:text-slate-100">{arm.id}</div>
                    <div className="text-[10px] text-slate-500 dark:text-slate-400">evidence {arm.rewardSamples ?? 0} · retained {arm.successes} · rollback {arm.rollbacks}</div>
                  </div>
                  <div className="shrink-0 text-right font-mono">
                    <div className="text-sm font-semibold text-slate-900 dark:text-slate-100">{arm.ucbScore.toFixed(3)}</div>
                    <div className="text-[9px] text-slate-500 dark:text-slate-400">UCB1</div>
                  </div>
                </div>
              ))}
            </div>
          </div>
        </div>
      </div>

      {/* Hybrid ONNX / Online RLS-MLP Neural Horizon Predictor Strip */}
      <div className="relative z-10 mt-4 m3-tonal-block p-4 sm:p-5 flex flex-col sm:flex-row sm:items-center justify-between gap-4">
        <div className="space-y-1 min-w-0">
          <div className="flex flex-wrap items-center gap-2 text-xs">
            <span className="font-semibold text-slate-900 dark:text-slate-100">
              Neural Horizon Predictor (ONNX / Online RLS-MLP){!nativeMode ? ' (sim)' : ''}
            </span>
            <span className="px-2 py-0.5 rounded-full text-[11px] font-mono bg-[#d3e3fd] text-[#041e49] dark:bg-[#0842a0] dark:text-[#d3e3fd]">
              {telemetry.aiBackend ?? 'embedded-rls-mlp'}
            </span>
          </div>
          <p className="text-xs text-slate-500 dark:text-slate-400 leading-relaxed">
            Forecasts 10-second thermal trajectory via Online Recursive Least Squares + 6×8×2 Micro-MLP with NLMS output-head adaptation (or dynamic <code className="font-mono text-[11px]">libonnxruntime.so</code> C-API when <code className="font-mono text-[11px]">equilibrium.onnx</code> is present), pre-pacing capacity strictly within Safety Admission invariants.
          </p>
        </div>

        <div className="grid grid-cols-4 gap-3 sm:gap-5 shrink-0 font-mono tabular-nums text-xs border-t sm:border-t-0 pt-3 sm:pt-0 border-slate-900/[0.06] dark:border-white/[0.06]">
          <div>
            <div className="text-[11px] text-slate-400 font-sans">10s Forecast{!nativeMode ? ' (sim)' : ''}</div>
            <div className="font-semibold text-sm text-slate-900 dark:text-slate-100 mt-0.5">
              {(telemetry.aiPredictedPressure10s ?? 45.0).toFixed(1)}
            </div>
          </div>
          <div>
            <div className="text-[11px] text-slate-400 font-sans">Velocity{!nativeMode ? ' (sim)' : ''}</div>
            <div className="font-semibold text-sm text-slate-900 dark:text-slate-100 mt-0.5">
              {(telemetry.aiPredictedVelocity ?? 0) >= 0
                ? `+${(telemetry.aiPredictedVelocity ?? 0).toFixed(2)}/s`
                : `${(telemetry.aiPredictedVelocity ?? 0).toFixed(2)}/s`}
            </div>
          </div>
          <div>
            <div className="text-[11px] text-slate-400 font-sans">Scale Hint{!nativeMode ? ' (sim)' : ''}</div>
            <div className="font-semibold text-sm text-slate-900 dark:text-slate-100 mt-0.5">
              {Math.round((telemetry.aiRecommendedScale ?? 1.0) * 100)}%
            </div>
          </div>
          <div>
            <div className="text-[11px] text-slate-400 font-sans">Confidence{!nativeMode ? ' (sim)' : ''}</div>
            <div className="font-semibold text-sm text-[#0b57d0] dark:text-[#a8c7fa] mt-0.5">
              {Math.round((telemetry.aiConfidence ?? 0.85) * 100)}%
            </div>
          </div>
        </div>
      </div>


    </div>
  );
};
