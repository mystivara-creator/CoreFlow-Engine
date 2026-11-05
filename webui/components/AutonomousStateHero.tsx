import React from 'react';
import { Activity, Flame, Gauge, ShieldCheck, Zap, GitCommit, HardDrive, Sliders } from 'lucide-react';
import { AutonomousTelemetry, getModeMeta, TransactionStage } from '../engine/autonomousEngine';

interface Props {
  telemetry: AutonomousTelemetry;
  nativeMode?: boolean;
  thermalTripCelsius: number;
  onSimulateCpuLoad: () => void;
  onSimulateMemoryPressure: () => void;
  onSimulateIoTrial?: () => void;
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
  onSimulateCpuLoad,
  onSimulateMemoryPressure,
  onSimulateIoTrial,
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
  const utilityDelta = telemetry.tunerUtilityDelta ?? 0;

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
              Coordinated Mutation Transaction
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
            Transaction Lifecycle State
          </span>
          <span className="text-xs font-mono text-slate-500 dark:text-slate-400">
            Stage: {telemetry.tunerStage ?? currentStage}
          </span>
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
        <div className="m3-tonal-block p-4 flex flex-col justify-between">
          <div className="flex items-center justify-between text-xs text-slate-500 dark:text-slate-400 mb-2">
            <span className="font-medium">Top-App UClamp</span>
            <Zap className="w-4 h-4 text-[#0b57d0] dark:text-[#a8c7fa]" />
          </div>
          <div className="text-2xl font-semibold font-mono tabular-nums text-slate-900 dark:text-slate-100">
            {telemetry.uclampMin}
            <span className="text-xs font-normal text-slate-500 dark:text-slate-400"> / {telemetry.uclampMax}</span>
          </div>
          <div className="text-xs text-slate-500 dark:text-slate-400 mt-2 leading-normal">
            {telemetry.uclampSupported ? 'Semantic cgroup v2 verified' : 'Capability unavailable'}
          </div>
        </div>

        <div className="m3-tonal-block p-4 flex flex-col justify-between">
          <div className="flex items-center justify-between text-xs text-slate-500 dark:text-slate-400 mb-2">
            <span className="font-medium">SoC Thermal Composite</span>
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
            <span className="font-medium">PSI Resource Pressure</span>
            <Gauge className="w-4 h-4 text-[#0b57d0] dark:text-[#a8c7fa]" />
          </div>
          <div className="grid grid-cols-3 gap-2 font-mono tabular-nums text-xs text-slate-800 dark:text-slate-200 my-1">
            <div>
              <div className="text-[11px] text-slate-400">CPU</div>
              <div className="font-semibold">{nativeMode ? 'Live' : `${telemetry.cpuPressurePct.toFixed(1)}%`}</div>
            </div>
            <div>
              <div className="text-[11px] text-slate-400">MEM</div>
              <div className="font-semibold">{nativeMode ? 'Live' : `${telemetry.memoryPressurePct.toFixed(1)}%`}</div>
            </div>
            <div>
              <div className="text-[11px] text-slate-400">I/O</div>
              <div className="font-semibold">{nativeMode ? 'Live' : `${telemetry.ioPressurePct.toFixed(1)}%`}</div>
            </div>
          </div>
          <div className="text-xs text-slate-500 dark:text-slate-400 mt-1 leading-normal">
            Linux `/proc/pressure` avg10
          </div>
        </div>

        <div className="m3-tonal-block p-4 flex flex-col justify-between">
          <div className="flex items-center justify-between text-xs text-slate-500 dark:text-slate-400 mb-2">
            <span className="font-medium">Durable Journal</span>
            <ShieldCheck className="w-4 h-4 text-[#0b57d0] dark:text-[#a8c7fa]" />
          </div>
          <div className="text-lg font-semibold text-slate-900 dark:text-slate-100">
            {telemetry.baselineStatus === 'protected' ? 'Journal Armed' : 'Baseline Restored'}
          </div>
          <div className="text-xs text-slate-500 dark:text-slate-400 mt-2 leading-normal">
            Write-ahead & readback verified
          </div>
        </div>
      </div>

