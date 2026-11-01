import React, { useState } from 'react';
import {
  AlertTriangle,
  ArrowDownRight,
  ArrowRight,
  ArrowUpRight,
  CheckCircle2,
  ChevronDown,
  Cpu,
  Layers,
  ShieldAlert,
} from 'lucide-react';
import type { TelemetryRecord } from '../bridge/diagnostics';
import { ParsedEngineStatus, PipelineStage } from '../bridge/daemonBridge';
import {
  CapabilityOverrides,
  Decision,
  DeviceHardwareProfile,
  EngineMode,
  HybridBrainEvaluation,
  NotificationEvent,
  RuntimeSample,
  RuntimeState,
  SysfsNodeEntry,
  Trend,
} from '../engine/types';

interface RuntimeConsoleViewProps {
  isNativeBridge?: boolean;
  nativeHistory?: TelemetryRecord[];
  profile: DeviceHardwareProfile;
  capabilities: CapabilityOverrides;
  currentSample: RuntimeSample;
  history: RuntimeSample[];
  policyState: RuntimeState;
  decision: Decision;
  pillarMode: EngineMode;
  hybridBrain?: HybridBrainEvaluation;
  lastNotification: NotificationEvent;
  effectiveSleepSec: number;
  sysfsNodes: SysfsNodeEntry[];
  parsedStatus: ParsedEngineStatus | null;
  statusSummary: { code: string; tone: string; title: string; detail: string } | null;
  pipelineStages: PipelineStage[];
  onOpenSysfs: () => void;
  onOpenPolicy: () => void;
}

function getStateSemantic(state: string) {
  switch (state) {
    case RuntimeState.ThermalGuard:
    case RuntimeState.Pressure:
      return {
        colorClass: 'text-red-700',
        badgeClass: 'bg-red-50 text-red-800 border-red-200',
        barColor: '#DC2626',
        Icon: ShieldAlert,
        note:
          state === RuntimeState.ThermalGuard
            ? 'Hysteresis lock active until <= 52.0 °C'
            : 'Hysteresis lock active until >= 15.0% RAM',
      };
    case RuntimeState.Warming:
    case RuntimeState.Elevated:
      return {
        colorClass: 'text-amber-700',
        badgeClass: 'bg-amber-50 text-amber-800 border-amber-200',
        barColor: '#D97706',
        Icon: AlertTriangle,
        note:
          state === RuntimeState.Warming
            ? 'Monitoring 48–55 °C warm transition band'
            : 'Elevated runnable queue load (>= 1.50)',
      };
    case RuntimeState.Idle:
    case RuntimeState.Normal:
    default:
      return {
        colorClass: 'text-emerald-700',
        badgeClass: 'bg-emerald-50 text-emerald-800 border-emerald-200',
        barColor: '#059669',
        Icon: CheckCircle2,
        note:
          state === RuntimeState.Idle
            ? 'Deep-sleep cadence scaling active'
            : 'Nominal operating envelope (< 48 °C)',
      };
  }
}

function renderTrendInline(trend: string) {
  if (trend === Trend.Rising) {
    return (
      <span className="inline-flex items-center gap-0.5 text-amber-700 font-mono font-medium">
        <ArrowUpRight className="w-3.5 h-3.5" aria-hidden="true" />
        RISING
      </span>
    );
  }
  if (trend === Trend.Falling) {
    return (
      <span className="inline-flex items-center gap-0.5 text-blue-700 font-mono font-medium">
        <ArrowDownRight className="w-3.5 h-3.5" aria-hidden="true" />
        FALLING
      </span>
    );
  }
  if (trend === Trend.Stable) {
    return (
      <span className="inline-flex items-center gap-0.5 text-emerald-700 font-mono font-medium">
        <ArrowRight className="w-3.5 h-3.5" aria-hidden="true" />
        STABLE
      </span>
    );
  }
  return <span className="text-slate-500 font-mono">UNKNOWN</span>;
}

function MiniSvgMeter({
  ratio,
  color,
}: {
  ratio: number;
  color: string;
}) {
  const clamped = Math.min(1, Math.max(0, Number.isFinite(ratio) ? ratio : 0));
  return (
    <svg
      viewBox="0 0 100 4"
      preserveAspectRatio="none"
      className="w-full h-1.5 rounded-full overflow-hidden mt-3"
      aria-hidden="true"
    >
      <rect x={0} y={0} width={100} height={4} rx={2} fill="#E2E8F0" opacity={0.65} />
      <rect
        x={0}
        y={0}
        width={Number((clamped * 100).toFixed(1))}
        height={4}
        rx={2}
        fill={color}
        className="cf-svg-metric"
      />
    </svg>
  );
}

