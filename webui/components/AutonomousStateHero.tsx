import React from 'react';
import { Activity, Flame, Gauge, ShieldCheck, Sparkles, Zap } from 'lucide-react';
import { AutonomousTelemetry, getModeMeta } from '../engine/autonomousEngine';

interface Props {
  telemetry: AutonomousTelemetry;
  nativeMode?: boolean;
  thermalTripCelsius: number;
  onSimulateCpuLoad: () => void;
  onSimulateMemoryPressure: () => void;
}

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

  return (
    <div className="relative overflow-hidden rounded-3xl bg-slate-100/80 dark:bg-[#111827]/90 border border-slate-200/90 dark:border-slate-800 p-5 sm:p-7 shadow-lg shadow-slate-900/5 transition-all duration-300">
      <div
        className="pointer-events-none absolute -top-24 -right-24 w-80 h-80 rounded-full blur-3xl opacity-20 transition-all duration-700"
        style={{ backgroundColor: meta.glowColor }}
      />

      <div className="flex flex-col sm:flex-row sm:items-center justify-between gap-4 mb-6 relative z-10">
        <div>
          <div className="flex items-center gap-2 mb-1.5">
            <Sparkles className="w-3.5 h-3.5 text-sky-500" />
            <span className="text-xs font-semibold uppercase tracking-wider text-slate-500 dark:text-slate-400">
              Stability Engine
            </span>
            <span className={'px-2.5 py-0.5 rounded-full text-xs font-semibold border ' + meta.badgeColor}>
              {nativeMode ? (telemetry.engineActive ? 'Native Active' : 'Native Stopped') : meta.label}
            </span>
          </div>
          <h2 className="text-xl sm:text-2xl font-bold tracking-tight text-slate-900 dark:text-white">
            {nativeMode ? telemetry.modeDescription : meta.description}
          </h2>
        </div>
        <div className="px-3.5 py-2 rounded-2xl bg-white/90 dark:bg-slate-900/90 border border-slate-200 dark:border-slate-800">
          <div className="text-[11px] text-slate-500 dark:text-slate-400">Policy priority</div>
          <div className="text-xs font-semibold text-slate-800 dark:text-slate-200">
            Thermal → Pressure → Load → Balanced
          </div>
        </div>
      </div>

      <div className="grid grid-cols-2 lg:grid-cols-4 gap-3 sm:gap-4 relative z-10 mb-5">
        <div className="rounded-2xl bg-white/70 dark:bg-slate-900/60 border border-slate-200/70 dark:border-slate-800/80 p-4">
          <div className="flex items-center justify-between text-xs text-slate-500 mb-1">
            <span>Top-app UClamp</span><Zap className="w-3.5 h-3.5 text-sky-500" />
          </div>
          <div className="text-2xl sm:text-3xl font-bold font-mono text-sky-500">
            {telemetry.uclampMin}<span className="text-xs text-slate-400"> / {telemetry.uclampMax}</span>
          </div>
          <div className="text-[11px] text-slate-500 mt-2">
            {telemetry.uclampSupported ? 'Semantic target discovered' : 'Capability unavailable'}
          </div>
        </div>

        <div className="rounded-2xl bg-white/70 dark:bg-slate-900/60 border border-slate-200/70 dark:border-slate-800/80 p-4">
          <div className="flex items-center justify-between text-xs text-slate-500 mb-1">
            <span>CPU/GPU Sensors</span><Flame className="w-3.5 h-3.5 text-amber-500" />
          </div>
          <div className="text-2xl sm:text-3xl font-bold font-mono text-amber-500">
            {tempText}<span className="text-xs text-slate-400"> °C</span>
          </div>
          <div className="text-[11px] text-slate-500 mt-2">{nativeMode ? 'Control: OEM thermal policy' : `Preview trip ${thermalTripCelsius.toFixed(1)}°C`}</div>
        </div>

        <div className="rounded-2xl bg-white/70 dark:bg-slate-900/60 border border-slate-200/70 dark:border-slate-800/80 p-4">
          <div className="flex items-center justify-between text-xs text-slate-500 mb-1">
            <span>System Pressure</span><Gauge className="w-3.5 h-3.5 text-teal-500" />
          </div>
          <div className="text-sm font-mono text-slate-800 dark:text-slate-100 space-y-1 mt-2">
            <div>CPU {nativeMode ? 'log' : telemetry.cpuPressurePct.toFixed(1) + '%'}</div>
            <div>MEM {nativeMode ? 'log' : telemetry.memoryPressurePct.toFixed(1) + '%'}</div>
            <div>I/O {nativeMode ? 'log' : telemetry.ioPressurePct.toFixed(1) + '%'}</div>
          </div>
        </div>

        <div className="rounded-2xl bg-white/70 dark:bg-slate-900/60 border border-slate-200/70 dark:border-slate-800/80 p-4">
          <div className="flex items-center justify-between text-xs text-slate-500 mb-1">
            <span>Recovery</span><ShieldCheck className="w-3.5 h-3.5 text-emerald-500" />
          </div>
          <div className="text-lg font-bold text-slate-800 dark:text-slate-100 mt-2">
            {telemetry.baselineStatus === 'protected' ? 'Journal Active' : 'Restored'}
          </div>
          <div className="text-[11px] text-slate-500 mt-2">Read-back verified writes</div>
        </div>
      </div>

      {!nativeMode && (
        <div className="flex flex-wrap items-center justify-between gap-3 pt-3 border-t border-slate-200/60 dark:border-slate-800/60 text-xs text-slate-500">
          <div className="flex items-center gap-1.5"><Activity className="w-3.5 h-3.5 text-sky-500" />Preview policy inputs:</div>
          <div className="flex gap-2">
            <button onClick={onSimulateCpuLoad} className="px-3 py-1.5 rounded-full bg-white dark:bg-slate-900 border border-slate-200 dark:border-slate-800">CPU Load</button>
            <button onClick={onSimulateMemoryPressure} className="px-3 py-1.5 rounded-full bg-white dark:bg-slate-900 border border-slate-200 dark:border-slate-800">Memory/I/O Pressure</button>
          </div>
        </div>
      )}
    </div>
  );
};