      {/* Multi-Domain Autonomous Scheduler & Outcome Evaluator (mq-deadline, kyber, bfq, iosched/*, vm/*) */}
      <div className="relative z-10 mt-4 m3-tonal-block p-4 sm:p-5 space-y-4">
        <div className="flex flex-col sm:flex-row sm:items-start justify-between gap-3 border-b border-slate-900/[0.06] dark:border-white/[0.06] pb-3.5">
          <div className="space-y-1 min-w-0">
            <div className="flex flex-wrap items-center gap-2 text-xs">
              <Sliders className="w-3.5 h-3.5 text-[#0b57d0] dark:text-[#a8c7fa]" />
              <span className="font-semibold text-slate-900 dark:text-slate-100">
                Autonomous Multi-Domain Scheduler & Tunable Evaluator
              </span>
              <span className="px-2 py-0.5 rounded-full text-[11px] font-mono bg-[#d3e3fd] text-[#041e49] dark:bg-[#0842a0] dark:text-[#d3e3fd]">
                Intent: {telemetry.tunerIntent ?? 'HoldEquilibrium'}
              </span>
            </div>
            <p className="text-xs text-slate-600 dark:text-slate-400 leading-relaxed">
              {telemetry.tunerRationale ??
                'Dynamically selects block I/O schedulers (mq-deadline, kyber, bfq) & iosched/VM tunables when contention arises, then evaluates empirical Before/After PSI delta (ΔU) to retain or roll back.'}
            </p>
          </div>

          <div className="grid grid-cols-3 gap-3 sm:gap-4 shrink-0 font-mono tabular-nums text-xs">
            <div>
              <div className="text-[11px] text-slate-400 font-sans">Empirical ΔU</div>
              <div className="font-semibold text-sm text-slate-900 dark:text-slate-100 mt-0.5">
                {utilityDelta >= 0 ? `+${utilityDelta.toFixed(2)}` : utilityDelta.toFixed(2)}
              </div>
            </div>
            <div>
              <div className="text-[11px] text-slate-400 font-sans">Retained</div>
              <div className="font-semibold text-sm text-slate-900 dark:text-slate-100 mt-0.5">
                {telemetry.tunerImproved ?? 0} / {telemetry.tunerTrials ?? 0}
              </div>
            </div>
            <div>
              <div className="text-[11px] text-slate-400 font-sans">Rolled Back</div>
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
                Block Queue Surface ({primaryQueue ? `/sys/block/${primaryQueue.device}/queue` : 'Discovered at runtime'})
              </span>
              <span className="font-mono text-[11px] text-slate-500 dark:text-slate-400">
                Baseline: {primaryQueue?.baselineScheduler ?? 'none'} → Active:{' '}
                <strong className="text-[#0b57d0] dark:text-[#a8c7fa]">
                  {primaryQueue?.activeScheduler ?? 'mq-deadline'}
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
                  {primaryQueue && primaryQueue.readAheadKb >= 0 ? `${primaryQueue.readAheadKb}KB` : '128KB'}
                </div>
              </div>
              <div>
                <div className="text-[10px] text-slate-400 font-sans">nr_requests</div>
                <div className="font-semibold text-slate-800 dark:text-slate-200">
                  {primaryQueue && primaryQueue.nrRequests >= 0 ? primaryQueue.nrRequests : '128'}
                </div>
              </div>
              <div>
                <div className="text-[10px] text-slate-400 font-sans">rq_affinity</div>
                <div className="font-semibold text-slate-800 dark:text-slate-200">
                  {primaryQueue && primaryQueue.rqAffinity >= 0 ? primaryQueue.rqAffinity : '2'}
                </div>
              </div>
            </div>
          </div>

