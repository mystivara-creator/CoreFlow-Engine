import React from 'react';
import { Activity, Flame, Gauge, ShieldCheck, Zap, GitCommit } from 'lucide-react';
import { AutonomousTelemetry, getModeMeta, TransactionStage } from '../engine/autonomousEngine';

interface Props {
  telemetry: AutonomousTelemetry;
  nativeMode?: boolean;
  thermalTripCelsius: number;
  onSimulateCpuLoad: () => void;
  onSimulateMemoryPressure: () => void;
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
}) => {
  const meta = getModeMeta(telemetry.mode);
  const temp = telemetry.socTempCelsius;
  const tempText = nativeMode && temp < 0 ? 'N/A' : temp.toFixed(1);
  const currentStage: TransactionStage = telemetry.engineActive
    ? telemetry.transactionStage ?? meta.stage
    : 'PLANNED';

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
            Mutations are coordinated by optimization intent, dependency provenance, and risk budget. Every kernel write requires a valid baseline, immediate readback verification, and OEM ownership guard.
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
            Outcome: {telemetry.lastOutcome ?? meta.outcome} · Ownership: {telemetry.ownershipIntact !== false ? 'Valid' : 'Released'}
          </div>
        </div>
      </div>

      {/* Transaction State Machine Pipeline Bar (Section 15 of Architecture Draft) */}
      <div className="relative z-10 py-5 border-b border-slate-900/[0.06] dark:border-white/[0.06]">
        <div className="flex items-center justify-between gap-2 mb-3">
          <span className="text-xs font-medium text-slate-600 dark:text-slate-300 flex items-center gap-1.5">
            <GitCommit className="w-3.5 h-3.5 text-[#0b57d0] dark:text-[#a8c7fa]" />
            Transaction Lifecycle State
          </span>
          <span className="text-xs font-mono text-slate-500 dark:text-slate-400">
            Stage: {currentStage}
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

      {/* Preview Simulation Controls */}
      {!nativeMode && (
        <div className="relative z-10 flex flex-wrap items-center justify-between gap-3 pt-5 mt-5 border-t border-slate-900/[0.06] dark:border-white/[0.06] text-xs text-slate-600 dark:text-slate-400">
          <div className="flex items-center gap-2">
            <Activity className="w-4 h-4 text-[#0b57d0] dark:text-[#a8c7fa]" />
            <span>Preview workload stimulus (tests Reasoning Engine & Admission Gate):</span>
          </div>
          <div className="flex items-center gap-2">
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
              Simulate Memory/I/O Contention
            </button>
          </div>
        </div>
      )}
    </div>
  );
};
