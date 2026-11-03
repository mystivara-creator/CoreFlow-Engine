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

  const update = <K extends keyof HardwareLongevityConfig>(key: K, value: HardwareLongevityConfig[K]) => {
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
    <div className="rounded-3xl bg-white dark:bg-[#111827] border border-slate-200 dark:border-slate-800 p-5 sm:p-7 shadow-sm space-y-6">
      {/* Header */}
      <div className="flex flex-col sm:flex-row sm:items-center justify-between gap-3">
        <div>
          <div className="flex items-center gap-2 mb-1">
            <ShieldCheck className="w-4 h-4 text-emerald-500" />
            <h3 className="text-base sm:text-lg font-bold text-slate-900 dark:text-white">
              Hardware Longevity & Silicon Health
            </h3>
            <span className="px-2.5 py-0.5 rounded-full text-xs font-semibold bg-emerald-500/10 text-emerald-500 border border-emerald-500/20">
              Active Protection
            </span>
            <span className="px-2 py-0.5 rounded-full text-[10px] font-semibold bg-slate-100 dark:bg-slate-800 text-slate-600 dark:text-slate-400 border border-slate-300 dark:border-slate-700">
              {nativeMode ? 'cgroups v2 Active' : 'Preview Sandbox'}
            </span>
          </div>
          <p className="text-xs text-slate-500 dark:text-slate-400">
            Protects lithium-ion battery chemistry from thermal degradation and paces UFS storage write bursts.
          </p>
        </div>

        <div className="flex items-center gap-2">
          {saveMessage && (
            <span className="text-xs font-medium text-emerald-500 flex items-center gap-1">
              <CheckCircle2 className="w-3.5 h-3.5" />
              {saveMessage}
            </span>
          )}
          <button
            onClick={handleReset}
            className="px-3 py-1.5 rounded-xl border border-slate-200 dark:border-slate-700 text-xs text-slate-600 dark:text-slate-400 hover:bg-slate-100 dark:hover:bg-slate-800 flex items-center gap-1.5"
            title="Reset to factory safety defaults"
          >
            <RotateCcw className="w-3.5 h-3.5" />
            Reset
          </button>
          <button
            onClick={handleSave}
            disabled={isSaving || !hasChanges}
            className="px-4 py-1.5 rounded-xl bg-emerald-600 hover:bg-emerald-500 disabled:opacity-50 text-white text-xs font-semibold shadow-sm shadow-emerald-500/20 transition-all flex items-center gap-1.5"
          >
            {isSaving ? 'Applying...' : 'Save & Arm'}
          </button>
        </div>
      </div>

      <div className="grid grid-cols-1 md:grid-cols-2 gap-5">
        {/* Subsystem 1: Battery Step-Charging Protection */}
        <div className="p-5 rounded-2xl bg-slate-50 dark:bg-slate-900/60 border border-slate-200/80 dark:border-slate-800 space-y-4">
          <div className="flex items-center justify-between">
            <div className="flex items-center gap-2">
              <div className="p-2 rounded-xl bg-emerald-500/10 text-emerald-500">
                <BatteryCharging className="w-4 h-4" />
              </div>
              <div>
                <h4 className="text-xs font-bold text-slate-900 dark:text-white">
                  Battery Step-Charging Guard
                </h4>
                <div className="text-[11px] text-slate-400">
                  Throttles current at elevated thermals
                </div>
              </div>
            </div>

            <label className="relative inline-flex items-center cursor-pointer">
              <input
                type="checkbox"
                checked={local.enableBatteryStepCharging}
                onChange={(e) => update('enableBatteryStepCharging', e.target.checked)}
                className="sr-only peer"
              />
              <div className="w-9 h-5 bg-slate-300 peer-focus:outline-none rounded-full peer dark:bg-slate-700 peer-checked:after:translate-x-full peer-checked:after:border-white after:content-[''] after:absolute after:top-[2px] after:left-[2px] after:bg-white after:border-slate-300 after:border after:rounded-full after:h-4 after:w-4 after:transition-all dark:border-slate-600 peer-checked:bg-emerald-500"></div>
            </label>
          </div>

          <p className="text-[11px] text-slate-500 dark:text-slate-400 leading-relaxed">
            High voltage coupled with temperatures above 42°C accelerates lithium plating and capacity fade. When active, CoreFlow steps down charging current during heavy computing sessions.
          </p>

          <div className="space-y-3 pt-2">
            <div>
              <div className="flex justify-between text-xs text-slate-600 dark:text-slate-300 mb-1">
                <span>Thermal Step Trigger</span>
                <span className="font-mono font-bold text-emerald-500">
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
                className="w-full accent-emerald-500"
              />
              <div className="flex justify-between text-[10px] text-slate-400 mt-1 font-mono">
                <span>38.0°C (Aggressive)</span>
                <span>41.5°C (Recommended)</span>
                <span>46.0°C (Permissive)</span>
              </div>
            </div>

            <div>
              <div className="flex justify-between text-xs text-slate-600 dark:text-slate-300 mb-1">
                <span>Stepped Max Current</span>
                <span className="font-mono font-bold text-emerald-500">
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
                className="w-full accent-emerald-500"
              />
              <div className="flex justify-between text-[10px] text-slate-400 mt-1 font-mono">
                <span>800 mA (Slow trickle)</span>
                <span>1500 mA (Balanced)</span>
                <span>2500 mA (Fast)</span>
              </div>
            </div>
          </div>
        </div>

        {/* Subsystem 2: UFS Storage Burst Pacing */}
        <div className="p-5 rounded-2xl bg-slate-50 dark:bg-slate-900/60 border border-slate-200/80 dark:border-slate-800 space-y-4">
          <div className="flex items-center justify-between">
            <div className="flex items-center gap-2">
              <div className="p-2 rounded-xl bg-sky-500/10 text-sky-500">
                <HardDrive className="w-4 h-4" />
              </div>
              <div>
                <h4 className="text-xs font-bold text-slate-900 dark:text-white">
                  UFS Storage I/O Burst Pacing
                </h4>
                <div className="text-[11px] text-slate-400">
                  cgroups v2 writeback queue pacing
                </div>
              </div>
            </div>

            <label className="relative inline-flex items-center cursor-pointer">
              <input
                type="checkbox"
                checked={local.enableUfsBurstPacing}
                onChange={(e) => update('enableUfsBurstPacing', e.target.checked)}
                className="sr-only peer"
              />
              <div className="w-9 h-5 bg-slate-300 peer-focus:outline-none rounded-full peer dark:bg-slate-700 peer-checked:after:translate-x-full peer-checked:after:border-white after:content-[''] after:absolute after:top-[2px] after:left-[2px] after:bg-white after:border-slate-300 after:border after:rounded-full after:h-4 after:w-4 after:transition-all dark:border-slate-600 peer-checked:bg-sky-500"></div>
            </label>
          </div>

          <p className="text-[11px] text-slate-500 dark:text-slate-400 leading-relaxed">
            Eliminates high-priority UI latency spikes caused by background applications saturating flash controller write buffers. Allocates 800+ IO weight to top-app while pacing background writes.
          </p>

          <div className="space-y-3 pt-2">
            <div>
              <div className="flex justify-between text-xs text-slate-600 dark:text-slate-300 mb-1">
                <span>Foreground App I/O Priority</span>
                <span className="font-mono font-bold text-sky-500">
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
                className="w-full accent-sky-500"
              />
              <div className="flex justify-between text-[10px] text-slate-400 mt-1 font-mono">
                <span>400</span>
                <span>800 (Top-app boost)</span>
                <span>1000 (Max)</span>
              </div>
            </div>

            <div>
              <div className="flex justify-between text-xs text-slate-600 dark:text-slate-300 mb-1">
                <span>Background App I/O Cap</span>
                <span className="font-mono font-bold text-sky-500">
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
                className="w-full accent-sky-500"
              />
              <div className="flex justify-between text-[10px] text-slate-400 mt-1 font-mono">
                <span>10 (Aggressive clamp)</span>
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
