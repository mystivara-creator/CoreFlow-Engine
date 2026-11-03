import React from 'react';
import {
  Activity,
  Flame,
  Layers,
  Sparkles,
  Zap,
  Gauge,
  Smartphone,
} from 'lucide-react';
import { AutonomousTelemetry, getModeMeta } from '../engine/autonomousEngine';

interface AutonomousStateHeroProps {
  telemetry: AutonomousTelemetry;
  nativeMode?: boolean;
  thermalTripCelsius: number;
  onSimulateTouchBoost: () => void;
  onSimulateAppLaunch: () => void;
}

export const AutonomousStateHero: React.FC<AutonomousStateHeroProps> = ({
  telemetry,
  nativeMode = false,
  thermalTripCelsius,
  onSimulateTouchBoost,
  onSimulateAppLaunch,
}) => {
  const meta = getModeMeta(telemetry.mode);
  const uclampPct = Math.min(100, Math.round((telemetry.uclampMin / 1024) * 100));

  // Determine thermal safety label & color based on dynamic trip ceiling
  const temp = telemetry.socTempCelsius;
  const isOverheating = temp >= thermalTripCelsius;
  const isWarm = temp >= thermalTripCelsius - 4.0;
  const tempColor = isOverheating
    ? 'text-rose-500'
    : isWarm
    ? 'text-amber-500'
    : 'text-teal-400 dark:text-teal-300';

  return (
    <div className="relative overflow-hidden rounded-3xl bg-slate-100/80 dark:bg-[#111827]/90 border border-slate-200/90 dark:border-slate-800 p-5 sm:p-7 shadow-lg shadow-slate-900/5 transition-all duration-300">
      {/* Background subtle glow matching current autonomous state */}
      <div
        className="pointer-events-none absolute -top-24 -right-24 w-80 h-80 rounded-full blur-3xl opacity-20 transition-all duration-700"
        style={{ backgroundColor: meta.glowColor }}
      />

      {/* Top Banner: Current Autonomous Mode */}
      <div className="flex flex-col sm:flex-row sm:items-center justify-between gap-4 mb-6 relative z-10">
        <div>
          <div className="flex items-center gap-2 mb-1.5">
            <span className="flex items-center gap-1.5 text-xs font-semibold uppercase tracking-wider text-slate-500 dark:text-slate-400">
              <Sparkles className="w-3.5 h-3.5 text-sky-500" />
              Autonomous Engine Adaptation
            </span>
            <span
              className={`inline-flex items-center px-2.5 py-0.5 rounded-full text-xs font-semibold border ${meta.badgeColor}`}
            >
              {nativeMode ? (telemetry.engineActive ? 'Native daemon active' : 'Native daemon stopped') : (telemetry.engineActive ? meta.label : 'Paused')}
            </span>
          </div>
          <h2 className="text-xl sm:text-2xl font-bold tracking-tight text-slate-900 dark:text-white">
            {nativeMode ? telemetry.modeDescription : (telemetry.engineActive ? meta.description : 'Running on OEM default parameters')}
          </h2>
        </div>

        {/* Foreground Active App detection chip */}
        <div className="flex items-center gap-2 px-3.5 py-2 rounded-2xl bg-white/90 dark:bg-slate-900/90 border border-slate-200 dark:border-slate-800 shadow-sm">
          <Smartphone className="w-4 h-4 text-sky-500 shrink-0" />
          <div className="text-left">
            <div className="text-[11px] text-slate-500 dark:text-slate-400 font-medium leading-none mb-0.5">
              {nativeMode ? 'Foreground detection' : `Top-App (PID ${telemetry.topAppPid})`}
            </div>
            <div className="text-xs font-semibold text-slate-800 dark:text-slate-200 font-mono truncate max-w-[170px]">
              {telemetry.topAppLabel}
            </div>
          </div>
        </div>
      </div>

      {/* 4 Essential Metric Cards (Material Squircles) */}
      <div className="grid grid-cols-2 lg:grid-cols-4 gap-3 sm:gap-4 relative z-10 mb-5">
        {/* Metric 1: EAS Uclamp Dynamic Floor */}
        <div className="rounded-2xl bg-white/70 dark:bg-slate-900/60 border border-slate-200/70 dark:border-slate-800/80 p-4 transition-all hover:border-sky-500/30">
          <div className="flex items-center justify-between text-slate-500 dark:text-slate-400 text-xs font-medium mb-1">
            <span>EAS Uclamp Min</span>
            <Zap className="w-3.5 h-3.5 text-sky-500" />
          </div>
          <div className="flex items-baseline gap-1.5 mb-2">
            <span className="text-2xl sm:text-3xl font-bold font-mono tracking-tight text-sky-500 tabular-nums">
              {telemetry.uclampMin}
            </span>
            <span className="text-xs text-slate-400 dark:text-slate-500 font-mono">
              / {telemetry.uclampMax}
            </span>
          </div>
          {/* Capacity fill bar */}
          <div className="w-full bg-slate-200 dark:bg-slate-800 h-1.5 rounded-full overflow-hidden">
            <div
              className="bg-gradient-to-r from-sky-500 to-teal-400 h-full rounded-full transition-all duration-300"
              style={{ width: `${uclampPct}%` }}
            />
          </div>
          <div className="text-[11px] text-slate-500 dark:text-slate-400 mt-1.5">
            Capacity floor: {uclampPct}%
          </div>
        </div>

        {/* Metric 2: Frame Pacing & FPS */}
        <div className="rounded-2xl bg-white/70 dark:bg-slate-900/60 border border-slate-200/70 dark:border-slate-800/80 p-4 transition-all hover:border-emerald-500/30">
          <div className="flex items-center justify-between text-slate-500 dark:text-slate-400 text-xs font-medium mb-1">
            <span>Frame Pacing</span>
            <Gauge className="w-3.5 h-3.5 text-emerald-500" />
          </div>
          <div className="flex items-baseline gap-1.5 mb-2">
            <span className="text-2xl sm:text-3xl font-bold font-mono tracking-tight text-emerald-500 tabular-nums">
              {nativeMode ? 'N/A' : telemetry.currentFps}
            </span>
            <span className="text-xs text-slate-400 dark:text-slate-500 font-mono">
              FPS
            </span>
          </div>
          <div className="flex items-center justify-between text-[11px] text-slate-500 dark:text-slate-400">
            <span>Jank miss rate:</span>
            <span className="font-mono text-emerald-500 font-semibold tabular-nums">
              {nativeMode ? 'N/A' : `${telemetry.jankRatePct}%`}
            </span>
          </div>
          <div className="text-[11px] text-slate-500 dark:text-slate-400 mt-0.5">
            {nativeMode ? 'Frame metrics unavailable' : `SurfaceFlinger ${telemetry.targetFps}Hz preview`}
          </div>
        </div>

        {/* Metric 3: SoC Temperature */}
        <div className="rounded-2xl bg-white/70 dark:bg-slate-900/60 border border-slate-200/70 dark:border-slate-800/80 p-4 transition-all hover:border-amber-500/30">
          <div className="flex items-center justify-between text-slate-500 dark:text-slate-400 text-xs font-medium mb-1">
            <span>SoC Temperature</span>
            <Flame className="w-3.5 h-3.5 text-amber-500" />
          </div>
          <div className="flex items-baseline gap-1.5 mb-2">
            <span className={`text-2xl sm:text-3xl font-bold font-mono tracking-tight tabular-nums ${tempColor}`}>
              {nativeMode && temp < 0 ? 'N/A' : telemetry.socTempCelsius.toFixed(1)}
            </span>
            <span className="text-xs text-slate-400 dark:text-slate-500 font-mono">
              °C
            </span>
          </div>
          <div className="flex items-center justify-between text-[11px] text-slate-500 dark:text-slate-400">
            <span>Thermal state:</span>
            <span className="font-semibold text-slate-700 dark:text-slate-300">
              {temp < 0 ? 'Unknown' : isOverheating ? 'Guarded' : isWarm ? 'Elevated' : 'Optimal'}
            </span>
          </div>
          <div className="text-[11px] text-slate-500 dark:text-slate-400 mt-0.5">
            Trip ceiling: {thermalTripCelsius.toFixed(1)}°C
          </div>
        </div>

        {/* Metric 4: discovered scheduler capability */}
        <div className="rounded-2xl bg-white/70 dark:bg-slate-900/60 border border-slate-200/70 dark:border-slate-800/80 p-4 transition-all hover:border-indigo-500/30">
          <div className="flex items-center justify-between text-slate-500 dark:text-slate-400 text-xs font-medium mb-1">
            <span>UClamp Capability</span>
            <Layers className="w-3.5 h-3.5 text-indigo-400" />
          </div>
          <div className="flex items-baseline gap-1 mb-2">
            <span className="text-lg sm:text-xl font-bold tracking-tight text-slate-800 dark:text-slate-100">
              {telemetry.uclampSupported ? 'Discovered' : 'Unavailable'}
            </span>
          </div>
          <div className="flex items-center justify-between text-[11px] text-slate-500 dark:text-slate-400">
            <span>Controllers:</span>
            <span className="font-mono text-indigo-400 font-semibold">
              {nativeMode ? 'Semantic target' : 'Preview capability'}
            </span>
          </div>
          <div className="text-[11px] text-slate-500 dark:text-slate-400 mt-0.5">
            {nativeMode ? 'top-app min/max pair' : 'Simulation only'}
          </div>
        </div>
      </div>

      {!nativeMode && <>
      {/* Autonomous Simulator Quick Test Triggers (For testing self-adaptation) */}
      <div className="flex flex-wrap items-center justify-between gap-3 pt-3 border-t border-slate-200/60 dark:border-slate-800/60 text-xs text-slate-500 dark:text-slate-400">
        <div className="flex items-center gap-1.5">
          <Activity className="w-3.5 h-3.5 text-sky-500" />
          <span>Test autonomous adaptation trigger:</span>
        </div>
        <div className="flex items-center gap-2">
          <button
            onClick={onSimulateTouchBoost}
            className="px-3 py-1.5 rounded-full bg-white dark:bg-slate-900 hover:bg-slate-200 dark:hover:bg-slate-800 border border-slate-200 dark:border-slate-800 text-slate-700 dark:text-slate-200 font-medium transition-colors active:scale-95 flex items-center gap-1.5"
          >
            <Zap className="w-3 h-3 text-emerald-500" />
            <span>Simulate Touch Scroll</span>
          </button>
          <button
            onClick={onSimulateAppLaunch}
            className="px-3 py-1.5 rounded-full bg-white dark:bg-slate-900 hover:bg-slate-200 dark:hover:bg-slate-800 border border-slate-200 dark:border-slate-800 text-slate-700 dark:text-slate-200 font-medium transition-colors active:scale-95 flex items-center gap-1.5"
          >
            <Sparkles className="w-3 h-3 text-sky-500" />
            <span>Simulate App Launch</span>
          </button>
        </div>
      </div>
      </>}
    </div>
  );
};
