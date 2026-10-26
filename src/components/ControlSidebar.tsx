import React from 'react';
import { RotateCcw, StepForward } from 'lucide-react';
import { HARDWARE_PROFILES, SCENARIO_PRESETS } from '../engine/presets';
import {
  AppClass,
  CapabilityOverrides,
  DevicePresetId,
  WorkloadInputs,
} from '../engine/types';

interface ControlSidebarProps {
  deviceId: DevicePresetId;
  onChangeDevice: (id: DevicePresetId) => void;
  capabilities: CapabilityOverrides;
  onChangeCapabilities: (next: CapabilityOverrides) => void;
  inputs: WorkloadInputs;
  onChangeInputs: (next: WorkloadInputs) => void;
  activeScenarioId: string | null;
  onApplyScenario: (scenarioId: string) => void;
  onStepTick: () => void;
  onResetEngine: () => void;
}

export const ControlSidebar: React.FC<ControlSidebarProps> = ({
  deviceId,
  onChangeDevice,
  capabilities,
  onChangeCapabilities,
  inputs,
  onChangeInputs,
  activeScenarioId,
  onApplyScenario,
  onStepTick,
  onResetEngine,
}) => {
  const updateField = <K extends keyof WorkloadInputs>(
    key: K,
    value: WorkloadInputs[K]
  ) => {
    onChangeInputs({ ...inputs, [key]: value });
  };

  return (
    <aside className="w-full lg:w-80 shrink-0 bg-white border-b lg:border-b-0 lg:border-r border-slate-200">
      <div className="p-5 space-y-6">
        <div className="flex items-center justify-between gap-2">
          <h2 className="text-sm font-semibold text-slate-900">Hardware & Telemetry</h2>
          <div className="flex items-center gap-1.5">
            <button
              type="button"
              onClick={onStepTick}
              title="Advance 1 observation tick immediately"
              className="inline-flex items-center gap-1.5 px-2.5 py-1.5 text-xs font-medium text-slate-700 bg-slate-100 rounded-md hover:bg-slate-200 transition-colors whitespace-nowrap shrink-0"
            >
              <StepForward className="w-3.5 h-3.5" aria-hidden="true" />
              <span>Step Tick</span>
            </button>
            <button
              type="button"
              onClick={onResetEngine}
              title="Reset daemon history & baseline"
              className="inline-flex items-center gap-1 px-2.5 py-1.5 text-xs font-medium text-slate-600 hover:text-slate-900 bg-slate-100 rounded-md hover:bg-slate-200 transition-colors whitespace-nowrap shrink-0"
            >
              <RotateCcw className="w-3.5 h-3.5" aria-hidden="true" />
              <span>Reset</span>
            </button>
          </div>
        </div>

        {/* Target Device Profile */}
        <div className="space-y-2">
          <label
            htmlFor="device-preset-select"
            className="block text-xs font-medium text-slate-700"
          >
            Target Android Device Profile
          </label>
          <select
            id="device-preset-select"
            value={deviceId}
            onChange={(e) => onChangeDevice(e.target.value as DevicePresetId)}
            className="w-full px-3 py-2 text-xs font-medium text-slate-900 bg-slate-50 border border-slate-200 rounded-lg focus:outline-none focus:ring-2 focus:ring-blue-600"
          >
            {Object.values(HARDWARE_PROFILES).map((prof) => (
              <option key={prof.id} value={prof.id}>
                {prof.name}
              </option>
            ))}
          </select>
          <p className="text-xs text-slate-500">
            {HARDWARE_PROFILES[deviceId].soc} · {HARDWARE_PROFILES[deviceId].gpu}
          </p>
        </div>

        {/* Workload Presets */}
        <div className="space-y-2 pt-4 border-t border-slate-200">
          <div className="text-xs font-medium text-slate-700">Workload Scenarios</div>
          <div className="grid grid-cols-2 gap-1.5">
            {SCENARIO_PRESETS.map((sc) => {
              const selected = activeScenarioId === sc.id;
              return (
                <button
                  key={sc.id}
                  type="button"
                  onClick={() => onApplyScenario(sc.id)}
                  className={`px-2.5 py-2 text-left text-xs font-medium rounded-md border transition-colors truncate ${
                    selected
                      ? 'bg-blue-50 border-blue-600 text-blue-800'
                      : 'bg-slate-50 border-slate-200 text-slate-700 hover:bg-slate-100 hover:text-slate-900'
                  }`}
                >
                  {sc.label}
                </button>
              );
            })}
          </div>
        </div>

        {/* Display & App State */}
        <div className="space-y-3 pt-4 border-t border-slate-200">
          <div className="flex items-center justify-between">
            <span className="text-xs font-medium text-slate-700">Display State (DRM/FB0)</span>
            <div className="flex items-center gap-1 p-0.5 bg-slate-100 rounded-md">
              <button
                type="button"
                onClick={() => updateField('screenOn', true)}
                className={`px-2.5 py-1 text-xs font-medium rounded transition-colors whitespace-nowrap ${
                  inputs.screenOn
                    ? 'bg-white text-slate-900 shadow-xs'
                    : 'text-slate-600 hover:text-slate-900'
                }`}
              >
                Screen On
              </button>
              <button
                type="button"
                onClick={() => updateField('screenOn', false)}
                className={`px-2.5 py-1 text-xs font-medium rounded transition-colors whitespace-nowrap ${
                  !inputs.screenOn
                    ? 'bg-white text-slate-900 shadow-xs'
                    : 'text-slate-600 hover:text-slate-900'
                }`}
              >
                Screen Off
              </button>
            </div>
          </div>

          <div className="space-y-1.5">
            <label
              htmlFor="foreground-app-select"
              className="block text-xs font-medium text-slate-700"
            >
              Foreground Process Class (/proc/pid/cmdline)
            </label>
            <select
              id="foreground-app-select"
              value={inputs.foregroundApp}
              onChange={(e) => {
                const nextClass = e.target.value as AppClass;
                const pkg =
                  nextClass === AppClass.Game
                    ? 'com.miHoYo.GenshinImpact'
                    : nextClass === AppClass.Launcher
                      ? 'com.miui.home'
                      : 'com.android.chrome';
                onChangeInputs({
                  ...inputs,
                  foregroundApp: nextClass,
                  foregroundPackage: pkg,
                });
              }}
              className="w-full px-3 py-2 text-xs font-medium text-slate-900 bg-slate-50 border border-slate-200 rounded-lg focus:outline-none focus:ring-2 focus:ring-blue-600"
            >
              <option value={AppClass.Default}>APP_DEFAULT (Standard App)</option>
              <option value={AppClass.Game}>APP_GAME (Game Engine Detected)</option>
              <option value={AppClass.Launcher}>APP_LAUNCHER (Home / HyperOS)</option>
            </select>
          </div>
        </div>

        {/* Telemetry Sliders */}
        <div className="space-y-4 pt-4 border-t border-slate-200">
          <div className="text-xs font-semibold text-slate-900">Live Sensor Inputs</div>

          <div className="space-y-1">
            <div className="flex justify-between text-xs">
              <label htmlFor="slider-soc-temp" className="text-slate-600">
                SoC Primary Temp
              </label>
              <span className="font-mono font-medium text-slate-900 tabular-nums">
                {inputs.socTempC.toFixed(1)} °C
              </span>
            </div>
            <input
              id="slider-soc-temp"
              type="range"
              min={26}
              max={52}
              step={0.2}
              value={inputs.socTempC}
              onChange={(e) => updateField('socTempC', Number(e.target.value))}
              className="w-full accent-blue-600"
            />
            <div className="flex justify-between text-[11px] text-slate-400 font-mono tabular-nums">
              <span>Exit &le;40°C</span>
              <span>Guard &ge;43°C</span>
            </div>
          </div>

          <div className="space-y-1">
            <div className="flex justify-between text-xs">
              <label htmlFor="slider-cpu" className="text-slate-600">
                CPU Active Utilization
              </label>
              <span className="font-mono font-medium text-slate-900 tabular-nums">
                {inputs.cpuUsagePct}%
              </span>
            </div>
            <input
              id="slider-cpu"
              type="range"
              min={1}
              max={100}
              step={1}
              value={inputs.cpuUsagePct}
              onChange={(e) => updateField('cpuUsagePct', Number(e.target.value))}
              className="w-full accent-blue-600"
            />
          </div>

          <div className="space-y-1">
            <div className="flex justify-between text-xs">
              <label htmlFor="slider-gpu" className="text-slate-600">
                KGSL GPU Busy
              </label>
              <span className="font-mono font-medium text-slate-900 tabular-nums">
                {capabilities.kgslAvailable ? `${inputs.gpuBusyPct}%` : 'Unavailable'}
              </span>
            </div>
            <input
              id="slider-gpu"
              type="range"
              min={0}
              max={100}
              step={1}
              disabled={!capabilities.kgslAvailable}
              value={inputs.gpuBusyPct}
              onChange={(e) => updateField('gpuBusyPct', Number(e.target.value))}
              className="w-full accent-blue-600 disabled:opacity-40"
            />
          </div>

          <div className="space-y-1">
            <div className="flex justify-between text-xs">
              <label htmlFor="slider-load1" className="text-slate-600">
                1-Min Load Average (load1)
              </label>
              <span className="font-mono font-medium text-slate-900 tabular-nums">
                {inputs.load1.toFixed(2)}
              </span>
            </div>
            <input
              id="slider-load1"
              type="range"
              min={0.02}
              max={6.0}
              step={0.05}
              value={inputs.load1}
              onChange={(e) => updateField('load1', Number(e.target.value))}
              className="w-full accent-blue-600"
            />
          </div>

          <div className="space-y-1">
            <div className="flex justify-between text-xs">
              <label htmlFor="slider-mem" className="text-slate-600">
                MemAvailable Ratio
              </label>
              <span className="font-mono font-medium text-slate-900 tabular-nums">
                {inputs.memAvailablePct.toFixed(1)}%
              </span>
            </div>
            <input
              id="slider-mem"
              type="range"
              min={4}
              max={85}
              step={0.5}
              value={inputs.memAvailablePct}
              onChange={(e) => updateField('memAvailablePct', Number(e.target.value))}
              className="w-full accent-blue-600"
            />
            <div className="flex justify-between text-[11px] text-slate-400 font-mono tabular-nums">
              <span>Pressure &lt;10%</span>
              <span>Exit &ge;15%</span>
            </div>
          </div>

          <div className="pt-2 space-y-2.5">
            <div className="flex items-center justify-between">
              <label htmlFor="toggle-charging" className="text-xs font-medium text-slate-700">
                Power Supply Charging
              </label>
              <input
                id="toggle-charging"
                type="checkbox"
                checked={inputs.charging}
                onChange={(e) => {
                  const isChg = e.target.checked;
                  onChangeInputs({
                    ...inputs,
                    charging: isChg,
                    batteryCurrentMa: isChg ? 2600 : -450,
                  });
                }}
                className="w-4 h-4 rounded border-slate-300 text-blue-600 focus:ring-blue-600"
              />
            </div>

            <div className="space-y-1">
              <div className="flex justify-between text-xs">
                <label htmlFor="slider-batt-temp" className="text-slate-600">
                  Battery Pack Temp
                </label>
                <span className="font-mono font-medium text-slate-900 tabular-nums">
                  {inputs.batteryTempC.toFixed(1)} °C
                </span>
              </div>
              <input
                id="slider-batt-temp"
                type="range"
                min={24}
                max={48}
                step={0.2}
                value={inputs.batteryTempC}
                onChange={(e) => updateField('batteryTempC', Number(e.target.value))}
                className="w-full accent-blue-600"
              />
            </div>
          </div>
        </div>

        {/* Kernel Capability Discovery Toggles */}
        <div className="space-y-2.5 pt-4 border-t border-slate-200">
          <div className="text-xs font-semibold text-slate-900">
            Exposed Kernel Interfaces
          </div>
          <p className="text-xs text-slate-500">
            Toggle sysfs nodes to test CoreFlow graceful skip adaptation.
          </p>

          {(
            [
              ['kgslAvailable', 'Qualcomm KGSL (/sys/class/kgsl)'],
              ['waltAvailable', 'WALT Scheduler (walt_target_load)'],
              ['vmSysctlWritable', 'VM Tunables (/proc/sys/vm)'],
              ['chargeControlNodePresent', 'Battery charge_control_limit'],
            ] as const
          ).map(([key, label]) => (
            <label
              key={key}
              className="flex items-center justify-between gap-2 text-xs text-slate-700 cursor-pointer py-0.5"
            >
              <span className="truncate">{label}</span>
              <input
                type="checkbox"
                checked={capabilities[key]}
                onChange={(e) =>
                  onChangeCapabilities({
                    ...capabilities,
                    [key]: e.target.checked,
                  })
                }
                className="w-4 h-4 rounded border-slate-300 text-blue-600 focus:ring-blue-600 shrink-0"
              />
            </label>
          ))}
        </div>
      </div>
    </aside>
  );
};
