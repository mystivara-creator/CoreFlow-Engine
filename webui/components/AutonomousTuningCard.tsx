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
    <div className="m3-card p-6 sm:p-8">
      <div className="flex flex-col sm:flex-row sm:items-center justify-between gap-4 pb-6 mb-6 border-b border-slate-900/[0.06] dark:border-white/[0.06]">
        <div className="space-y-1">
          <div className="flex items-center gap-2">
            <Sliders className="w-4 h-4 text-[#0b57d0] dark:text-[#a8c7fa]" />
            <h3 className="font-semibold text-lg text-slate-900 dark:text-slate-100">
              Coordinated Policy & Risk Budget
            </h3>
          </div>
          <p className="text-xs sm:text-sm text-slate-600 dark:text-slate-400">
            Configures the target UClamp floor for grouped scheduler transactions while honoring OEM thermal ceilings.
          </p>
        </div>

        <div className="flex items-center gap-3 shrink-0">
          {saveMessage && (
            <span className="text-xs font-medium text-[#0b57d0] dark:text-[#a8c7fa]">
              {saveMessage}
            </span>
          )}
          <button
            onClick={onSaveConfig}
            disabled={isSaving}
            className="px-5 py-2.5 rounded-full bg-[#0b57d0] hover:bg-[#0842a0] dark:bg-[#a8c7fa] dark:hover:bg-[#8ab4f8] text-white dark:text-[#062e6f] font-medium text-xs disabled:opacity-50"
          >
            {isSaving ? 'Applying...' : nativeMode ? 'Commit Policy' : 'Apply Preview'}
          </button>
        </div>
      </div>

      <div className="grid grid-cols-1 md:grid-cols-3 gap-5">
        {/* M3 Segmented Button Group */}
        <div className="m3-tonal-block p-5 flex flex-col justify-between">
          <div>
            <label className="block text-xs font-semibold text-slate-800 dark:text-slate-200 mb-3">
              Optimization Intent Bias
            </label>
            <div className="grid grid-cols-3 gap-1 p-1 rounded-full bg-slate-900/[0.06] dark:bg-white/[0.06]">
              {([
                ['battery', 'Battery', Battery],
                ['balanced', 'Balanced', Cpu],
                ['responsiveness', 'Responsive', Zap],
              ] as const).map(([id, label, Icon]) => {
                const active = config.adaptiveBias === id;
                return (
                  <button
                    key={id}
                    type="button"
                    onClick={() => update('adaptiveBias', id)}
                    className={
                      'flex items-center justify-center gap-1.5 py-2 px-2 rounded-full text-xs font-medium transition-all ' +
                      (active
                        ? 'bg-[#d3e3fd] text-[#041e49] dark:bg-[#0842a0] dark:text-[#d3e3fd] font-semibold shadow-xs'
                        : 'text-slate-600 dark:text-slate-400 hover:text-slate-900 dark:hover:text-slate-200')
                    }
                  >
                    <Icon className="w-3.5 h-3.5 shrink-0" />
                    <span className="truncate">{label}</span>
                  </button>
                );
              })}
            </div>
          </div>
          <p className="text-xs text-slate-500 dark:text-slate-400 mt-4 leading-relaxed">
            Scales the baseline UClamp floor. OEM thermal and PSI pressure guards always retain override priority.
          </p>
        </div>

        {/* Balanced UClamp Floor Slider */}
        <div className="m3-tonal-block p-5 flex flex-col justify-between">
          <div>
            <div className="flex items-center justify-between text-xs mb-3">
              <span className="font-semibold text-slate-800 dark:text-slate-200">
                Balanced UClamp Floor
              </span>
              <span className="font-mono tabular-nums font-semibold text-[#0b57d0] dark:text-[#a8c7fa]">
                {config.balancedUclampMin} / 1024
              </span>
            </div>
            <input
              type="range"
              min="0"
              max="640"
              step="16"
              value={config.balancedUclampMin}
              onChange={(e) =>
                update('balancedUclampMin', Math.max(0, Math.min(640, Number(e.target.value))))
              }
              className="w-full"
            />
          </div>
          <p className="text-xs text-slate-500 dark:text-slate-400 mt-4 leading-relaxed">
            SustainedLoad transactions scale up proportionally from this baseline; PressureGuard steps down cleanly.
          </p>
        </div>

        {/* Thermal Policy Slider */}
        <div className="m3-tonal-block p-5 flex flex-col justify-between">
          <div>
            <div className="flex items-center justify-between text-xs mb-3">
              <span className="font-semibold text-slate-800 dark:text-slate-200">
                {nativeMode ? 'OEM Thermal Policy' : 'Preview Thermal Trip'}
              </span>
              <span className="font-mono tabular-nums font-semibold text-[#0b57d0] dark:text-[#a8c7fa]">
                {nativeMode ? 'OEM / Auto' : `${config.thermalTripCelsius.toFixed(1)} °C`}
              </span>
            </div>
            <input
              type="range"
              disabled={nativeMode}
              min="40"
              max="48"
              step="0.5"
              value={config.thermalTripCelsius}
              onChange={(e) =>
                update('thermalTripCelsius', Math.max(40, Math.min(48, Number(e.target.value))))
              }
              className="w-full disabled:opacity-40"
            />
          </div>
          <p className="text-xs text-slate-500 dark:text-slate-400 mt-4 leading-relaxed">
            {nativeMode
              ? 'Native daemon strictly honors Android NDK thermal severity and per-zone OEM trip points.'
              : 'Synthetic preview threshold for testing ThermalGuard state transitions in the browser.'}
          </p>
        </div>
      </div>
    </div>
  );
};
