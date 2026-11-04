import React, { useState } from 'react';
import { BatteryCharging, HardDrive, ShieldCheck, CheckCircle2, RotateCcw } from 'lucide-react';
import { HardwareLongevityConfig, DEFAULT_LONGEVITY_CONFIG } from '../engine/autonomousEngine';

interface Props {
  longevityConfig: HardwareLongevityConfig;
  nativeMode: boolean;
  onSaveLongevity: (config: HardwareLongevityConfig) => Promise<void>;
  isSaving: boolean;
  saveMessage: string | null;
}

export const HardwareLongevityCard: React.FC<Props> = ({
  longevityConfig,
  nativeMode,
  onSaveLongevity,
  isSaving,
  saveMessage,
}) => {
  const [local, setLocal] = useState<HardwareLongevityConfig>(longevityConfig);
  const [hasChanges, setHasChanges] = useState(false);

  const update = <K extends keyof HardwareLongevityConfig>(
    key: K,
    value: HardwareLongevityConfig[K]
  ) => {
    setLocal((prev) => {
      const next = { ...prev, [key]: value };
      setHasChanges(true);
      return next;
    });
  };

  const handleReset = () => {
    setLocal({ ...DEFAULT_LONGEVITY_CONFIG });
    setHasChanges(true);
  };

  const handleSave = async () => {
    await onSaveLongevity(local);
    setHasChanges(false);
  };

  return (
    <div className="m3-card p-6 sm:p-8 space-y-6">
      {/* Header */}
      <div className="flex flex-col sm:flex-row sm:items-start justify-between gap-4 pb-5 border-b border-slate-900/[0.06] dark:border-white/[0.06]">
        <div className="space-y-1.5 max-w-2xl">
          <div className="flex flex-wrap items-center gap-2 text-xs text-slate-500 dark:text-slate-400">
            <ShieldCheck className="w-4 h-4 text-[#0b57d0] dark:text-[#a8c7fa]" />
            <span className="text-base sm:text-lg font-semibold text-slate-900 dark:text-slate-100">
              Hardware Longevity & Resource Preservation
            </span>
            <span aria-hidden="true">·</span>
            <span>{nativeMode ? 'cgroup v2 & Power Supply Active' : 'Preview Sandbox'}</span>
          </div>
          <p className="text-xs sm:text-sm text-slate-600 dark:text-slate-400 leading-relaxed">
            Applies bounded, ownership-verified controls: steps down charging current strictly when battery temperature is elevated while charging, and balances cgroup v2 relative I/O weights.
          </p>
        </div>

        <div className="flex items-center gap-2 shrink-0 self-start">
          {saveMessage && (
            <span className="text-xs font-medium text-[#0b57d0] dark:text-[#a8c7fa] flex items-center gap-1 mr-1">
              <CheckCircle2 className="w-3.5 h-3.5" />
              {saveMessage}
            </span>
          )}
          <button
            onClick={handleReset}
            className="px-4 py-2 rounded-full bg-slate-900/[0.05] hover:bg-slate-900/[0.09] dark:bg-white/[0.06] dark:hover:bg-white/[0.11] text-xs font-medium text-slate-700 dark:text-slate-300 inline-flex items-center gap-1.5"
            title="Reset to defaults"
          >
            <RotateCcw className="w-3.5 h-3.5" />
            <span>Reset</span>
          </button>
          <button
            onClick={handleSave}
            disabled={isSaving || !hasChanges}
            className="px-5 py-2 rounded-full bg-[#0b57d0] hover:bg-[#0842a0] dark:bg-[#a8c7fa] dark:hover:bg-[#8ab4f8] disabled:opacity-40 text-white dark:text-[#062e6f] text-xs font-medium transition-all"
          >
            {isSaving ? 'Applying...' : 'Save & Arm'}
          </button>
        </div>
      </div>

      <div className="grid grid-cols-1 md:grid-cols-2 gap-5">
        {/* Subsystem 1: Battery Step-Charging Protection */}
        <div className="m3-tonal-block p-6 space-y-5 flex flex-col justify-between">
          <div className="space-y-4">
            <div className="flex items-center justify-between gap-3">
              <div className="flex items-center gap-3">
                <div className="w-10 h-10 rounded-2xl bg-[#d3e3fd] dark:bg-[#0842a0] text-[#041e49] dark:text-[#d3e3fd] flex items-center justify-center shrink-0">
                  <BatteryCharging className="w-4 h-4" />
                </div>
                <div>
                  <h4 className="text-sm font-semibold text-slate-900 dark:text-slate-100">
                    Battery Step-Charging Guard
                  </h4>
                  <div className="text-xs text-slate-500 dark:text-slate-400">
                    Standard `constant_charge_current_max` ceiling reduction
                  </div>
                </div>
              </div>

              <button
                type="button"
                role="switch"
                aria-checked={local.enableBatteryStepCharging}
                onClick={() => update('enableBatteryStepCharging', !local.enableBatteryStepCharging)}
                className={
                  'w-12 h-7 rounded-full p-1 transition-colors flex items-center shrink-0 ' +
                  (local.enableBatteryStepCharging
                    ? 'bg-[#0b57d0] dark:bg-[#a8c7fa] justify-end'
                    : 'bg-slate-300 dark:bg-slate-700 justify-start')
                }
              >
                <span className="w-5 h-5 rounded-full bg-white dark:bg-[#0e1117] shadow-xs" />
              </button>
            </div>

            <p className="text-xs text-slate-600 dark:text-slate-400 leading-relaxed">
              Evaluates battery sensor temperature exclusively while charging. Reduces the charging ceiling during elevated heat and never raises an OEM current limit.
            </p>
          </div>

          <div className="space-y-4 pt-2 border-t border-slate-900/[0.06] dark:border-white/[0.06]">
            <div>
              <div className="flex justify-between text-xs text-slate-700 dark:text-slate-300 mb-2">
                <span className="font-medium">Battery Thermal Step Trigger</span>
                <span className="font-mono tabular-nums font-semibold text-[#0b57d0] dark:text-[#a8c7fa]">
                  {local.batteryStepTempThreshold.toFixed(1)} °C
                </span>
              </div>
              <input
                type="range"
                min="38"
                max="46"
                step="0.5"
                value={local.batteryStepTempThreshold}
                onChange={(e) => update('batteryStepTempThreshold', parseFloat(e.target.value))}
                className="w-full"
              />
              <div className="flex justify-between text-[11px] text-slate-400 mt-1.5 font-mono tabular-nums">
                <span>38.0 °C</span>
                <span>41.5 °C (Default)</span>
                <span>46.0 °C</span>
              </div>
            </div>

            <div>
              <div className="flex justify-between text-xs text-slate-700 dark:text-slate-300 mb-2">
                <span className="font-medium">Stepped Current Ceiling</span>
                <span className="font-mono tabular-nums font-semibold text-[#0b57d0] dark:text-[#a8c7fa]">
                  {local.steppedChargeCurrentMa} mA
                </span>
              </div>
              <input
                type="range"
                min="800"
                max="2500"
                step="100"
                value={local.steppedChargeCurrentMa}
                onChange={(e) => update('steppedChargeCurrentMa', parseInt(e.target.value, 10))}
                className="w-full"
              />
              <div className="flex justify-between text-[11px] text-slate-400 mt-1.5 font-mono tabular-nums">
                <span>800 mA</span>
                <span>1500 mA (Balanced)</span>
                <span>2500 mA</span>
              </div>
            </div>
          </div>
        </div>

        {/* Subsystem 2: UFS Storage Burst Pacing */}
        <div className="m3-tonal-block p-6 space-y-5 flex flex-col justify-between">
          <div className="space-y-4">
            <div className="flex items-center justify-between gap-3">
              <div className="flex items-center gap-3">
                <div className="w-10 h-10 rounded-2xl bg-[#d3e3fd] dark:bg-[#0842a0] text-[#041e49] dark:text-[#d3e3fd] flex items-center justify-center shrink-0">
                  <HardDrive className="w-4 h-4" />
                </div>
                <div>
                  <h4 className="text-sm font-semibold text-slate-900 dark:text-slate-100">
                    Storage I/O Relative Weighting
                  </h4>
                  <div className="text-xs text-slate-500 dark:text-slate-400">
                    cgroup v2 `io.weight` contention control
                  </div>
                </div>
              </div>

              <button
                type="button"
                role="switch"
                aria-checked={local.enableUfsBurstPacing}
                onClick={() => update('enableUfsBurstPacing', !local.enableUfsBurstPacing)}
                className={
                  'w-12 h-7 rounded-full p-1 transition-colors flex items-center shrink-0 ' +
                  (local.enableUfsBurstPacing
                    ? 'bg-[#0b57d0] dark:bg-[#a8c7fa] justify-end'
                    : 'bg-slate-300 dark:bg-slate-700 justify-start')
                }
              >
                <span className="w-5 h-5 rounded-full bg-white dark:bg-[#0e1117] shadow-xs" />
              </button>
            </div>

            <p className="text-xs text-slate-600 dark:text-slate-400 leading-relaxed">
              Prioritizes foreground interactive I/O queues over background write bursts using documented `default N` cgroup v2 weights without modifying global VM ratios.
            </p>
          </div>

          <div className="space-y-4 pt-2 border-t border-slate-900/[0.06] dark:border-white/[0.06]">
            <div>
              <div className="flex justify-between text-xs text-slate-700 dark:text-slate-300 mb-2">
                <span className="font-medium">Foreground Top-App I/O Weight</span>
                <span className="font-mono tabular-nums font-semibold text-[#0b57d0] dark:text-[#a8c7fa]">
                  weight {local.topAppIoWeight}
                </span>
              </div>
              <input
                type="range"
                min="400"
                max="1000"
                step="50"
                value={local.topAppIoWeight}
                onChange={(e) => update('topAppIoWeight', parseInt(e.target.value, 10))}
                className="w-full"
              />
              <div className="flex justify-between text-[11px] text-slate-400 mt-1.5 font-mono tabular-nums">
                <span>400</span>
                <span>800 (Recommended)</span>
                <span>1000</span>
              </div>
            </div>

            <div>
              <div className="flex justify-between text-xs text-slate-700 dark:text-slate-300 mb-2">
                <span className="font-medium">Background I/O Weight</span>
                <span className="font-mono tabular-nums font-semibold text-[#0b57d0] dark:text-[#a8c7fa]">
                  weight {local.backgroundIoWeight}
                </span>
              </div>
              <input
                type="range"
                min="10"
                max="200"
                step="10"
                value={local.backgroundIoWeight}
                onChange={(e) => update('backgroundIoWeight', parseInt(e.target.value, 10))}
                className="w-full"
              />
              <div className="flex justify-between text-[11px] text-slate-400 mt-1.5 font-mono tabular-nums">
                <span>10</span>
                <span>50 (Recommended)</span>
                <span>200</span>
              </div>
            </div>
          </div>
        </div>
      </div>
    </div>
  );
};