          <div className="rounded-2xl bg-white/70 dark:bg-slate-900/40 p-3.5 border border-slate-900/[0.05] dark:border-white/[0.05] space-y-2">
            <div className="flex items-center justify-between text-xs">
              <span className="font-medium text-slate-700 dark:text-slate-200">
                UCB1 Empirical Bandit Candidate Memory
              </span>
              <span className="font-mono text-[11px] text-slate-500 dark:text-slate-400">
                Active: {telemetry.tunerCandidateId ?? 'oem_baseline'}
              </span>
            </div>
            <div className="space-y-1.5">
              {candidates.slice(0, 3).map((c) => (
                <div
                  key={c.id}
                  className="flex items-center justify-between gap-2 text-xs py-1 border-b last:border-b-0 border-slate-900/[0.04] dark:border-white/[0.04]"
                >
                  <span className="truncate text-slate-700 dark:text-slate-300 font-medium">
                    {c.label}
                  </span>
                  <span className="shrink-0 font-mono tabular-nums text-[11px] text-slate-500 dark:text-slate-400">
                    wins {c.successes}/{c.trials} · ΔU {c.meanDelta >= 0 ? `+${c.meanDelta.toFixed(1)}` : c.meanDelta.toFixed(1)}
                  </span>
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
              Neural Horizon Predictor (ONNX / Online RLS-MLP)
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
            <div className="text-[11px] text-slate-400 font-sans">10s Forecast</div>
            <div className="font-semibold text-sm text-slate-900 dark:text-slate-100 mt-0.5">
              {(telemetry.aiPredictedPressure10s ?? 45.0).toFixed(1)}
            </div>
          </div>
          <div>
            <div className="text-[11px] text-slate-400 font-sans">Velocity</div>
            <div className="font-semibold text-sm text-slate-900 dark:text-slate-100 mt-0.5">
              {(telemetry.aiPredictedVelocity ?? 0) >= 0
                ? `+${(telemetry.aiPredictedVelocity ?? 0).toFixed(2)}/s`
                : `${(telemetry.aiPredictedVelocity ?? 0).toFixed(2)}/s`}
            </div>
          </div>
          <div>
            <div className="text-[11px] text-slate-400 font-sans">Scale Hint</div>
            <div className="font-semibold text-sm text-slate-900 dark:text-slate-100 mt-0.5">
              {Math.round((telemetry.aiRecommendedScale ?? 1.0) * 100)}%
            </div>
          </div>
          <div>
            <div className="text-[11px] text-slate-400 font-sans">Confidence</div>
            <div className="font-semibold text-sm text-[#0b57d0] dark:text-[#a8c7fa] mt-0.5">
              {Math.round((telemetry.aiConfidence ?? 0.85) * 100)}%
            </div>
          </div>
        </div>
      </div>

      {/* Preview Simulation Controls */}
      {!nativeMode && (
        <div className="relative z-10 flex flex-wrap items-center justify-between gap-3 pt-5 mt-5 border-t border-slate-900/[0.06] dark:border-white/[0.06] text-xs text-slate-600 dark:text-slate-400">
          <div className="flex items-center gap-2">
            <Activity className="w-4 h-4 text-[#0b57d0] dark:text-[#a8c7fa]" />
            <span>Preview workload stimulus (tests Reasoning Engine & Empirical Evaluator):</span>
          </div>
          <div className="flex flex-wrap items-center gap-2">
            <button
              onClick={onSimulateCpuLoad}
              className="px-4 py-1.5 rounded-full bg-slate-900/[0.05] hover:bg-slate-900/[0.09] dark:bg-white/[0.06] dark:hover:bg-white/[0.11] text-slate-800 dark:text-slate-200 font-medium"
            >
              Simulate Interactive CPU Load
            </button>
            <button
              onClick={onSimulateMemoryPressure}
              className="px-4 py-1.5 rounded-full bg-slate-900/[0.05] hover:bg-slate-900/[0.09] dark:bg-white/[0.06] dark:hover:bg-white/[0.11] text-slate-800 dark:text-slate-200 font-medium"
            >
              Simulate Memory Reclaim
            </button>
            {onSimulateIoTrial && (
              <button
                onClick={onSimulateIoTrial}
                className="px-4 py-1.5 rounded-full bg-[#d3e3fd] hover:bg-[#c2d7fc] dark:bg-[#0842a0] dark:hover:bg-[#0b57d0] text-[#041e49] dark:text-[#d3e3fd] font-medium"
              >
                Simulate I/O Scheduler Trial (mq-deadline)
              </button>
            )}
          </div>
        </div>
      )}
    </div>
  );
};
