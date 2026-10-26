import React, { useState } from 'react';
import { ChevronDown, ChevronUp, Play, Radio, RefreshCw, RotateCcw, Square, StepForward } from 'lucide-react';
import { DaemonProbe } from '../bridge/daemonBridge';
import { HARDWARE_PROFILES, SCENARIO_PRESETS } from '../engine/presets';
import {
  AppClass,
  CapabilityOverrides,
  DevicePresetId,
  WorkloadInputs,
} from '../engine/types';

interface ControlSidebarProps {
  isNativeBridge: boolean;
  probe: DaemonProbe | null;
  daemonMode: string;
  daemonBusy: boolean;
  statusMessage: string | null;
  onDaemonAction: (action: 'start' | 'stop' | 'restart' | 'rediscover') => void;
  mirrorSafeMode: boolean;
  onToggleMirrorSafeMode: (val: boolean) => void;
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
  isNativeBridge,
  probe,
  daemonMode,
  daemonBusy,
  statusMessage,
  onDaemonAction,
  mirrorSafeMode,
  onToggleMirrorSafeMode,
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

  const [mobileExpanded, setMobileExpanded] = useState(false);
  const pidDisplay = probe?.pids?.length ? probe.pids.join(', ') : 'stopped';

  return (
    <aside className="w-full lg:w-80 shrink-0 bg-white border-b lg:border-b-0 lg:border-r border-slate-200">
      <div className="px-4 py-3 lg:hidden flex items-center justify-between gap-3">
        <div className="min-w-0">
          <div className="text-xs font-semibold text-slate-900 truncate">
            C++ Daemon Bridge · {isNativeBridge ? 'KernelSU Live' : 'C++ Mirror'}
          </div>
          <div className="text-xs font-mono text-slate-500 truncate tabular-nums">
            PID {pidDisplay} · mode={daemonMode}
          </div>
        </div>
        <button
          type="button"
          aria-expanded={mobileExpanded}
          onClick={() => setMobileExpanded((prev) => !prev)}
          className="inline-flex items-center gap-1.5 min-h-[44px] px-3 py-2 text-xs font-medium text-slate-700 bg-slate-100 hover:bg-slate-200 rounded-lg shrink-0 transition-colors focus-visible:outline-2 focus-visible:outline-offset-2 focus-visible:outline-blue-600"
        >
          <span>{mobileExpanded ? 'Hide Controls' : 'Controls'}</span>
          {mobileExpanded ? (
            <ChevronUp className="w-4 h-4" aria-hidden="true" />
          ) : (
            <ChevronDown className="w-4 h-4" aria-hidden="true" />
          )}
        </button>
      </div>

      <div className={`${mobileExpanded ? 'block border-t border-slate-100' : 'hidden'} lg:block p-5 space-y-6`}>
        {/* Native C++ coreflowd Control Section */}
        <div className="space-y-3">
          <div className="flex items-center justify-between gap-2">
            <h2 className="text-sm font-semibold text-slate-900">
              C++ Daemon Bridge
            </h2>
            <span className="text-xs font-mono text-slate-500">
              {isNativeBridge ? 'KernelSU Live' : 'C++ Mirror'}
            </span>
          </div>

          <div className="text-xs text-slate-600 font-mono tabular-nums space-y-1 bg-slate-50 border border-slate-200 rounded-lg p-3">
            <div className="flex justify-between">
              <span className="text-slate-500">coreflowd PID</span>
              <span className="font-semibold text-slate-900">{pidDisplay}</span>
            </div>
            <div className="flex justify-between">
              <span className="text-slate-500">Config Mode</span>
              <span className="font-semibold text-slate-900">{daemonMode}</span>
            </div>
            <div className="flex justify-between">
              <span className="text-slate-500">Markers</span>
              <span className="text-slate-800">
                {probe?.safe ? 'SAFE_MODE ' : ''}
                {probe?.disable ? 'DISABLE ' : ''}
                {probe?.pending ? 'PENDING ' : ''}
                {!probe?.safe && !probe?.disable && !probe?.pending ? 'CLEAN' : ''}
              </span>
            </div>
          </div>

          <div className="grid grid-cols-2 gap-1.5">
            <button
              id="btn-start-engine"
              type="button"
              disabled={daemonBusy}
              onClick={() => onDaemonAction('start')}
              className="inline-flex items-center justify-center gap-1.5 px-3 py-1.5 text-xs font-medium text-slate-700 bg-slate-100 rounded-md hover:bg-slate-200 disabled:opacity-50 transition-colors whitespace-nowrap"
            >
              <Play className="w-3.5 h-3.5" aria-hidden="true" />
              <span>Start</span>
            </button>
            <button
              id="btn-stop-engine"
              type="button"
              disabled={daemonBusy}
              onClick={() => onDaemonAction('stop')}
              className="inline-flex items-center justify-center gap-1.5 px-3 py-1.5 text-xs font-medium text-slate-700 bg-slate-100 rounded-md hover:bg-slate-200 disabled:opacity-50 transition-colors whitespace-nowrap"
            >
              <Square className="w-3.5 h-3.5" aria-hidden="true" />
              <span>Stop</span>
            </button>
            <button
              id="btn-restart-engine"
              type="button"
              disabled={daemonBusy}
              onClick={() => onDaemonAction('restart')}
              className="inline-flex items-center justify-center gap-1.5 px-3 py-1.5 text-xs font-medium text-slate-700 bg-slate-100 rounded-md hover:bg-slate-200 disabled:opacity-50 transition-colors whitespace-nowrap"
            >
              <RefreshCw className="w-3.5 h-3.5" aria-hidden="true" />
              <span>Restart</span>
            </button>
            <button
              id="btn-rediscover"
              type="button"
              disabled={daemonBusy}
              onClick={() => onDaemonAction('rediscover')}
              title="Send SIGUSR1 to coreflowd to re-run EnvironmentDiscovery without writing tunables"
              className="inline-flex items-center justify-center gap-1.5 px-3 py-1.5 text-xs font-medium text-blue-700 bg-blue-50 border border-blue-200 rounded-md hover:bg-blue-100 disabled:opacity-50 transition-colors whitespace-nowrap"
            >
              <Radio className="w-3.5 h-3.5" aria-hidden="true" />
              <span>SIGUSR1 Scan</span>
            </button>
          </div>

          {statusMessage && (
            <p className="text-xs text-blue-700 bg-blue-50/80 border border-blue-200 rounded-md px-2.5 py-1.5">
              {statusMessage}
            </p>
          )}
        </div>

        {/* Interactive Telemetry Controls (shown in Mirror / Preview mode) */}
        {!isNativeBridge && (
          <>
            <div className="pt-4 border-t border-slate-200 flex items-center justify-between gap-2">
              <h2 className="text-sm font-semibold text-slate-900">Hardware &amp; Sensors</h2>
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

            <div className="space-y-3 pt-4 border-t border-slate-200">
              <div className="flex items-center justify-between">
                <span className="text-xs font-medium text-slate-700">Display State</span>
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
                  Foreground Tracker Class
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
                  <option value={AppClass.Default}>APP_DEFAULT (Interactive)</option>
                  <option value={AppClass.Game}>APP_GAME (Sustained Render)</option>
                  <option value={AppClass.Launcher}>APP_LAUNCHER (Home Idle)</option>
                </select>
              </div>
            </div>

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
                    GPU Busy (gpu_busy_source)
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
              </div>

              <div className="pt-2 space-y-2">
                <label className="flex items-center justify-between text-xs font-medium text-slate-700 cursor-pointer">
                  <span>Power Supply Charging</span>
                  <input
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
                </label>

                <label className="flex items-center justify-between text-xs font-medium text-slate-700 cursor-pointer">
                  <span>Simulate SAFE_MODE Marker</span>
                  <input
                    type="checkbox"
                    checked={mirrorSafeMode}
                    onChange={(e) => onToggleMirrorSafeMode(e.target.checked)}
                    className="w-4 h-4 rounded border-slate-300 text-blue-600 focus:ring-blue-600"
                  />
                </label>
              </div>
            </div>

            <div className="space-y-2 pt-4 border-t border-slate-200">
              <div className="text-xs font-semibold text-slate-900">
                Exposed Kernel Interfaces
              </div>
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
          </>
        )}
      </div>
    </aside>
  );
};
