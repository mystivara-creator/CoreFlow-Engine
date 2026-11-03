import React from 'react';
import { Sliders, CheckCircle2, ShieldAlert, Cpu, Battery, Gauge, Zap } from 'lucide-react';
import { AutonomousConfig } from '../engine/autonomousEngine';

interface AutonomousTuningCardProps {
  config: AutonomousConfig;
  onChangeConfig: (newConfig: AutonomousConfig) => void;
  onSaveConfig: () => void;
  isSaving: boolean;
  saveMessage: string | null;
}

export const AutonomousTuningCard: React.FC<AutonomousTuningCardProps> = ({
  config,
  onChangeConfig,
  onSaveConfig,
  isSaving,
  saveMessage,
}) => {
  const update = <K extends keyof AutonomousConfig>(key: K, value: AutonomousConfig[K]) => {
    onChangeConfig({ ...config, [key]: value });
  };

  return (
    <div className="rounded-3xl bg-slate-100/80 dark:bg-[#111827]/90 border border-slate-200/90 dark:border-slate-800 p-5 sm:p-7 shadow-lg shadow-slate-900/5 transition-all">
      <div className="flex flex-col sm:flex-row sm:items-center justify-between gap-3 mb-6 pb-4 border-b border-slate-200/60 dark:border-slate-800/60">
        <div>
          <div className="flex items-center gap-2 mb-1">
            <Sliders className="w-4 h-4 text-sky-500" />
            <h3 className="font-bold text-lg text-slate-900 dark:text-white">
              Autonomous Adaptation Dials
            </h3>
          </div>
          <p className="text-xs text-slate-500 dark:text-slate-400">
            Engine self-adapts dynamically; adjust sensitivity bias and hardware boundaries.
          </p>
        </div>

        {/* Save button with status feedback */}
        <div className="flex items-center gap-2 self-start sm:self-auto">
          {saveMessage && (
            <span className="text-xs text-emerald-500 font-medium animate-fade-in">
              {saveMessage}
            </span>
          )}
          <button
            onClick={onSaveConfig}
            disabled={isSaving}
            className="px-4 py-2 rounded-full bg-sky-500 hover:bg-sky-400 text-slate-950 font-semibold text-xs transition-all active:scale-95 disabled:opacity-50 shadow-sm shadow-sky-500/20"
          >
            {isSaving ? 'Applying...' : 'Apply to coreflow.conf'}
          </button>
        </div>
      </div>

      <div className="grid grid-cols-1 md:grid-cols-2 gap-6">
        {/* Bias Selector (Segmented Pill) */}
        <div>
          <label className="block text-xs font-semibold uppercase tracking-wider text-slate-600 dark:text-slate-400 mb-2">
            Autonomous Adaptation Bias
          </label>
          <div className="grid grid-cols-3 gap-1.5 p-1 rounded-2xl bg-slate-200/70 dark:bg-slate-900/80 border border-slate-200 dark:border-slate-800">
            <button
              type="button"
              onClick={() => update('adaptiveBias', 'battery')}
              className={`flex flex-col items-center justify-center py-2 px-1 rounded-xl text-xs font-semibold transition-all ${
                config.adaptiveBias === 'battery'
                  ? 'bg-white dark:bg-slate-800 text-sky-600 dark:text-sky-400 shadow-sm'
                  : 'text-slate-600 dark:text-slate-400 hover:text-slate-900 dark:hover:text-white'
              }`}
            >
              <Battery className="w-4 h-4 mb-1" />
              <span>Battery</span>
            </button>
            <button
              type="button"
              onClick={() => update('adaptiveBias', 'balanced')}
              className={`flex flex-col items-center justify-center py-2 px-1 rounded-xl text-xs font-semibold transition-all ${
                config.adaptiveBias === 'balanced'
                  ? 'bg-white dark:bg-slate-800 text-teal-600 dark:text-teal-400 shadow-sm'
                  : 'text-slate-600 dark:text-slate-400 hover:text-slate-900 dark:hover:text-white'
              }`}
            >
              <Cpu className="w-4 h-4 mb-1" />
              <span>Balanced</span>
            </button>
            <button
              type="button"
              onClick={() => update('adaptiveBias', 'responsiveness')}
              className={`flex flex-col items-center justify-center py-2 px-1 rounded-xl text-xs font-semibold transition-all ${
                config.adaptiveBias === 'responsiveness'
                  ? 'bg-white dark:bg-slate-800 text-emerald-600 dark:text-emerald-400 shadow-sm'
                  : 'text-slate-600 dark:text-slate-400 hover:text-slate-900 dark:hover:text-white'
              }`}
            >
              <Zap className="w-4 h-4 mb-1" />
              <span>Performance</span>
            </button>
          </div>
          <p className="text-[11px] text-slate-500 dark:text-slate-400 mt-2">
            {config.adaptiveBias === 'battery'
              ? 'Caps uclamp ceiling and favors lower frequency clusters during daily browsing.'
              : config.adaptiveBias === 'responsiveness'
              ? 'Aggressive uclamp floor for gaming and instantaneous 120Hz frame pacing.'
              : 'Intelligent self-calibration balancing thermal efficiency and 120Hz fluidity.'}
          </p>
        </div>

        {/* Target Frame Refresh Rate */}
        <div>
          <label className="block text-xs font-semibold uppercase tracking-wider text-slate-600 dark:text-slate-400 mb-2">
            Frame Pacing Target
          </label>
          <div className="grid grid-cols-3 gap-1.5 p-1 rounded-2xl bg-slate-200/70 dark:bg-slate-900/80 border border-slate-200 dark:border-slate-800">
            {([120, 90, 60] as const).map((hz) => (
              <button
                key={hz}
                type="button"
                onClick={() => update('targetFps', hz)}
                className={`py-2 px-2 rounded-xl text-xs font-mono font-bold transition-all ${
                  config.targetFps === hz
                    ? 'bg-white dark:bg-slate-800 text-sky-600 dark:text-sky-400 shadow-sm'
                    : 'text-slate-600 dark:text-slate-400 hover:text-slate-900 dark:hover:text-white'
                }`}
              >
                {hz} Hz
              </button>
            ))}
          </div>
          <p className="text-[11px] text-slate-500 dark:text-slate-400 mt-2">
            SurfaceFlinger booster aligns render deadline with display VSYNC cadence.
          </p>
        </div>

        {/* Thermal Trip Guard Slider */}
        <div className="rounded-2xl bg-white/70 dark:bg-slate-900/60 border border-slate-200/70 dark:border-slate-800/80 p-4">
          <div className="flex items-center justify-between text-xs mb-2">
            <span className="font-semibold text-slate-700 dark:text-slate-300">
              Thermal Guard Trip Ceiling
            </span>
            <span className="font-mono font-bold text-amber-500 tabular-nums">
              {config.thermalTripCelsius.toFixed(1)}°C
            </span>
          </div>
          <input
            type="range"
            min="40.0"
            max="48.0"
            step="0.5"
            value={config.thermalTripCelsius}
            onChange={(e) => {
              const val = parseFloat(e.target.value);
              update('thermalTripCelsius', Number.isFinite(val) ? Math.min(48, Math.max(40, val)) : 44.0);
            }}
            className="w-full accent-sky-500 cursor-pointer"
          />
          <div className="flex justify-between text-[10px] text-slate-400 font-mono mt-1">
            <span>40.0°C (Conservative)</span>
            <span>48.0°C (High Gaming)</span>
          </div>
          <p className="text-[11px] text-slate-500 dark:text-slate-400 mt-2">
            Engine proactively clamps top-app before OEM kernel engages severe thermal throttling.
          </p>
        </div>

        {/* Touch Boost Floor Slider */}
        <div className="rounded-2xl bg-white/70 dark:bg-slate-900/60 border border-slate-200/70 dark:border-slate-800/80 p-4">
          <div className="flex items-center justify-between text-xs mb-2">
            <span className="font-semibold text-slate-700 dark:text-slate-300">
              Touch Boost Capacity Floor
            </span>
            <span className="font-mono font-bold text-emerald-500 tabular-nums">
              {config.touchBoostMin} / 1024
            </span>
          </div>
          <input
            type="range"
            min="200"
            max="600"
            step="20"
            value={config.touchBoostMin}
            onChange={(e) => {
              const val = parseInt(e.target.value, 10);
              update('touchBoostMin', Number.isFinite(val) ? Math.min(600, Math.max(200, val)) : 380);
            }}
            className="w-full accent-sky-500 cursor-pointer"
          />
          <div className="flex justify-between text-[10px] text-slate-400 font-mono mt-1">
            <span>200 (Subtle)</span>
            <span>600 (Ultra-Responsive)</span>
          </div>
          <p className="text-[11px] text-slate-500 dark:text-slate-400 mt-2">
            Applied for {config.burstDurationMs}ms upon touchscreen touch/scroll velocity.
          </p>
        </div>
      </div>

      {/* Modern Kernel Switches */}
      <div className="grid grid-cols-1 sm:grid-cols-2 gap-4 mt-5 pt-4 border-t border-slate-200/60 dark:border-slate-800/60">
        <label className="flex items-start gap-3 cursor-pointer">
          <input
            type="checkbox"
            checked={config.unlockVendorClamping}
            onChange={(e) => update('unlockVendorClamping', e.target.checked)}
            className="mt-0.5 w-4 h-4 rounded text-sky-500 focus:ring-sky-500 accent-sky-500"
          />
          <div>
            <div className="text-xs font-semibold text-slate-800 dark:text-slate-200">
              Unlock Vendor Frequency Restraints
            </div>
            <div className="text-[11px] text-slate-500 dark:text-slate-400">
              Overrides proprietary OEM vendor clamp limits on interactive top-apps.
            </div>
          </div>
        </label>

        <label className="flex items-start gap-3 cursor-pointer">
          <input
            type="checkbox"
            checked={config.proactiveMemoryGuardian}
            onChange={(e) => update('proactiveMemoryGuardian', e.target.checked)}
            className="mt-0.5 w-4 h-4 rounded text-sky-500 focus:ring-sky-500 accent-sky-500"
          />
          <div>
            <div className="text-xs font-semibold text-slate-800 dark:text-slate-200">
              Linux PSI Memory Guardian
            </div>
            <div className="text-[11px] text-slate-500 dark:text-slate-400">
              Monitors Pressure Stall Information to prevent aggressive Android LMK kills.
            </div>
          </div>
        </label>
      </div>
    </div>
  );
};
