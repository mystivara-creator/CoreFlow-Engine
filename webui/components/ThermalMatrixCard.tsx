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
  const cpuAvg = telemetry.cpuAverageCelsius ?? -1;
  const cpuPeak = telemetry.cpuPeakCelsius ?? -1;
  const gpu = telemetry.gpuTempCelsius ?? -1;
  const battery = telemetry.batteryTempCelsius ?? -1;
  const skin = telemetry.skinTempCelsius ?? -1;

  const isThrottling = composite >= thermalTripCelsius;
  const thermalProgress = Math.min(100, Math.max(0, ((composite - 30) / (60 - 30)) * 100));

  const sensors: ThermalSensorDetail[] = telemetry.thermalSensors ?? [];

  const filteredSensors = sensors.filter((s) => {
    if (filterCategory !== null && s.category !== filterCategory) return false;
    if (searchQuery && !s.type.toLowerCase().includes(searchQuery.toLowerCase())) return false;
    return true;
  });

  const getCategoryLabel = (cat: number) => {
    switch (cat) {
      case 1:
        return 'Compute · CPU/GPU';
      case 2:
        return 'Battery Cell';
      case 3:
        return 'Skin · Chassis';
      case 4:
        return 'Board · PMIC';
      default:
        return 'Peripheral';
    }
  };

  const formatZoneId = (rawId: string) => {
    const trimmed = rawId.trim();
    if (trimmed.startsWith('/sys/class/thermal/thermal_zone')) {
      return `tz${trimmed.slice('/sys/class/thermal/thermal_zone'.length)}`;
    }
    if (trimmed.startsWith('thermal_zone')) {
      return `tz${trimmed.slice('thermal_zone'.length)}`;
    }
    const parts = trimmed.split('/').filter(Boolean);
    const last = parts[parts.length - 1] ?? trimmed;
    return last.replace(/^thermal_zone/, 'tz');
  };

  return (
    <div className="m3-card p-5 sm:p-8 space-y-6">
      {/* Top Header */}
      <div className="flex flex-col sm:flex-row sm:items-start justify-between gap-4 pb-5 border-b border-slate-900/[0.06] dark:border-white/[0.06]">
        <div className="space-y-1.5 max-w-2xl">
          <div className="flex flex-wrap items-center gap-2 text-xs text-slate-500 dark:text-slate-400">
            <Flame className="w-4 h-4 text-[#0b57d0] dark:text-[#a8c7fa]" />
            <span className="font-semibold text-slate-900 dark:text-slate-100 text-base sm:text-lg">
              Multi-Sensor Thermal & OEM Policy Matrix
            </span>
            <span aria-hidden="true">·</span>
            <span className="font-medium text-[#0b57d0] dark:text-[#a8c7fa]">
              {isThrottling ? 'ThermalGuard Active Mitigation' : 'Thermal Headroom Nominal'}
            </span>
            <span aria-hidden="true">·</span>
            <span>{nativeMode ? 'Hardware sysfs Live' : 'Preview Model'}</span>
          </div>
          <p className="text-xs sm:text-sm text-slate-600 dark:text-slate-400 leading-relaxed">
            Prioritizes validated Android thermal status and per-zone OEM trip points over single-sensor heuristics. Separate compute, battery, and skin domains are evaluated without conflating distinct physical units.
          </p>
        </div>

        <div className="m3-tonal-block px-4 py-2.5 text-xs font-mono shrink-0 self-start">
          <span className="text-slate-500 dark:text-slate-400">Policy Reference: </span>
          <span className="font-semibold text-slate-900 dark:text-slate-100">
            {nativeMode ? 'OEM / Android HAL' : `${thermalTripCelsius.toFixed(1)} °C`}
          </span>
        </div>
      </div>

      {/* Primary Composite SoC Bar */}
      <div className="m3-tonal-block p-5">
        <div className="flex items-center justify-between text-xs text-slate-600 dark:text-slate-300 mb-3">
          <span className="flex items-center gap-2 font-medium">
            <Thermometer className="w-4 h-4 text-[#0b57d0] dark:text-[#a8c7fa] shrink-0" />
            <span>Weighted SoC Composite Diagnostic</span>
          </span>
          <span className="font-mono tabular-nums font-semibold text-slate-900 dark:text-slate-100 text-sm shrink-0">
            {composite > 0 ? composite.toFixed(1) : '--.-'} °C
          </span>
        </div>
        <div className="w-full h-2.5 rounded-full bg-slate-900/[0.08] dark:bg-white/[0.08] overflow-hidden">
          <div
            className="h-full transition-all duration-500 rounded-full bg-[#0b57d0] dark:bg-[#a8c7fa]"
            style={{ width: `${thermalProgress}%` }}
          />
        </div>
        <div className="flex justify-between text-[11px] text-slate-500 dark:text-slate-400 mt-2 font-mono tabular-nums">
          <span>30.0 °C (Idle)</span>
          <span>{thermalTripCelsius.toFixed(1)} °C (Mitigation Gate)</span>
          <span>60.0 °C (OEM Ceiling)</span>
        </div>
      </div>

      {/* Cluster & Subsystem Cards Grid */}
      <div className="grid grid-cols-2 lg:grid-cols-5 gap-3.5">
        {[
          { label: 'CPU Cluster Avg', val: cpuAvg, sub: 'Little & Big Cores', Icon: Cpu },
          { label: 'CPU Peak Core', val: cpuPeak, sub: 'Hottest Core Zone', Icon: Zap },
          { label: 'GPU Domain', val: gpu, sub: 'Shader / Devfreq', Icon: Layers },
          { label: 'Battery Cell', val: battery, sub: 'Charge Guard Input', Icon: ShieldCheck },
          { label: 'Skin / Surface', val: skin, sub: 'Chassis Thermistor', Icon: Thermometer },
        ].map(({ label, val, sub, Icon }) => (
          <div
            key={label}
            className="m3-tonal-block p-4 flex flex-col justify-between col-span-1 last:col-span-2 lg:last:col-span-1 min-w-0"
          >
            <div className="flex items-center justify-between gap-2 text-xs text-slate-500 dark:text-slate-400 mb-2">
              <span className="font-medium truncate">{label}</span>
              <Icon className="w-3.5 h-3.5 text-[#0b57d0] dark:text-[#a8c7fa] shrink-0" />
            </div>
            <div className="text-xl font-semibold font-mono tabular-nums text-slate-900 dark:text-slate-100">
              {val > 0 ? val.toFixed(1) : '--.-'}{' '}
              <span className="text-xs font-normal text-slate-500 dark:text-slate-400">°C</span>
            </div>
            <div className="text-[11px] text-slate-500 dark:text-slate-400 mt-1.5 truncate">{sub}</div>
          </div>
        ))}
      </div>

      {/* Collapsible Full Hardware Thermal Zone Inspector */}
      {sensors.length > 0 && (
        <details className="group m3-tonal-block overflow-hidden">
          <summary className="flex items-center justify-between gap-3 px-5 py-4 cursor-pointer select-none text-xs font-semibold text-slate-800 dark:text-slate-200 hover:bg-slate-900/[0.03] dark:hover:bg-white/[0.03]">
            <span className="flex items-center gap-2 min-w-0">
              <Filter className="w-3.5 h-3.5 text-[#0b57d0] dark:text-[#a8c7fa] shrink-0" />
              <span className="truncate">Discovered Hardware Thermal Zones ({sensors.length} Zones)</span>
            </span>
            <span className="text-xs font-normal text-slate-400 group-open:rotate-180 transition-transform duration-200 shrink-0">
              ▼
            </span>
          </summary>

          <div className="p-4 sm:p-5 border-t border-slate-900/[0.06] dark:border-white/[0.06] space-y-4">
            <div className="flex flex-col sm:flex-row sm:items-center justify-between gap-3">
              <div className="flex gap-1.5 flex-wrap">
                {[
                  { id: null, label: 'All Zones' },
                  { id: 1, label: 'Compute' },
                  { id: 2, label: 'Battery' },
                  { id: 3, label: 'Skin' },
                  { id: 4, label: 'Board' },
                ].map((cat) => {
                  const active = filterCategory === cat.id;
                  return (
                    <button
                      key={String(cat.id)}
                      onClick={() => setFilterCategory(cat.id)}
                      className={
                        'px-3.5 py-1.5 text-xs rounded-full font-medium transition-all ' +
                        (active
                          ? 'bg-[#d3e3fd] text-[#041e49] dark:bg-[#0842a0] dark:text-[#d3e3fd] font-semibold'
                          : 'bg-slate-900/[0.04] dark:bg-white/[0.05] text-slate-600 dark:text-slate-300 hover:bg-slate-900/[0.08]')
                      }
                    >
                      {cat.label}
                    </button>
                  );
                })}
              </div>

              <div className="relative w-full sm:w-56">
                <Search className="w-3.5 h-3.5 absolute left-3.5 top-1/2 -translate-y-1/2 text-slate-400" />
                <input
                  type="text"
                  placeholder="Search sensor type..."
                  value={searchQuery}
                  onChange={(e) => setSearchQuery(e.target.value)}
                  className="w-full pl-9 pr-4 py-1.5 text-xs rounded-full bg-white/70 dark:bg-slate-900/60 border border-slate-900/[0.08] dark:border-white/[0.08] text-slate-800 dark:text-slate-200 focus:outline-none focus:border-[#0b57d0]"
                />
              </div>
            </div>

            <div className="max-h-72 overflow-y-auto rounded-2xl border border-slate-900/[0.06] dark:border-white/[0.06] divide-y divide-slate-900/[0.05] dark:divide-white/[0.05]">
              {filteredSensors.length === 0 ? (
                <div className="py-8 text-center text-xs text-slate-500">
                  No matching hardware thermal zones found.
                </div>
              ) : (
                filteredSensors.map((sensor) => (
                  <div
                    key={sensor.id + sensor.type}
                    title={`${sensor.id} (${sensor.type})`}
                    className="flex items-center justify-between gap-3 px-3.5 sm:px-4 py-2.5 text-xs hover:bg-slate-900/[0.02] dark:hover:bg-white/[0.02]"
                  >
                    <div className="flex items-center gap-2.5 min-w-0 flex-1">
                      <span className="font-mono text-[11px] px-2 py-0.5 rounded-lg bg-slate-900/[0.05] dark:bg-white/[0.07] text-slate-600 dark:text-slate-300 shrink-0 min-w-[44px] text-center">
                        {formatZoneId(sensor.id)}
                      </span>
                      <div className="min-w-0 flex-1">
                        <div className="font-medium text-slate-800 dark:text-slate-200 truncate">
                          {sensor.type}
                        </div>
                        <div className="text-[11px] text-slate-500 dark:text-slate-400 truncate sm:hidden">
                          {getCategoryLabel(sensor.category)}
                        </div>
                      </div>
                    </div>
                    <span className="text-slate-500 dark:text-slate-400 hidden sm:inline shrink-0 w-36 truncate">
                      {getCategoryLabel(sensor.category)}
                    </span>
                    <div className="font-mono tabular-nums font-semibold text-right text-slate-900 dark:text-slate-100 shrink-0 pl-2">
                      {sensor.celsius !== null && sensor.celsius >= 0 ? (
                        `${sensor.celsius.toFixed(1)} °C`
                      ) : (
                        <span className="text-slate-400 font-normal">Offline</span>
                      )}
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