export const RuntimeConsoleView: React.FC<RuntimeConsoleViewProps> = ({
  isNativeBridge = false,
  nativeHistory = [],
  profile,
  capabilities,
  currentSample,
  history,
  policyState,
  decision,
  pillarMode,
  hybridBrain,
  lastNotification,
  effectiveSleepSec,
  sysfsNodes,
  parsedStatus,
  statusSummary,
  pipelineStages,
  onOpenSysfs,
  onOpenPolicy,
}) => {
  const [selectedTickIdx, setSelectedTickIdx] = useState<number | null>(null);
  const d = parsedStatus?.data;
  const measured = nativeHistory.slice(-60);
  const temperatures = measured
    .map((row) => row['Thermal(C)'])
    .filter((v): v is number => typeof v === 'number' && Number.isFinite(v));
  const minTemperature = temperatures.length ? Math.floor(Math.min(...temperatures) - 2) : 0;
  const maxTemperature = temperatures.length ? Math.ceil(Math.max(...temperatures) + 2) : 1;

  const measuredPath = (key: string, min: number, max: number) => {
    let connected = false;
    return measured
      .map((row, index) => {
        const value = row[key];
        if (typeof value !== 'number' || !Number.isFinite(value)) {
          connected = false;
          return '';
        }
        const x = measured.length === 1 ? 340 : 40 + (index * 600) / (measured.length - 1);
        const y = 170 - Math.min(1, Math.max(0, (value - min) / (max - min))) * 150;
        const segment = `${connected ? 'L' : 'M'}${x.toFixed(1)},${y.toFixed(1)}`;
        connected = true;
        return segment;
      })
      .join(' ');
  };

  const activeState = d?.brain.state || policyState;
  const stateMeta = getStateSemantic(activeState);
  const StateIcon = stateMeta.Icon;

  const mutatedCount = sysfsNodes.filter((n) => n.status === 'MUTATED').length;
  const clampedCount = sysfsNodes.filter((n) => n.status === 'CLAMPED').length;

  const chartSamples = (
    history.length > 0 &&
    history[history.length - 1].sampleIndex === currentSample.sampleIndex
      ? [...history.slice(0, -1), currentSample]
      : [...history, currentSample]
  ).slice(-18);

  const svgWidth = 680;
  const svgHeight = 196;
  const padX = 38;
  const padY = 18;
  const plotW = svgWidth - padX * 2;
  const plotH = svgHeight - padY * 2;

  const tempMinScale = 24;
  const tempMaxScale = 64;

  const tempToY = (tempC: number) => {
    const clamped = Math.min(tempMaxScale, Math.max(tempMinScale, tempC));
    return padY + plotH - ((clamped - tempMinScale) / (tempMaxScale - tempMinScale)) * plotH;
  };

  const pctToY = (pct: number) => {
    const clamped = Math.min(100, Math.max(0, pct));
    return padY + plotH - (clamped / 100) * plotH;
  };

  const sampleCoords = chartSamples.map((s, idx) => {
    const x =
      chartSamples.length === 1
        ? padX + plotW / 2
        : padX + (idx / (chartSamples.length - 1)) * plotW;
    const yTemp = tempToY(s.thermalMillidegrees / 1000);
    const yCpu = pctToY(s.cpuUsagePct);
    return { s, x, yTemp, yCpu };
  });

  const tempPoints = sampleCoords.map((c) => `${c.x.toFixed(1)},${c.yTemp.toFixed(1)}`).join(' ');
  const cpuPoints = sampleCoords.map((c) => `${c.x.toFixed(1)},${c.yCpu.toFixed(1)}`).join(' ');

  const cpuAreaPoints =
    sampleCoords.length > 1
      ? `${sampleCoords[0].x.toFixed(1)},${(padY + plotH).toFixed(1)} ${cpuPoints} ${sampleCoords[
          sampleCoords.length - 1
        ].x.toFixed(1)},${(padY + plotH).toFixed(1)}`
      : '';

  const guardEnterY = tempToY(55);
  const guardExitY = tempToY(52);
  const warmBandY = tempToY(48);

  const displayThermal =
    d?.eyes.thermal != null
      ? d.eyes.thermal
      : currentSample.thermalMillidegrees / 1000;
  const displayHottest =
    d?.eyes.hottest != null
      ? d.eyes.hottest
      : currentSample.hottestThermalMillidegrees / 1000;
  const displayConfidence =
    d?.eyes.confidence != null ? d.eyes.confidence : currentSample.confidence;

  const inspectedSample =
    selectedTickIdx != null && sampleCoords[selectedTickIdx]
      ? sampleCoords[selectedTickIdx].s
      : chartSamples[chartSamples.length - 1];

  const brainRows = d && window.CoreFlowStatus ? window.CoreFlowStatus.brainRows(d) : [];
  const deviceRows = d && window.CoreFlowStatus ? window.CoreFlowStatus.deviceRows(d) : [];

  return (
    <div className="space-y-6">
      {/* Device Header Context */}
      <div className="flex flex-col sm:flex-row sm:items-end justify-between gap-3 pb-4 border-b border-slate-200">
        <div>
          <div className="text-xs text-slate-500 flex flex-wrap items-center gap-x-1.5 gap-y-0.5">
            <span className="font-medium text-slate-700">
              {d?.device.present
                ? `${d.device.manufacturer} ${d.device.model} (Android ${d.device.androidRelease})`
                : isNativeBridge
                  ? 'Device identity unavailable'
                  : `${profile.manufacturer} ${profile.model} (Android ${profile.androidRelease})`}
            </span>
            <span aria-hidden="true">·</span>
            <span className="font-mono">
              {d?.device.abi || (isNativeBridge ? 'ABI unavailable' : profile.abi)}
            </span>
            <span aria-hidden="true">·</span>
            <span className="font-mono">
              kernel {d?.device.kernelRelease || (isNativeBridge ? 'unavailable' : profile.kernelRelease)}
            </span>
          </div>
          <h1 className="mt-1 text-2xl font-bold tracking-tight text-slate-900">
            Autonomous Runtime Pipeline
          </h1>
        </div>
        <div className="text-xs text-slate-600 font-mono tabular-nums bg-white border border-slate-200 rounded-lg px-3 py-1.5 self-start sm:self-auto">
          <span>Sample #{d?.memory?.lifetimeSample ?? d?.sample ?? currentSample.sampleIndex}</span>
          {isNativeBridge && <span className="ml-1.5 text-slate-500">(sesi #{d?.sample ?? 0})</span>}
          <span className="mx-2 text-slate-300" aria-hidden="true">|</span>
          <span>v{d?.version || '3.1.2'}</span>
          <span className="mx-2 text-slate-300" aria-hidden="true">|</span>
          <span>Poll {d?.interval ?? effectiveSleepSec}s</span>
        </div>
      </div>

      {/* Live C++ StatusSnapshot Blocker / Authority Summary */}
      {statusSummary && (
        <div className="cf-stagger-1 cf-card bg-white border border-slate-200 rounded-xl p-4 flex flex-col sm:flex-row sm:items-center justify-between gap-3">
          <div className="space-y-0.5">
            <div className="flex items-center gap-2 text-xs font-mono text-slate-500">
              <span
                className={`cf-live-dot inline-block w-2 h-2 rounded-full ${
                  statusSummary.tone === 'ok'
                    ? 'bg-emerald-500'
                    : statusSummary.tone === 'warn'
                      ? 'bg-amber-500'
                      : statusSummary.tone === 'bad'
                        ? 'bg-red-500'
                        : 'bg-blue-500'
                }`}
                aria-hidden="true"
              />
              <span>
                status.json blocker={statusSummary.code} · {parsedStatus?.stale ? 'STALE' : d?.recoveryPhase ? 'HEARTBEAT ACTIVE · TELEMETRY PAUSED' : 'FRESH'}
              </span>
            </div>
            <div className="text-sm font-semibold text-slate-900">
              {statusSummary.title} —{' '}
              <span className="font-normal text-slate-600">{statusSummary.detail}</span>
            </div>
          </div>
          {statusSummary.code === 'OBSERVE_MODE' && (
            <button
              type="button"
              onClick={onOpenPolicy}
              className="px-3.5 py-2 text-xs font-semibold text-blue-700 bg-blue-50 border border-blue-200 rounded-lg hover:bg-blue-100 whitespace-nowrap shrink-0"
            >
              Open Engine Settings &rarr;
            </button>
          )}
        </div>
      )}

      {/* Full-Width 4-Column Metric Strip */}
      <section
        aria-label="Primary runtime metrics"
        className="cf-stagger-1 cf-card bg-white border border-slate-200 rounded-xl grid grid-cols-1 sm:grid-cols-2 xl:grid-cols-4 divide-y sm:divide-y-0 sm:divide-x divide-slate-200 overflow-hidden"
      >
        <div className="p-5 flex flex-col justify-between">
          <div>
            <div className="flex items-center justify-between gap-2">
              <span className="text-xs font-medium text-slate-500">Engine State &amp; Activity</span>
              <span className={`px-2 py-0.5 text-[10px] font-mono font-semibold border rounded ${stateMeta.badgeClass}`}>
                {d && window.CoreFlowStatus ? window.CoreFlowStatus.activity(d) : decision}
              </span>
            </div>
            <div className={`mt-2 flex items-center gap-2 text-lg font-bold font-mono ${stateMeta.colorClass}`}>
              <StateIcon className="w-5 h-5 shrink-0" aria-hidden="true" />
              <span>{activeState}</span>
            </div>
            <div className="mt-1.5 text-xs text-slate-500">
              {isNativeBridge
                ? d?.selection?.current && d.selection.winner === 'KEEP_CURRENT' ? statusSummary?.title || d.selection.reason : d?.brain.skipReason || d?.brain.planReason || 'See daemon policy and safety hold details.'
                : stateMeta.note}
            </div>
          </div>
          <MiniSvgMeter
            ratio={
              activeState === RuntimeState.ThermalGuard || activeState === RuntimeState.Pressure
                ? 1
                : activeState === RuntimeState.Warming || activeState === RuntimeState.Elevated
                  ? 0.68
                  : activeState === RuntimeState.Idle
                    ? 0.2
                    : 0.45
            }
            color={stateMeta.barColor}
          />
        </div>

        <div className="p-5 flex flex-col justify-between">
          <div>
            <div className="flex items-center justify-between gap-2">
              <span className="text-xs font-medium text-slate-500">Authority &amp; Plan Action</span>
              <span className="text-[11px] font-mono text-slate-500">
                {d?.brain.workload || pillarMode}
              </span>
            </div>
            <div className="mt-2 text-lg font-bold font-mono text-slate-900">
              {d ? (d.recoveryPhase ? 'RECOVERING' : d.brain.mode === 'adaptive' && d.brain.armed ? 'ACTIVE' : 'HELD') : pillarMode}
            </div>
            <div className="mt-1.5 text-xs text-slate-500">
              <span>Plan: </span>
              <span className="font-mono font-medium text-slate-700">
                {d?.brain.planAction || decision}
              </span>
              {d?.intelligence?.interactionState && (
                <span className="ml-1.5 font-mono text-slate-400">
                  · Interaction: {d.intelligence.interactionState}
                </span>
              )}
            </div>
          </div>
          <MiniSvgMeter
            ratio={d ? (d.brain.mode === 'adaptive' && d.brain.armed ? 1 : 0.3) : 0.8}
            color={d?.brain.mode === 'adaptive' && d?.brain.armed ? '#2563EB' : '#64748B'}
          />
        </div>

        <div className="p-5 flex flex-col justify-between">
          <div>
            <div className="text-xs font-medium text-slate-500">Primary &amp; Hottest Thermal</div>
            <div className="mt-2 text-lg font-bold font-mono text-slate-900 tabular-nums">
              {isNativeBridge && d?.eyes.thermal == null ? 'unavailable' : `${displayThermal.toFixed(1)} °C`}
              <span className="mx-1.5 text-slate-300 font-normal">/</span>
              <span className="text-sm font-semibold text-slate-600">
                {isNativeBridge && d?.eyes.hottest == null ? 'unavailable' : `${displayHottest.toFixed(1)} °C peak`}
              </span>
            </div>
            <div className="mt-1.5 text-xs text-slate-500 flex items-center gap-1.5">
              <span>Trend:</span>
              {renderTrendInline(d?.eyes.thermalTrend || currentSample.thermalTrend)}
            </div>
          </div>
          <MiniSvgMeter
            ratio={(displayThermal - 25) / (60 - 25)}
            color={displayThermal >= 55 ? '#DC2626' : displayThermal >= 48 ? '#D97706' : '#059669'}
          />
        </div>

        <div className="p-5 flex flex-col justify-between">
          <div>
            <div className="text-xs font-medium text-slate-500">Confidence &amp; Hands Ledger</div>
            <div className="mt-2 text-lg font-bold font-mono text-slate-900 tabular-nums">
              {isNativeBridge && d?.eyes.confidence == null
                ? 'unavailable'
                : `${(displayConfidence * 100).toFixed(0)}%`}
              <span className="ml-2 text-xs font-normal text-slate-500">
                {isNativeBridge && d?.eyes.confidence == null
                  ? ''
                  : `(${displayConfidence.toFixed(2)} / 1.00)`}
              </span>
            </div>
            <div className="mt-1.5 text-xs text-slate-500 font-mono tabular-nums">
              {d
                ? `${d.hands.verified} verified · ${d.hands.rolledBack} rolled back · ${d.device.discoveredResources} nodes`
                : `${mutatedCount} mutated · ${clampedCount} clamped`}
            </div>
          </div>
          <MiniSvgMeter
            ratio={displayConfidence ?? 0}
            color={displayConfidence >= 0.6 ? '#2563EB' : '#D97706'}
          />
        </div>
      </section>

      {/* C++ 4-Stage Pipeline (Context -> Policy -> Authority -> Actuators) */}
      {pipelineStages.length > 0 && (
        <section
          aria-label="Decision pipeline stages"
          className="cf-stagger-2 cf-card bg-white border border-slate-200 rounded-xl grid grid-cols-1 sm:grid-cols-2 lg:grid-cols-4 divide-y sm:divide-y-0 sm:divide-x divide-slate-200 overflow-hidden"
        >
          {pipelineStages.map((stage, i) => {
            const stageTitle = stage.label || stage.name || stage.id;
            const dotClass =
              stage.tone === 'ok'
                ? 'bg-emerald-500'
                : stage.tone === 'warn'
                  ? 'bg-amber-500'
                  : stage.tone === 'bad'
                    ? 'bg-red-500'
                    : 'bg-blue-500';
            return (
              <div key={stage.id} className="p-4 flex flex-col justify-between">
                <div>
                  <div className="flex items-center justify-between text-xs text-slate-500">
                    <span className="font-mono font-medium">
                      0{i + 1}. {stageTitle}
                    </span>
                    <span className={`w-2 h-2 rounded-full ${dotClass}`} aria-hidden="true" />
                  </div>
                  <div
                    className={`mt-1.5 text-sm font-bold font-mono ${
                      stage.tone === 'ok'
                        ? 'text-emerald-700'
                        : stage.tone === 'warn'
                          ? 'text-amber-700'
                          : stage.tone === 'bad'
                            ? 'text-red-700'
                            : 'text-slate-900'
                    }`}
                  >
                    {stage.state}
                  </div>
                </div>
                <div className="mt-1 text-xs text-slate-600 leading-relaxed">{stage.note}</div>
              </div>
            );
          })}
        </section>
      )}

      {isNativeBridge && d && (
        <section className="cf-stagger-2 cf-card p-5 bg-white border border-slate-200 rounded-xl space-y-2">
          <h2 className="text-sm font-semibold text-slate-900">Memori &amp; kelanjutan</h2>
          <p className="text-sm text-slate-700 tabular-nums">
            ID lintas restart: <strong>{d.memory?.lifetimeSample ?? 'belum tersedia'}</strong> · Sesi ini:{' '}
            {d.sample} · Pengalaman: {d.intelligence?.learnedCandidates ?? 0} kandidat
          </p>
          <p className="text-xs text-slate-500 leading-relaxed">
            {d.memory?.sampleCheckpointOk
              ? 'Penghitung lintas restart tersimpan dengan reservasi ID. Reboot mendadak dapat melewati sebagian nomor agar ID tidak dipakai ulang.'
              : 'Kelanjutan penghitung belum terkonfirmasi. Periksa log sebelum mengandalkan ID lintas restart.'}{' '}
            Pengalaman dan konfigurasi tersimpan di disk; baseline pengujian selalu diambil ulang setelah boot.
          </p>
        </section>
      )}

      {/* Telemetry Horizon & Signal Trends Grid */}
      {isNativeBridge && d ? (
        <section className="cf-stagger-3 cf-card bg-white border border-slate-200 rounded-xl divide-y divide-slate-100">
          <div className="p-5 space-y-3">
            <h2 className="text-base font-semibold text-slate-900">Measured daemon telemetry</h2>
            <p className="text-xs text-slate-500">
              Recent measured snapshots; gaps indicate unavailable readings.
            </p>
            <svg
              viewBox="0 0 680 200"
              className="w-full h-48"
              role="img"
              aria-label="Measured daemon temperature and CPU history"
            >
              <line x1="40" y1="170" x2="640" y2="170" stroke="#CBD5E1" />
              {temperatures.length > 0 && (
                <>
                  <text x="3" y="24" fontSize="11" fill="#DC2626">
                    {maxTemperature}°C
                  </text>
                  <text x="3" y="170" fontSize="11" fill="#DC2626">
                    {minTemperature}°C
                  </text>
                  <path
                    d={measuredPath('Thermal(C)', minTemperature, maxTemperature)}
                    fill="none"
                    stroke="#DC2626"
                    strokeWidth="2"
                  />
                </>
              )}
              <text x="643" y="24" fontSize="11" fill="#2563EB">
                100%
              </text>
              <text x="643" y="170" fontSize="11" fill="#2563EB">
                0%
              </text>
              <path
                d={measuredPath('CPU(%)', 0, 100)}
                fill="none"
                stroke="#2563EB"
                strokeWidth="2"
              />
              <text x="40" y="192" fontSize="11" fill="#64748B">
                {measured.length
                  ? new Date(Number(measured[0].UpdatedEpoch) * 1000).toLocaleTimeString()
                  : 'No measured samples'}
              </text>
              <text x="640" y="192" textAnchor="end" fontSize="11" fill="#64748B">
                {measured.length
                  ? new Date(
                      Number(measured[measured.length - 1].UpdatedEpoch) * 1000
                    ).toLocaleTimeString()
                  : ''}
              </text>
            </svg>
            <div className="flex flex-wrap gap-4 text-xs">
              <span className="text-red-700 font-medium">Temperature · left axis</span>
              <span className="text-blue-700 font-medium">CPU · right axis</span>
            </div>
          </div>
          {(window.CoreFlowStatus?.eyesRows(d) ?? []).map(([name, value, detail]) => (
            <div
              key={name}
              className="px-5 py-3.5 text-xs flex flex-col sm:flex-row sm:items-center justify-between gap-1 break-words [overflow-wrap:anywhere]"
            >
              <div className="font-medium text-slate-600">{name}</div>
              <div className="sm:text-right">
                <span className="font-mono font-semibold text-slate-900 tabular-nums">{value}</span>
                {detail && <span className="ml-2 text-slate-500">{detail}</span>}
              </div>
            </div>
          ))}
        </section>
      ) : (
        <div className="cf-stagger-3 grid grid-cols-1 xl:grid-cols-12 gap-6 items-stretch">
          <section className="xl:col-span-7 cf-card bg-white border border-slate-200 rounded-xl p-5 flex flex-col justify-between">
            <div className="flex flex-wrap items-start justify-between gap-2 mb-3">
              <div>
                <h2 className="text-base font-semibold text-slate-900">
                  Thermal Hysteresis &amp; CPU Utilization Trace
                </h2>
                <p className="text-xs text-slate-500 mt-0.5">
                  Calibrated SoC thresholds: Warm &ge; 48.0 °C · Guard enters &ge; 55.0 °C and holds until &le; 52.0 °C.
                </p>
              </div>
              <div className="flex items-center gap-4 text-xs text-slate-600">
                <span className="inline-flex items-center gap-1.5">
                  <span className="w-3 h-0.5 bg-red-600 inline-block rounded-full" />
                  SoC Temp (°C)
                </span>
                <span className="inline-flex items-center gap-1.5">
                  <span className="w-3 h-0.5 bg-blue-600 inline-block rounded-full" />
                  CPU Active (%)
                </span>
              </div>
            </div>

            <div className="w-full overflow-x-auto">
              <svg
                viewBox={`0 0 ${svgWidth} ${svgHeight}`}
                className="w-full h-52 select-none"
                role="img"
                aria-label="Line chart of SoC temperature and CPU utilization over recent ticks"
                onMouseLeave={() => setSelectedTickIdx(null)}
              >
                <defs>
                  <linearGradient id="cfCpuAreaGrad" x1="0" y1="0" x2="0" y2="1">
                    <stop offset="0%" stopColor="#2563EB" stopOpacity="0.16" />
                    <stop offset="100%" stopColor="#2563EB" stopOpacity="0.0" />
                  </linearGradient>
                </defs>

                {/* Hysteresis Lock Band (52°C .. 55°C) */}
                <rect
                  x={padX}
                  y={guardEnterY}
                  width={plotW}
                  height={guardExitY - guardEnterY}
                  fill="#FEF3C7"
                  opacity={0.45}
                />

                <line
                  x1={padX}
                  y1={guardEnterY}
                  x2={padX + plotW}
                  y2={guardEnterY}
                  stroke="#DC2626"
                  strokeDasharray="4 4"
                  strokeWidth={1}
                />
                <text
                  x={padX + 4}
                  y={guardEnterY - 4}
                  className="fill-red-700 text-[10px] font-mono"
                >
                  55.0°C Guard Enter
                </text>

                <line
                  x1={padX}
                  y1={guardExitY}
                  x2={padX + plotW}
                  y2={guardExitY}
                  stroke="#D97706"
                  strokeDasharray="3 3"
                  strokeWidth={1}
                />
                <text
                  x={padX + 4}
                  y={guardExitY + 10}
                  className="fill-amber-700 text-[10px] font-mono"
                >
                  52.0°C Hysteresis Exit
                </text>

                <line
                  x1={padX}
                  y1={warmBandY}
                  x2={padX + plotW}
                  y2={warmBandY}
                  stroke="#94A3B8"
                  strokeDasharray="2 4"
                  strokeWidth={1}
                />
                <text
                  x={padX + plotW - 4}
                  y={warmBandY - 4}
                  textAnchor="end"
                  className="fill-slate-500 text-[10px] font-mono"
                >
                  48.0°C Warm
                </text>

                <line
                  x1={padX}
                  y1={padY + plotH}
                  x2={padX + plotW}
                  y2={padY + plotH}
                  stroke="#CBD5E1"
                  strokeWidth={1}
                />

                {cpuAreaPoints && <polygon points={cpuAreaPoints} fill="url(#cfCpuAreaGrad)" />}

                {sampleCoords.length > 1 && (
                  <polyline
                    fill="none"
                    stroke="#2563EB"
                    strokeWidth={2}
                    strokeLinejoin="round"
                    strokeLinecap="round"
                    points={cpuPoints}
                  />
                )}

                {sampleCoords.length > 1 && (
                  <polyline
                    fill="none"
                    stroke="#DC2626"
                    strokeWidth={2.25}
                    strokeLinejoin="round"
                    strokeLinecap="round"
                    points={tempPoints}
                  />
                )}

                {sampleCoords.map((c, idx) => {
                  const isFocused =
                    selectedTickIdx === idx ||
                    (selectedTickIdx === null && idx === sampleCoords.length - 1);
                  return (
                    <g
                      key={`pt-${c.s.sampleIndex}-${idx}`}
                      onMouseEnter={() => setSelectedTickIdx(idx)}
                      onClick={() => setSelectedTickIdx(idx)}
                      className="cursor-pointer"
                    >
                      {isFocused && (
                        <line
                          x1={c.x}
                          y1={padY}
                          x2={c.x}
                          y2={padY + plotH}
                          stroke="#94A3B8"
                          strokeDasharray="2 2"
                          strokeWidth={1}
                        />
                      )}
                      <circle
                        cx={c.x}
                        cy={c.yCpu}
                        r={isFocused ? 4 : 2.5}
                        fill="#2563EB"
                        className="cf-svg-point"
                      />
                      <circle
                        cx={c.x}
                        cy={c.yTemp}
                        r={isFocused ? 4.5 : 3}
                        fill="#DC2626"
                        className="cf-svg-point"
                      />
                      <rect
                        x={c.x - 12}
                        y={padY}
                        width={24}
                        height={plotH}
                        fill="transparent"
                      />
                    </g>
                  );
                })}
              </svg>
            </div>

            <div className="mt-3 pt-3 border-t border-slate-100 flex flex-wrap items-center justify-between gap-2 text-xs text-slate-600">
              <div className="font-mono tabular-nums">
                <span className="text-slate-500">Tick #{inspectedSample.sampleIndex}: </span>
                <span className="font-semibold text-red-700">
                  {(inspectedSample.thermalMillidegrees / 1000).toFixed(1)}°C
                </span>
                <span className="mx-1.5 text-slate-300">·</span>
                <span className="font-semibold text-blue-700">
                  CPU {inspectedSample.cpuUsagePct.toFixed(1)}%
                </span>
                <span className="mx-1.5 text-slate-300">·</span>
                <span>load1={inspectedSample.load1.toFixed(2)}</span>
              </div>
              <div>
                <span>Hold / Alert: </span>
                <span className="font-mono font-medium text-slate-900">
                  {d?.brain.hold && d.brain.hold !== 'NONE' ? d.brain.hold : lastNotification}
                </span>
              </div>
            </div>
          </section>

          {/* Observer Signal Breakdown */}
          <section className="xl:col-span-5 cf-card bg-white border border-slate-200 rounded-xl flex flex-col justify-between overflow-hidden">
            <div className="p-5 border-b border-slate-200 flex items-center justify-between gap-2">
              <div>
                <h2 className="text-base font-semibold text-slate-900">
                  Observer &amp; Context Telemetry
                </h2>
                <p className="text-xs text-slate-500">
                  Live readings from <span className="font-mono">src/observe/</span> &amp;{' '}
                  <span className="font-mono">src/understand/</span>.
                </p>
              </div>
              <button
                type="button"
                onClick={onOpenSysfs}
                className="px-2.5 py-1.5 text-xs font-semibold text-blue-700 bg-blue-50 border border-blue-200 rounded-lg hover:bg-blue-100 whitespace-nowrap"
              >
                Inspect Sysfs &rarr;
              </button>
            </div>

            <div className="divide-y divide-slate-200 text-xs flex-1">
              <div className="px-5 py-3 flex items-center justify-between">
                <div>
                  <div className="font-medium text-slate-900">CPU &amp; Load Average</div>
                  <div className="text-slate-500">/proc/stat &amp; /proc/loadavg</div>
                </div>
                <div className="text-right font-mono tabular-nums">
                  <div className="font-semibold text-slate-900">
                    CPU{' '}
                    {d?.eyes.cpu != null
                      ? `${(d.eyes.cpu * 100).toFixed(1)}%`
                      : `${currentSample.cpuUsagePct.toFixed(1)}%`}{' '}
                    · GPU {capabilities.kgslAvailable ? `${currentSample.gpuBusyPct}%` : 'SKIP'}
                  </div>
                  <div className="text-slate-500">
                    load1={(d?.eyes.load ?? currentSample.load1).toFixed(2)}
                  </div>
                </div>
              </div>

              <div className="px-5 py-3 flex items-center justify-between">
                <div>
                  <div className="font-medium text-slate-900">Block I/O Throughput</div>
                  <div className="text-slate-500">Read / Write KB/s</div>
                </div>
                <div className="text-right font-mono tabular-nums">
                  <div className="font-semibold text-slate-900">
                    R {(d?.eyes.ioRead ?? currentSample.ioReadKbs).toFixed(0)} KB/s · W{' '}
                    {(d?.eyes.ioWrite ?? currentSample.ioWriteKbs).toFixed(0)} KB/s
                  </div>
                  <div>{renderTrendInline(d?.eyes.loadTrend || currentSample.loadTrend)}</div>
                </div>
              </div>

              <div className="px-5 py-3 flex items-center justify-between">
                <div>
                  <div className="font-medium text-slate-900">Available Memory &amp; ZRAM</div>
                  <div className="text-slate-500">
                    /proc/meminfo · OEM swappiness={profile.stockSwappiness}/200
                  </div>
                </div>
                <div className="text-right font-mono tabular-nums">
                  <div className="font-semibold text-slate-900">
                    {(
                      (d?.eyes.mem != null ? d.eyes.mem : currentSample.memAvailableRatio) * 100
                    ).toFixed(1)}
                    % free
                  </div>
                  <div>{renderTrendInline(d?.eyes.memTrend || currentSample.memoryTrend)}</div>
                </div>
              </div>

              <div className="px-5 py-3 flex items-center justify-between">
                <div>
                  <div className="font-medium text-slate-900">Power &amp; Battery</div>
                  <div className="text-slate-500">/sys/class/power_supply</div>
                </div>
                <div className="text-right font-mono tabular-nums">
                  <div className="font-semibold text-slate-900">
                    {(d?.eyes.charging ?? currentSample.charging) ? 'CHARGING' : 'DISCHARGING'} ·{' '}
                    {d?.eyes.battery ?? currentSample.batteryPercent}%
                  </div>
                  <div className="text-slate-500">
                    Headroom: {d ? (d.brain.powerHeadroom ? 'OK' : 'LOW') : 'OK'}
                  </div>
                </div>
              </div>

              <div className="px-5 py-3 flex items-center justify-between bg-slate-50/70">
                <div>
                  <div className="font-medium text-slate-900">Kernel Capability Registry</div>
                  <div className="text-slate-500">cgroupv2 / uclamp / devfreq</div>
                </div>
                <div className="text-right font-mono tabular-nums">
                  <div className="font-semibold text-slate-900">
                    {d?.device.discoveredResources ?? profile.discoveredResources} discovered /{' '}
                    {d?.device.mutationReadyResources ?? profile.mutationReadyResources} ready
                  </div>
                  <div className="text-slate-500">
                    uclamp={d?.device.uclamp !== false ? 'YES' : 'NO'} · cgroupv2=
                    {d?.device.cgroupV2 !== false ? 'YES' : 'NO'}
                  </div>
                </div>
              </div>
            </div>
          </section>
        </div>
      )}

      {/* System Semantics & Kernel Understanding Diagnostic Card */}
      <section className="cf-stagger-4 cf-card bg-white border border-slate-200 rounded-xl p-5 space-y-4">
        <div className="flex flex-col sm:flex-row sm:items-center justify-between gap-2 pb-3 border-b border-slate-100">
          <div className="flex items-center gap-2.5">
            <span className="inline-flex items-center justify-center w-8 h-8 rounded-lg bg-blue-50 text-blue-700 border border-blue-200 shrink-0">
              <Layers className="w-4 h-4" aria-hidden="true" />
            </span>
            <div>
              <h2 className="text-sm font-semibold text-slate-900">
                Pemahaman Sistem &amp; Semantik Kernel Android Modern
              </h2>
              <p className="text-xs text-slate-500">
                Kontrak domain kernel nyata yang dipatuhi <span className="font-mono">EffectModel</span>,{' '}
                <span className="font-mono">StrategyCatalog</span>, dan{' '}
                <span className="font-mono">EcosystemKnowledge</span>.
              </p>
            </div>
          </div>
          <span className="text-[11px] font-mono text-slate-500">
            Linux 5.8+ ZRAM · Power HAL Coexistence
          </span>
        </div>

        <div className="grid grid-cols-1 md:grid-cols-3 gap-3 text-xs">
          <div className="p-3.5 bg-slate-50 border border-slate-200 rounded-xl space-y-1.5">
            <div className="flex items-center justify-between font-mono font-semibold text-slate-900">
              <span>vm.swappiness [0..200]</span>
              <span className="text-blue-700">
                {isNativeBridge ? 'ZRAM-Aware' : `OEM ${profile.stockSwappiness}`}
              </span>
            </div>
            <p className="text-slate-600 leading-relaxed">
              Pada kernel Android modern dengan ZRAM cepat di RAM, batas valid adalah{' '}
              <span className="font-mono">0..200</span> dan bawaan pabrik umumnya{' '}
              <span className="font-mono">100–160</span> (seperti <span className="font-mono">120</span>), bukan 60 desktop. CoreFlow menghormati baseline ZRAM OEM dan tidak menaikkan swappiness secara buta saat tekanan memori.
            </p>
          </div>

          <div className="p-3.5 bg-slate-50 border border-slate-200 rounded-xl space-y-1.5">
            <div className="flex items-center justify-between font-mono font-semibold text-slate-900">
              <span>scaling_max_freq</span>
              <span className="text-emerald-700">FrameworkOwned</span>
            </div>
            <p className="text-slate-600 leading-relaxed">
              Batas frekuensi klaster CPU dimiliki oleh Android Power HAL &amp;{' '}
              <span className="font-mono">thermal-engine</span>. CoreFlow membacanya secara read-only untuk deteksi konflik dan memakai{' '}
              <span className="font-mono">cgroup v2 uclamp</span> serta target load scheduler yang aman untuk rollback.
            </p>
          </div>

          <div className="p-3.5 bg-slate-50 border border-slate-200 rounded-xl space-y-1.5">
            <div className="flex items-center justify-between font-mono font-semibold text-slate-900">
              <span>Thermal Hysteresis</span>
              <span className="text-amber-700">55°C / 52°C</span>
            </div>
            <p className="text-slate-600 leading-relaxed">
              Sensor die SoC modern bekerja normal pada <span className="font-mono">42–52 °C</span> saat beban aktif.{' '}
              <span className="font-mono">THERMAL_GUARD</span> dikalibrasi masuk pada{' '}
              <span className="font-mono">&ge; 55.0 °C</span> dan lepas pada{' '}
              <span className="font-mono">&le; 52.0 °C</span> agar tidak terkunci permanen.
            </p>
          </div>
        </div>
      </section>

      {/* Expandable Brain & Device Ledger from CoreFlowStatus */}
      {d && (brainRows.length > 0 || deviceRows.length > 0) && (
        <details className="cf-stagger-4 cf-card bg-white border border-slate-200 rounded-xl overflow-hidden group">
          <summary className="px-5 py-4 flex items-center justify-between gap-3 cursor-pointer hover:bg-slate-50/80">
            <div className="flex items-center gap-2.5">
              <Cpu className="w-4 h-4 text-slate-500 shrink-0" aria-hidden="true" />
              <div>
                <div className="text-sm font-semibold text-slate-900">
                  Brain, Intelligence &amp; Consultation Ledger
                </div>
                <div className="text-xs text-slate-500">
                  Rincian lengkap keputusan agen, metrik render SurfaceFlinger, dan audit namespace.
                </div>
              </div>
            </div>
            <ChevronDown
              className="w-4 h-4 text-slate-500 transition-transform duration-200 group-open:rotate-180 shrink-0"
              aria-hidden="true"
            />
          </summary>
          <div className="border-t border-slate-200 grid grid-cols-1 lg:grid-cols-2 divide-y lg:divide-y-0 lg:divide-x divide-slate-200 text-xs">
            <div className="divide-y divide-slate-100">
              {brainRows.map(([label, val, note]) => (
                <div key={label} className="px-5 py-2.5 flex items-start justify-between gap-4">
                  <div className="text-slate-600 font-medium">{label}</div>
                  <div className="text-right min-w-0">
                    <div className="font-mono font-semibold text-slate-900 break-words">{val}</div>
                    {note && <div className="text-[11px] text-slate-500">{note}</div>}
                  </div>
                </div>
              ))}
            </div>
            <div className="divide-y divide-slate-100">
              {deviceRows.map(([label, val, note]) => (
                <div key={label} className="px-5 py-2.5 flex items-start justify-between gap-4">
                  <div className="text-slate-600 font-medium">{label}</div>
                  <div className="text-right min-w-0">
                    <div className="font-mono font-semibold text-slate-900 break-words">{val}</div>
                    {note && <div className="text-[11px] text-slate-500">{note}</div>}
                  </div>
                </div>
              ))}
            </div>
          </div>
        </details>
      )}

      {/* Raw Silicon Envelope vs OEM Factory Throttling & Tier Audit (Preview) */}
      {!isNativeBridge && (
        <section className="cf-stagger-4 cf-card bg-white border border-slate-200 rounded-xl p-5 space-y-4">
          <div className="flex flex-col lg:flex-row lg:items-center justify-between gap-3 pb-3 border-b border-slate-100">
            <div>
              <div className="text-xs text-slate-500 flex flex-wrap items-center gap-x-2 gap-y-0.5">
                <span className="font-semibold text-slate-700">{profile.soc}</span>
                <span aria-hidden="true">·</span>
                <span>{profile.siliconArchitecture}</span>
                <span aria-hidden="true">·</span>
                <span className="font-mono">{profile.oemRomLabel}</span>
              </div>
              <h2 className="mt-1 text-base font-semibold text-slate-900">
                Raw Silicon Potential vs OEM Factory Baseline Fine-Tuning
              </h2>
            </div>

            {hybridBrain && (
              <div className="text-xs font-mono tabular-nums text-slate-600 flex flex-wrap items-center gap-x-3 gap-y-1">
                <span>
                  Brain Arm:{' '}
                  <strong className="text-blue-700">{hybridBrain.selectedArmLabel}</strong>
                </span>
                <span aria-hidden="true">·</span>
                <span>
                  Silicon Headroom Unlocked:{' '}
                  <strong className="text-emerald-700">{hybridBrain.siliconGapPct}%</strong>
                </span>
              </div>
            )}
          </div>

          {hybridBrain && (
            <div className="p-3.5 bg-slate-50 border border-slate-200 rounded-xl flex flex-col md:flex-row md:items-center justify-between gap-3 text-xs">
              <div className="text-slate-700 font-mono leading-relaxed">
                {hybridBrain.reasoningSummary}
              </div>
              <div className="w-full md:w-48 shrink-0">
                <div className="flex items-center justify-between text-[11px] font-mono text-slate-500 mb-1">
                  <span>Dynamic Silicon Drive</span>
                  <span>{(hybridBrain.unlockDrive * 100).toFixed(0)}%</span>
                </div>
                <MiniSvgMeter
                  ratio={hybridBrain.unlockDrive}
                  color={hybridBrain.unlockDrive > 0 ? '#2563EB' : '#64748B'}
                />
              </div>
            </div>
          )}

          <div className="grid grid-cols-1 lg:grid-cols-3 gap-3 text-xs">
            {profile.oemAuditItems.map((item) => (
              <div
                key={item.id}
                className="p-4 bg-slate-50 border border-slate-200 rounded-xl flex flex-col justify-between space-y-2.5"
              >
                <div className="space-y-1.5">
                  <div className="text-[11px] font-mono text-blue-700">
                    {item.domain} · Reversible Kernel Layer
                  </div>
                  <h3 className="text-sm font-semibold text-slate-900">{item.title}</h3>
                  <div className="space-y-1 text-slate-600 leading-relaxed pt-1">
                    <div>
                      <span className="font-semibold text-slate-700">OEM Factory Cap: </span>
                      <span className="font-mono text-[11px]">{item.oemStockPosture}</span>
                    </div>
                    <div>
                      <span className="font-semibold text-slate-700">Raw Silicon Spec: </span>
                      <span>{item.rawSiliconCapability}</span>
                    </div>
                    <div>
                      <span className="font-semibold text-emerald-700">CoreFlow Brain: </span>
                      <span>{item.coreflowResolution}</span>
                    </div>
                  </div>
                </div>
                <div className="pt-2 border-t border-slate-200/80 text-[11px] font-mono text-slate-500">
                  Safety: {item.reversibleMethod}
                </div>
              </div>
            ))}
          </div>
        </section>
      )}

      {isNativeBridge && d && (
        <section aria-label="Live CPUFreq policies" className="cf-card bg-white border border-slate-200 rounded-xl overflow-hidden">
          <div className="px-5 py-4 border-b border-slate-200">
            <h2 className="text-base font-semibold text-slate-900">CPUFreq · Live Kernel Policies</h2>
            <p className="mt-1 text-xs text-slate-500">Governor dan frekuensi tetap ditampilkan meski tidak ada mutasi; batas frekuensi hanya diamati.</p>
            <p className="mt-2 text-xs font-mono text-slate-600 break-words">{'Last assessment: ' + (d.selection?.cpuReason || 'Belum ada penilaian CPUFreq pada snapshot ini.')}</p>
          </div>
          <div className="divide-y divide-slate-200">
            {(d.cpuPolicies || []).map((policy) => (
              <div key={policy.path} className="px-5 py-4 text-xs space-y-2">
                <div className="font-mono text-slate-600 break-all">{policy.path}</div>
                <div className="flex flex-wrap justify-between gap-2">
                  <span className="font-semibold text-slate-900">Governor: {policy.governorReadable ? policy.currentGovernor || 'unavailable' : 'unavailable'}</span>
                  <span className="text-slate-500">{policy.governorWritable ? 'OS-writable · izin tuning diperiksa terpisah' : 'Governor write unavailable'}</span>
                </div>
                <div className="text-slate-600">CPU topology (kernel): {policy.relatedCpus || 'unavailable'} · governors: {policy.availableGovernors.join(', ') || 'unavailable'}</div>
                <div className="font-mono text-slate-500">Current {policy.currentKhz == null ? '—' : policy.currentKhz + ' kHz'} · policy min {policy.minKhz == null ? '—' : policy.minKhz + ' kHz'} · policy max {policy.maxKhz == null ? '—' : policy.maxKhz + ' kHz'}</div>
                <div className="font-mono text-slate-500">Kernel hardware min {policy.hardwareMinKhz == null ? 'unavailable' : policy.hardwareMinKhz + ' kHz'} · max {policy.hardwareMaxKhz == null ? 'unavailable' : policy.hardwareMaxKhz + ' kHz'}</div>
                <div className="font-mono text-slate-500 break-words">Kernel OPPs ({policy.availableFrequenciesKhz.length}): {policy.availableFrequenciesKhz.length ? policy.availableFrequenciesKhz.join(', ') + ' kHz' : 'not advertised; no inferred frequencies'}</div>
              </div>
            ))}
            {!(d.cpuPolicies || []).length && <p className="px-5 py-4 text-xs text-slate-500">Inventaris CPUFreq belum tersedia dari daemon. Tidak memakai data preview sebagai pengganti.</p>}
          </div>
        </section>
      )}

      {isNativeBridge && d && (
        <section aria-label="Device capability provenance" className="cf-card bg-white border border-slate-200 rounded-xl px-5 py-4 text-xs space-y-2">
          <h2 className="text-base font-semibold text-slate-900">Device Capability · Evidence</h2>
          <p className="text-slate-600">Observed: {d.device.manufacturer || 'unknown'} {d.device.model || 'unknown'} · SoC {d.device.socModel || 'unavailable'} · Android {d.device.androidRelease || 'unavailable'} (API {d.device.sdkLevel || '?'})</p>
          <p className="text-slate-600">Kernel {d.device.kernelRelease || 'unavailable'} · Security patch {d.device.securityPatch || 'unavailable'} · Firmware {d.device.firmwareIncremental || 'unavailable'}</p>
          <p className="text-slate-600">Vendor evidence: {d.device.vendorFilesParsed} files / {d.device.vendorKnobs} tunables · discovery {d.device.discoveryTruncated ? 'INCOMPLETE (bounded budget)' : 'complete within discovery budget'}</p>
          <p className="text-slate-500">Informasi berasal dari device/kernel aktif, bukan profil chipset generik. Kernel OPP yang tidak tersedia tidak dianggap aman untuk ditulis. Mutasi tetap memerlukan izin, evidence, dan pemeriksaan seluruh konfigurasi.</p>
        </section>
      )}

      {isNativeBridge && d?.lastEvaluation && (
        <details className="cf-card bg-white border border-slate-200 rounded-xl overflow-hidden">
          <summary className="px-5 py-4 cursor-pointer text-sm font-semibold text-slate-900">Last Measured Outcome · {d.lastEvaluation.reason}</summary>
          <div className="px-5 pb-4 text-xs text-slate-600">
            <p>Score {d.lastEvaluation.score?.toFixed(3) ?? '—'} · confidence {d.lastEvaluation.confidence?.toFixed(3) ?? '—'} · {d.lastEvaluation.baselineSamples}/{d.lastEvaluation.observationSamples} samples · {d.lastEvaluation.valid ? 'comparable' : 'inconclusive'}</p>
            <p className="mt-1">Perbandingan sebelum → sesudah; ketersediaan sensor dan jumlah sampel tetap ditampilkan.</p>
            <div className="mt-3 space-y-2 font-mono">
              {Object.entries(d.lastEvaluation.metrics).map(([metric, value]) => (
                <div key={metric} className="flex flex-wrap justify-between gap-2 border-t border-slate-100 pt-2">
                  <span>{metric}</span><span>{value.before?.toFixed(3) ?? '—'} → {value.after?.toFixed(3) ?? '—'} · n={value.beforeSamples}/{value.afterSamples} · score {value.score?.toFixed(3) ?? '—'}</span>
                </div>
              ))}
            </div>
          </div>
        </details>
      )}

      {/* Illustrative topology is preview-only; native policies are observed above. */}
      {!isNativeBridge && (
        <section className="cf-stagger-4 cf-card bg-white border border-slate-200 rounded-xl overflow-hidden">
          <div className="px-5 py-4 border-b border-slate-200 flex flex-wrap items-center justify-between gap-2">
            <div>
              <h2 className="text-base font-semibold text-slate-900">
                CPUFreq Actuators &amp; Silicon Cluster Envelope
              </h2>
              <p className="text-xs text-slate-500">
                Governor &amp; rate-limits dikelola dinamis dari OPP perangkat;{' '}
                <span className="font-mono">scaling_max_freq</span> diamati read-only (Power HAL owned).
              </p>
            </div>
            <span className="text-xs font-mono text-slate-500">
              {profile.cpuPolicies.length} discovered cpufreq policies
            </span>
          </div>
          <div className="overflow-x-auto">
            <table className="w-full text-left border-collapse">
              <thead>
                <tr className="border-b border-slate-200 bg-slate-50 text-xs font-medium text-slate-600">
                  <th className="py-2.5 px-5">Cluster Policy</th>
                  <th className="py-2.5 px-4">Topology</th>
                  <th className="py-2.5 px-4">OEM Governor</th>
                  <th className="py-2.5 px-4">Active Governor</th>
                  <th className="py-2.5 px-4 text-right">OEM Boot Cap</th>
                  <th className="py-2.5 px-5 text-right">Raw Silicon Spec</th>
                </tr>
              </thead>
              <tbody className="divide-y divide-slate-200 text-xs">
                {profile.cpuPolicies.map((cluster) => {
                  const govNode = sysfsNodes.find(
                    (n) => n.path === `${cluster.policyPath}/scaling_governor`
                  );
                  const maxNode = sysfsNodes.find(
                    (n) => n.path === `${cluster.policyPath}/scaling_max_freq`
                  );
                  const activeMaxKhz =
                    maxNode && maxNode.currentValue !== 'N/A'
                      ? Number(maxNode.currentValue)
                      : cluster.maxFreqKhz;
                  const activeGov = govNode?.currentValue ?? cluster.stockGovernor;
                  const isOemCapped = activeMaxKhz < cluster.siliconMaxFreqKhz;

                  return (
                    <tr key={cluster.id} className="hover:bg-slate-50/80 transition-colors">
                      <td className="py-3 px-5 font-medium text-slate-900">
                        <div>{cluster.label}</div>
                        <div className="text-[11px] font-mono text-slate-400">
                          {cluster.policyPath}
                        </div>
                      </td>
                      <td className="py-3 px-4 text-slate-600">
                        <div>{cluster.cores}</div>
                        <div className="text-[11px] font-mono text-slate-400">
                          {cluster.oppTableKhz.length} hardware OPPs
                        </div>
                      </td>
                      <td className="py-3 px-4 font-mono text-slate-600">
                        {cluster.stockGovernor}
                      </td>
                      <td className="py-3 px-4 font-mono font-semibold">
                        <span
                          className={
                            activeGov !== cluster.stockGovernor
                              ? 'text-blue-700'
                              : 'text-slate-900'
                          }
                        >
                          {activeGov}
                        </span>
                      </td>
                      <td className="py-3 px-4 text-right font-mono tabular-nums">
                        <span className={isOemCapped ? 'text-amber-700 font-semibold' : 'text-slate-700'}>
                          {(activeMaxKhz / 1_000_000).toFixed(2)} GHz
                        </span>
                      </td>
                      <td className="py-3 px-5 text-right font-mono text-emerald-700 font-semibold tabular-nums">
                        {(cluster.siliconMaxFreqKhz / 1_000_000).toFixed(2)} GHz
                      </td>
                    </tr>
                  );
                })}
              </tbody>
            </table>
          </div>
        </section>
      )}
    </div>
  );
};
