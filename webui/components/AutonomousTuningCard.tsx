import React from 'react';
import { Battery, Cpu, Sliders, Zap } from 'lucide-react';
import { AutonomousConfig } from '../engine/autonomousEngine';

interface Props {
  config: AutonomousConfig;
  nativeMode?: boolean;
  onChangeConfig: (next: AutonomousConfig) => void;
  onSaveConfig: () => void;
  isSaving: boolean;
  saveMessage: string | null;
}

export const AutonomousTuningCard: React.FC<Props> = ({
  config,
  nativeMode = false,
  onChangeConfig,
  onSaveConfig,
  isSaving,
  saveMessage,
}) => {
  const update = <K extends keyof AutonomousConfig>(key: K, value: AutonomousConfig[K]) =>
    onChangeConfig({ ...config, [key]: value });

  return (
    <div className="rounded-3xl bg-slate-100/80 dark:bg-[#111827]/90 border border-slate-200/90 dark:border-slate-800 p-5 sm:p-7 shadow-lg shadow-slate-900/5 transition-all">
      <div className="flex flex-col sm:flex-row sm:items-center justify-between gap-3 mb-6 pb-4 border-b border-slate-200/60 dark:border-slate-800/60">
        <div>
          <div className="flex items-center gap-2 mb-1">
            <Sliders className="w-4 h-4 text-sky-500" />
            <h3 className="font-bold text-lg text-slate-900 dark:text-white">Stability Policy</h3>
          </div>
          <p className="text-xs text-slate-500 dark:text-slate-400">
            One balanced UClamp baseline plus thermal and pressure guards.
          </p>
        </div>
        <div className="flex items-center gap-2">
          {saveMessage && <span className="text-xs text-emerald-500 font-medium">{saveMessage}</span>}
          <button
            onClick={onSaveConfig}
            disabled={isSaving}
            className="px-4 py-2 rounded-full bg-sky-500 hover:bg-sky-400 text-slate-950 font-semibold text-xs transition-all active:scale-95 disabled:opacity-50"
          >
            {isSaving ? 'Applying...' : nativeMode ? 'Apply Policy' : 'Apply Preview'}
          </button>
        </div>
      </div>

      <div className="grid grid-cols-1 md:grid-cols-3 gap-5">
        <div>
          <label className="block text-xs font-semibold uppercase tracking-wider text-slate-600 dark:text-slate-400 mb-2">
            Baseline Bias
          </label>
          <div className="grid grid-cols-3 gap-1.5 p-1 rounded-2xl bg-slate-200/70 dark:bg-slate-900/80">
            {([
              ['battery', 'Battery', Battery],
              ['balanced', 'Balanced', Cpu],
              ['responsiveness', 'Responsive', Zap],
            ] as const).map(([id, label, Icon]) => (
              <button
                key={id}
                type="button"
                onClick={() => update('adaptiveBias', id)}
                className={'flex flex-col items-center py-2 rounded-xl text-xs font-semibold transition-all ' +
                  (config.adaptiveBias === id
                    ? 'bg-white dark:bg-slate-800 text-sky-500 shadow-sm'
                    : 'text-slate-500 dark:text-slate-400')}
              >
                <Icon className="w-4 h-4 mb-1" />
                {label}
              </button>
            ))}
          </div>
          <p className="text-[11px] text-slate-500 dark:text-slate-400 mt-2">
            Adjusts only the balanced floor. Thermal and system pressure always have priority.
          </p>
        </div>

        <div className="rounded-2xl bg-white/70 dark:bg-slate-900/60 border border-slate-200/70 dark:border-slate-800/80 p-4">
          <div className="flex items-center justify-between text-xs mb-2">
            <span className="font-semibold">Balanced UClamp Floor</span>
            <span className="font-mono font-bold text-sky-500">{config.balancedUclampMin} / 1024</span>
          </div>
          <input
            type="range"
            min="0"
            max="640"
            step="16"
            value={config.balancedUclampMin}
            onChange={(e) => update('balancedUclampMin', Math.max(0, Math.min(640, Number(e.target.value))))}
            className="w-full accent-sky-500 cursor-pointer"
          />
          <p className="text-[11px] text-slate-500 dark:text-slate-400 mt-2">
            SustainedLoad rises proportionally from this value; PressureGuard backs off from it.
          </p>
        </div>

        <div className="rounded-2xl bg-white/70 dark:bg-slate-900/60 border border-slate-200/70 dark:border-slate-800/80 p-4">
          <div className="flex items-center justify-between text-xs mb-2">
            <span className="font-semibold">Thermal Guard Trip</span>
            <span className="font-mono font-bold text-amber-500">{config.thermalTripCelsius.toFixed(1)}°C</span>
          </div>
          <input
            type="range"
            min="40"
            max="48"
            step="0.5"
            value={config.thermalTripCelsius}
            onChange={(e) => update('thermalTripCelsius', Math.max(40, Math.min(48, Number(e.target.value))))}
            className="w-full accent-sky-500 cursor-pointer"
          />
          <p className="text-[11px] text-slate-500 dark:text-slate-400 mt-2">
            Above this point performance boosting stops taking priority over component health.
          </p>
        </div>
      </div>
    </div>
  );
};
