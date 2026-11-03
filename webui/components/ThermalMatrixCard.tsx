import React, { useState } from 'react';
import { Cpu, Flame, Layers, ShieldCheck, Thermometer, Zap, Filter, Search } from 'lucide-react';
import { AutonomousTelemetry, ThermalSensorDetail } from '../engine/autonomousEngine';

interface Props {
  telemetry: AutonomousTelemetry;
  nativeMode: boolean;
  thermalTripCelsius: number;
}

export const ThermalMatrixCard: React.FC<Props> = ({
  telemetry,
  nativeMode,
  thermalTripCelsius,
}) => {
  const [filterCategory, setFilterCategory] = useState<number | null>(null);
  const [searchQuery, setSearchQuery] = useState('');

  const composite = telemetry.socCompositeCelsius ?? telemetry.socTempCelsius;
  const cpuAvg = telemetry.cpuAverageCelsius ?? (composite > 0 ? composite - 0.4 : -1);
  const cpuPeak = telemetry.cpuPeakCelsius ?? (composite > 0 ? composite + 2.5 : -1);
  const gpu = telemetry.gpuTempCelsius ?? (composite > 0 ? composite - 1.8 : -1);
  const battery = telemetry.batteryTempCelsius ?? (composite > 0 ? composite - 5.0 : -1);
  const skin = telemetry.skinTempCelsius ?? (composite > 0 ? composite - 3.2 : -1);

  const isThrottling = composite >= thermalTripCelsius;
  const thermalProgress = Math.min(100, Math.max(0, ((composite - 30) / (60 - 30)) * 100));

  const sensors: ThermalSensorDetail[] = telemetry.thermalSensors ?? [];

  const filteredSensors = sensors.filter((s) => {
    if (filterCategory !== null && s.category !== filterCategory) return false;
    if (searchQuery && !s.type.toLowerCase().includes(searchQuery.toLowerCase())) return false;
    return true;
  });

  const getCategoryBadge = (cat: number) => {
    switch (cat) {
      case 1:
        return <span className="px-2 py-0.5 rounded text-[10px] font-medium bg-sky-500/10 text-sky-500 border border-sky-500/20">Compute / CPU-GPU</span>;
      case 2:
        return <span className="px-2 py-0.5 rounded text-[10px] font-medium bg-emerald-500/10 text-emerald-500 border border-emerald-500/20">Battery Cell</span>;
      case 3:
        return <span className="px-2 py-0.5 rounded text-[10px] font-medium bg-purple-500/10 text-purple-500 border border-purple-500/20">Skin / Chassis</span>;
      case 4:
        return <span className="px-2 py-0.5 rounded text-[10px] font-medium bg-amber-500/10 text-amber-500 border border-amber-500/20">Board / PMIC</span>;
      default:
        return <span className="px-2 py-0.5 rounded text-[10px] font-medium bg-slate-500/10 text-slate-400 border border-slate-500/20">Peripheral</span>;
    }
  };

  return (
    <div className="rounded-3xl bg-white dark:bg-[#111827] border border-slate-200 dark:border-slate-800 p-5 sm:p-7 shadow-sm space-y-6">
      {/* Top Header */}
      <div className="flex flex-col sm:flex-row sm:items-center justify-between gap-3">
        <div>
          <div className="flex items-center gap-2 mb-1">
            <Flame className="w-4 h-4 text-amber-500" />
            <h3 className="text-base sm:text-lg font-bold text-slate-900 dark:text-white">
              Multi-Sensor Thermal Matrix
            </h3>
            {isThrottling ? (
              <span className="px-2.5 py-0.5 rounded-full text-xs font-semibold bg-rose-500/15 text-rose-500 border border-rose-500/30 animate-pulse">
                ThermalGuard Cooling
              </span>
            ) : (
              <span className="px-2.5 py-0.5 rounded-full text-xs font-semibold bg-emerald-500/15 text-emerald-500 border border-emerald-500/30">
                Thermal Equilibrium Safe
              </span>
            )}
            <span className="px-2 py-0.5 rounded-full text-[10px] font-semibold bg-slate-100 dark:bg-slate-800 text-slate-600 dark:text-slate-400 border border-slate-300 dark:border-slate-700">
              {nativeMode ? 'Hardware sysfs Active' : 'Simulation Model'}
            </span>
          </div>
          <p className="text-xs text-slate-500 dark:text-slate-400">
            Weighted composite fusion across CPU clusters, GPU shaders, battery cell, and chassis thermistors.
          </p>
        </div>

        <div className="flex items-center gap-2 text-xs font-mono px-3 py-1.5 rounded-xl bg-slate-100 dark:bg-slate-800/80 border border-slate-200 dark:border-slate-700">
          <span className="text-slate-500">Trip Point:</span>
          <span className="font-semibold text-amber-500">{thermalTripCelsius.toFixed(1)}°C</span>
        </div>
      </div>

      {/* Primary Composite SoC Bar */}
      <div className="p-4 rounded-2xl bg-slate-50 dark:bg-slate-900/80 border border-slate-200/80 dark:border-slate-800">
        <div className="flex items-center justify-between text-xs text-slate-600 dark:text-slate-400 mb-2">
          <span className="flex items-center gap-1.5 font-medium">
            <Thermometer className="w-3.5 h-3.5 text-amber-500" />
            Weighted SoC Composite Temperature
          </span>
          <span className="font-mono font-bold text-slate-900 dark:text-white text-sm">
            {composite > 0 ? composite.toFixed(1) : '--.-'} °C
          </span>
        </div>
        <div className="w-full h-3 rounded-full bg-slate-200 dark:bg-slate-800 overflow-hidden relative">
          <div
            className={`h-full transition-all duration-500 rounded-full ${
              isThrottling
                ? 'bg-gradient-to-r from-amber-500 to-rose-500'
                : 'bg-gradient-to-r from-teal-500 via-sky-500 to-amber-500'
            }`}
            style={{ width: `${thermalProgress}%` }}
          />
        </div>
        <div className="flex justify-between text-[10px] text-slate-400 mt-1 font-mono">
          <span>30°C (Idle)</span>
          <span>44°C (Trip Threshold)</span>
          <span>60°C (Hard Limit)</span>
        </div>
      </div>

      {/* Cluster & Subsystem Cards Grid */}
      <div className="grid grid-cols-2 lg:grid-cols-5 gap-3">
        <div className="p-3.5 rounded-2xl bg-slate-50/80 dark:bg-slate-900/50 border border-slate-200/70 dark:border-slate-800">
          <div className="flex items-center justify-between text-[11px] text-slate-500 mb-1">
            <span>CPU Cluster Avg</span>
            <Cpu className="w-3.5 h-3.5 text-sky-500" />
          </div>
          <div className="text-xl font-bold font-mono text-sky-500">
            {cpuAvg > 0 ? cpuAvg.toFixed(1) : '--.-'} <span className="text-xs font-normal text-slate-400">°C</span>
          </div>
          <div className="text-[10px] text-slate-400 mt-1">Little & Big Cores</div>
        </div>

        <div className="p-3.5 rounded-2xl bg-slate-50/80 dark:bg-slate-900/50 border border-slate-200/70 dark:border-slate-800">
          <div className="flex items-center justify-between text-[11px] text-slate-500 mb-1">
            <span>CPU Peak Core</span>
            <Zap className="w-3.5 h-3.5 text-amber-500" />
          </div>
          <div className="text-xl font-bold font-mono text-amber-500">
            {cpuPeak > 0 ? cpuPeak.toFixed(1) : '--.-'} <span className="text-xs font-normal text-slate-400">°C</span>
          </div>
          <div className="text-[10px] text-slate-400 mt-1">Hottest Single Core</div>
        </div>

        <div className="p-3.5 rounded-2xl bg-slate-50/80 dark:bg-slate-900/50 border border-slate-200/70 dark:border-slate-800">
          <div className="flex items-center justify-between text-[11px] text-slate-500 mb-1">
            <span>GPU Domain</span>
            <Layers className="w-3.5 h-3.5 text-teal-500" />
          </div>
          <div className="text-xl font-bold font-mono text-teal-500">
            {gpu > 0 ? gpu.toFixed(1) : '--.-'} <span className="text-xs font-normal text-slate-400">°C</span>
          </div>
          <div className="text-[10px] text-slate-400 mt-1">Adreno / Mali GPU</div>
        </div>

        <div className="p-3.5 rounded-2xl bg-slate-50/80 dark:bg-slate-900/50 border border-slate-200/70 dark:border-slate-800">
          <div className="flex items-center justify-between text-[11px] text-slate-500 mb-1">
            <span>Battery Cell</span>
            <ShieldCheck className="w-3.5 h-3.5 text-emerald-500" />
          </div>
          <div className="text-xl font-bold font-mono text-emerald-500">
            {battery > 0 ? battery.toFixed(1) : '--.-'} <span className="text-xs font-normal text-slate-400">°C</span>
          </div>
          <div className="text-[10px] text-slate-400 mt-1">Li-ion Health Guard</div>
        </div>

        <div className="p-3.5 rounded-2xl bg-slate-50/80 dark:bg-slate-900/50 border border-slate-200/70 dark:border-slate-800 col-span-2 lg:col-span-1">
          <div className="flex items-center justify-between text-[11px] text-slate-500 mb-1">
            <span>Skin / Quiet</span>
            <Thermometer className="w-3.5 h-3.5 text-purple-500" />
          </div>
          <div className="text-xl font-bold font-mono text-purple-500">
            {skin > 0 ? skin.toFixed(1) : '--.-'} <span className="text-xs font-normal text-slate-400">°C</span>
          </div>
          <div className="text-[10px] text-slate-400 mt-1">Chassis / Surface Therm</div>
        </div>
      </div>

      {/* Collapsible Full Hardware Thermal Zone Inspector */}
      {sensors.length > 0 && (
        <details className="group border border-slate-200/80 dark:border-slate-800 rounded-2xl overflow-hidden bg-slate-50/50 dark:bg-slate-900/30">
          <summary className="flex items-center justify-between px-4 py-3 cursor-pointer select-none text-xs font-semibold text-slate-700 dark:text-slate-300 hover:bg-slate-100/60 dark:hover:bg-slate-800/40">
            <span className="flex items-center gap-2">
              <Filter className="w-3.5 h-3.5 text-sky-500" />
              Hardware Thermal Zones Inspector ({sensors.length} Detected)
            </span>
            <span className="text-[11px] font-normal text-slate-400 group-open:rotate-180 transition-transform duration-200">
              ▼
            </span>
          </summary>

          <div className="p-4 border-t border-slate-200/80 dark:border-slate-800 space-y-3">
            <div className="flex flex-wrap items-center gap-2">
              <div className="relative flex-1 min-w-[200px]">
                <Search className="w-3.5 h-3.5 absolute left-3 top-1/2 -translate-y-1/2 text-slate-400" />
                <input
                  type="text"
                  placeholder="Filter sensor name (e.g. cpu, gpu, quiet)..."
                  value={searchQuery}
                  onChange={(e) => setSearchQuery(e.target.value)}
                  className="w-full pl-8 pr-3 py-1.5 text-xs rounded-xl bg-white dark:bg-slate-800 border border-slate-200 dark:border-slate-700 text-slate-800 dark:text-slate-200 focus:outline-none focus:border-sky-500"
                />
              </div>

              <div className="flex gap-1.5 flex-wrap">
                <button
                  onClick={() => setFilterCategory(null)}
                  className={`px-2.5 py-1 text-[11px] rounded-lg border font-medium ${
                    filterCategory === null
                      ? 'bg-sky-500 text-white border-sky-600'
                      : 'bg-white dark:bg-slate-800 border-slate-200 dark:border-slate-700 text-slate-600 dark:text-slate-300'
                  }`}
                >
                  All
                </button>
                <button
                  onClick={() => setFilterCategory(1)}
                  className={`px-2.5 py-1 text-[11px] rounded-lg border font-medium ${
                    filterCategory === 1
                      ? 'bg-sky-500 text-white border-sky-600'
                      : 'bg-white dark:bg-slate-800 border-slate-200 dark:border-slate-700 text-slate-600 dark:text-slate-300'
                  }`}
                >
                  Compute
                </button>
                <button
                  onClick={() => setFilterCategory(2)}
                  className={`px-2.5 py-1 text-[11px] rounded-lg border font-medium ${
                    filterCategory === 2
                      ? 'bg-emerald-500 text-white border-emerald-600'
                      : 'bg-white dark:bg-slate-800 border-slate-200 dark:border-slate-700 text-slate-600 dark:text-slate-300'
                  }`}
                >
                  Battery
                </button>
                <button
                  onClick={() => setFilterCategory(3)}
                  className={`px-2.5 py-1 text-[11px] rounded-lg border font-medium ${
                    filterCategory === 3
                      ? 'bg-purple-500 text-white border-purple-600'
                      : 'bg-white dark:bg-slate-800 border-slate-200 dark:border-slate-700 text-slate-600 dark:text-slate-300'
                  }`}
                >
                  Skin
                </button>
                <button
                  onClick={() => setFilterCategory(4)}
                  className={`px-2.5 py-1 text-[11px] rounded-lg border font-medium ${
                    filterCategory === 4
                      ? 'bg-amber-500 text-white border-amber-600'
                      : 'bg-white dark:bg-slate-800 border-slate-200 dark:border-slate-700 text-slate-600 dark:text-slate-300'
                  }`}
                >
                  Board
                </button>
              </div>
            </div>

            <div className="max-h-64 overflow-y-auto rounded-xl border border-slate-200/70 dark:border-slate-800 divide-y divide-slate-200/60 dark:divide-slate-800">
              {filteredSensors.length === 0 ? (
                <div className="py-6 text-center text-xs text-slate-400">
                  No matching hardware sensors found
                </div>
              ) : (
                filteredSensors.map((sensor) => (
                  <div
                    key={sensor.id + sensor.type}
                    className="flex items-center justify-between px-3.5 py-2 text-xs hover:bg-slate-100/50 dark:hover:bg-slate-800/50"
                  >
                    <div className="flex items-center gap-2">
                      <span className="font-mono text-[11px] text-slate-400">{sensor.id}</span>
                      <span className="font-medium text-slate-800 dark:text-slate-200">{sensor.type}</span>
                      {getCategoryBadge(sensor.category)}
                    </div>
                    <div className="font-mono font-bold text-slate-800 dark:text-slate-200">
                      {sensor.celsius !== null && sensor.celsius >= 0
                        ? `${sensor.celsius.toFixed(1)} °C`
                        : <span className="text-slate-400 font-normal">Offline</span>}
                    </div>
                  </div>
                ))
              )}
            </div>
          </div>
        </details>
      )}
    </div>
  );
};
