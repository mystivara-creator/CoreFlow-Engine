import React, { useState } from 'react';
import {
  Activity,
  Battery,
  ChevronDown,
  Cpu,
  Flame,
  Search,
  ShieldCheck,
  Smartphone,
  Thermometer,
} from 'lucide-react';
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

  const composite =
    telemetry.socCompositeCelsius && telemetry.socCompositeCelsius > 0
      ? telemetry.socCompositeCelsius
      : telemetry.socTempCelsius;

  const cpuAvg = telemetry.cpuAverageCelsius ?? -1;
  const cpuPeak = telemetry.cpuPeakCelsius ?? -1;
  const gpu = telemetry.gpuTempCelsius ?? -1;
  const battery = telemetry.batteryTempCelsius ?? -1;
  const skin = telemetry.skinTempCelsius ?? -1;

  const isThrottling = nativeMode
    ? Boolean(telemetry.thermalGuardActive)
    : composite >= thermalTripCelsius;
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
        return 'Diagnostic · Aux';
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
    <div className="md3-card-elevated p-6 sm:p-8 space-y-6">
      {/* Header */}
      <div
        className="flex flex-col sm:flex-row sm:items-start justify-between gap-4 pb-5"
        style={{ borderBottom: '1px solid var(--md-sys-color-outline-variant)' }}
      >
        <div className="space-y-1.5 max-w-2xl">
          <div className="flex flex-wrap items-center gap-2">
            <Flame className="w-4 h-4" style={{ color: 'var(--md-sys-color-primary)' }} />
            <h2 className="font-bold text-base sm:text-lg">
              Multi-Sensor Thermal &amp; OEM Policy Matrix{!nativeMode ? ' (sim)' : ''}
            </h2>
            <span
              className={
                isThrottling
                  ? 'md3-badge md3-badge-warning'
                  : 'md3-badge md3-badge-success'
              }
            >
              {isThrottling ? 'ThermalGuard Active Mitigation' : 'Thermal Headroom Nominal'}
            </span>
            {telemetry.thermalDataDegraded && (
              <span className="md3-badge md3-badge-warning">
                Sensor Degraded (Partial Read)
              </span>
            )}
            {telemetry.thermalFallbackActive && (
              <span className="md3-badge md3-badge-warning">
                Generic Fallback Mode
              </span>
            )}
          </div>
          <p
            className="text-xs sm:text-sm leading-relaxed"
            style={{ color: 'var(--md-sys-color-on-surface-variant)' }}
          >
            Prioritizes validated Android NDK thermal severity and per-zone OEM trip points over single-sensor heuristics. Compute, battery, skin, and PMIC/board domains are evaluated independently with physical velocity tracking.
          </p>
        </div>

        <div className="md3-card-filled px-4 py-2.5 text-xs font-mono shrink-0 self-start">
          <span style={{ color: 'var(--md-sys-color-on-surface-variant)' }}>Policy Reference: </span>
          <span className="font-bold">
            {nativeMode
              ? telemetry.thermalFallbackActive
                ? `Fallback (${thermalTripCelsius.toFixed(1)} °C)`
                : `OEM / Android (${telemetry.thermalSource ?? 'HAL'})`
              : `${thermalTripCelsius.toFixed(1)} °C`}
          </span>
        </div>
      </div>

      {/* Primary Composite SoC Bar */}
      <div className="md3-card-filled p-5 space-y-3">
        <div className="flex items-center justify-between text-xs">
          <span className="flex items-center gap-2 font-bold">
            <Thermometer className="w-4 h-4 shrink-0" style={{ color: 'var(--md-sys-color-primary)' }} />
            <span>Weighted SoC Composite Diagnostic{!nativeMode ? ' (sim)' : ''}</span>
          </span>
          <span className="font-mono tabular-nums font-bold text-base shrink-0">
            {composite > 0 ? composite.toFixed(1) : '--.-'} °C
          </span>
        </div>

        <div className="md3-progress-track">
          <div
            className="md3-progress-bar"
            style={{ width: `${thermalProgress}%` }}
          />
        </div>

        <div
          className="flex justify-between text-[11px] font-mono tabular-nums"
          style={{ color: 'var(--md-sys-color-on-surface-variant)' }}
        >
          <span>30.0 °C (Idle)</span>
          <span>{thermalTripCelsius.toFixed(1)} °C (Mitigation Gate)</span>
          <span>60.0 °C (OEM Ceiling)</span>
        </div>
      </div>

      {/* 5-Domain Subsystem Grid */}
      <div className="grid grid-cols-2 sm:grid-cols-3 lg:grid-cols-5 gap-3">
        {[
          { label: 'CPU Cluster Avg', val: cpuAvg, Icon: Cpu, sub: 'All compute cores' },
          { label: 'CPU Peak Core', val: cpuPeak, Icon: Flame, sub: 'Hottest core die' },
          { label: 'GPU Domain', val: gpu, Icon: Activity, sub: 'Graphics subsystem' },
          { label: 'Battery Cell', val: battery, Icon: Battery, sub: 'Charging protection' },
          { label: 'Skin / Chassis', val: skin, Icon: Smartphone, sub: 'Surface comfort' },
        ].map(({ label, val, Icon, sub }) => (
          <div key={label} className="md3-card-filled p-4 flex flex-col justify-between">
            <div className="flex items-center justify-between text-xs mb-2">
              <span className="font-semibold truncate">{label}</span>
              <Icon className="w-3.5 h-3.5 shrink-0" style={{ color: 'var(--md-sys-color-primary)' }} />
            </div>
            <div className="text-xl font-bold font-mono tabular-nums">
              {val > 0 ? val.toFixed(1) : '--.-'}{' '}
              <span
                className="text-xs font-normal"
                style={{ color: 'var(--md-sys-color-on-surface-variant)' }}
              >
                °C
              </span>
            </div>
            <div
              className="text-[11px] mt-1.5"
              style={{ color: 'var(--md-sys-color-on-surface-variant)' }}
            >
              {sub}
            </div>
          </div>
        ))}
      </div>

      {/* Expandable Hardware Thermal Zone Matrix */}
      {sensors.length > 0 && (
        <details className="group md3-card-filled overflow-hidden" open>
          <summary className="cursor-pointer p-4 sm:p-5 flex items-center justify-between text-xs sm:text-sm font-bold select-none">
            <div className="flex items-center gap-2.5 min-w-0">
              <ShieldCheck
                className="w-4 h-4 shrink-0"
                style={{ color: 'var(--md-sys-color-primary)' }}
              />
              <span className="truncate">
                Discovered Hardware Thermal Zones{!nativeMode ? ' (sim)' : ''} ({sensors.length} Zones)
              </span>
            </div>
            <ChevronDown className="w-4 h-4 shrink-0 transition-transform duration-200 group-open:rotate-180" />
          </summary>

          <div
            className="p-4 sm:p-5 pt-0 space-y-4"
            style={{ borderTop: '1px solid var(--md-sys-color-outline-variant)' }}
          >
            {/* Filter & Search Bar */}
            <div className="flex flex-col sm:flex-row sm:items-center justify-between gap-3 pt-4">
              <div className="flex flex-wrap gap-1.5">
                {[
                  { id: null, label: 'All Domains' },
                  { id: 1, label: 'Compute' },
                  { id: 2, label: 'Battery' },
                  { id: 3, label: 'Skin' },
                  { id: 4, label: 'Board/PMIC' },
                ].map((cat) => {
                  const active = filterCategory === cat.id;
                  return (
                    <button
                      key={String(cat.id)}
                      type="button"
                      aria-pressed={active}
                      onClick={() => setFilterCategory(cat.id)}
                      className={'md3-chip ' + (active ? 'is-selected' : '')}
                    >
                      {cat.label}
                    </button>
                  );
                })}
              </div>

              <div className="relative">
                <Search
                  className="w-3.5 h-3.5 absolute left-3.5 top-1/2 -translate-y-1/2 pointer-events-none"
                  style={{ color: 'var(--md-sys-color-on-surface-variant)' }}
                />
                <input
                  type="text"
                  aria-label="Search thermal zone"
                  placeholder="Search zone..."
                  value={searchQuery}
                  onChange={(e) => setSearchQuery(e.target.value)}
                  className="w-full sm:w-52 pl-9 pr-3.5 py-1.5 text-xs rounded-full focus:outline-none"
                  style={{
                    backgroundColor: 'var(--md-sys-color-surface-container-lowest)',
                    border: '1px solid var(--md-sys-color-outline-variant)',
                    color: 'var(--md-sys-color-on-surface)',
                  }}
                />
              </div>
            </div>

            {/* Zone Cards */}
            <div className="grid grid-cols-1 sm:grid-cols-2 lg:grid-cols-4 gap-2.5 max-h-80 overflow-y-auto pr-1">
              {filteredSensors.length === 0 ? (
                <div
                  className="col-span-full text-center py-6 text-xs"
                  style={{ color: 'var(--md-sys-color-on-surface-variant)' }}
                >
                  No thermal sensors match your filter.
                </div>
              ) : (
                filteredSensors.map((sensor) => (
                  <div
                    key={sensor.id}
                    className="md3-card-elevated p-3.5 flex items-center justify-between gap-2"
                  >
                    <div className="min-w-0 pr-2">
                      <div className="text-xs font-mono font-bold truncate" title={sensor.type}>
                        {sensor.type}
                      </div>
                      <div
                        className="text-[11px] truncate"
                        style={{ color: 'var(--md-sys-color-on-surface-variant)' }}
                        title={sensor.id}
                      >
                        {formatZoneId(sensor.id)} · {getCategoryLabel(sensor.category)}
                      </div>
                    </div>
                    <div className="text-right shrink-0">
                      <div className="text-xs font-mono tabular-nums font-bold">
                        {sensor.valid && sensor.celsius !== null ? (
                          `${sensor.celsius.toFixed(1)} °C`
                        ) : (
                          <span style={{ color: 'var(--md-sys-color-on-surface-variant)' }}>N/A</span>
                        )}
                      </div>
                      <div
                        className="text-[10px] font-mono tabular-nums"
                        style={{ color: 'var(--md-sys-color-on-surface-variant)' }}
                      >
                        {sensor.velocityValid &&
                        sensor.velocityCelsiusPerSec !== null &&
                        sensor.velocityCelsiusPerSec !== undefined
                          ? `${sensor.velocityCelsiusPerSec > 0 ? '+' : ''}${sensor.velocityCelsiusPerSec.toFixed(2)} °C/s`
                          : sensor.policyAvailable
                            ? 'OEM trip'
                            : 'no-trip'}
                      </div>
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
