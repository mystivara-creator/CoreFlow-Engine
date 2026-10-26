import React from 'react';
import {
  AlertTriangle,
  ArrowDownRight,
  ArrowRight,
  ArrowUpRight,
  CheckCircle2,
  ShieldAlert,
} from 'lucide-react';
import { ParsedEngineStatus, PipelineStage } from '../bridge/daemonBridge';
import {
  CapabilityOverrides,
  Decision,
  DeviceHardwareProfile,
  EngineMode,
  NotificationEvent,
  RuntimeSample,
  RuntimeState,
  SysfsNodeEntry,
  Trend,
} from '../engine/types';

interface RuntimeConsoleViewProps {
  profile: DeviceHardwareProfile;
  capabilities: CapabilityOverrides;
  currentSample: RuntimeSample;
  history: RuntimeSample[];
  policyState: RuntimeState;
  decision: Decision;
  pillarMode: EngineMode;
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
        Icon: ShieldAlert,
        note:
          state === RuntimeState.ThermalGuard
            ? 'Hysteresis lock active until <= 40.0 °C'
            : 'Hysteresis lock active until >= 15.0% RAM',
      };
    case RuntimeState.Warming:
    case RuntimeState.Elevated:
      return {
        colorClass: 'text-amber-700',
        Icon: AlertTriangle,
        note:
          state === RuntimeState.Warming
            ? 'Monitoring 40–43 °C transition band'
            : 'Elevated runnable queue load (>= 1.50)',
      };
    case RuntimeState.Idle:
    case RuntimeState.Normal:
    default:
      return {
        colorClass: 'text-emerald-700',
        Icon: CheckCircle2,
        note:
          state === RuntimeState.Idle
            ? 'Deep-sleep cadence scaling active'
            : 'Nominal operating envelope',
      };
  }
}

function renderTrendInline(trend: string) {
  if (trend === Trend.Rising) {
    return (
      <span className="inline-flex items-center gap-0.5 text-amber-700 font-mono">
        <ArrowUpRight className="w-3.5 h-3.5" aria-hidden="true" />
        RISING
      </span>
    );
  }
  if (trend === Trend.Falling) {
    return (
      <span className="inline-flex items-center gap-0.5 text-blue-700 font-mono">
        <ArrowDownRight className="w-3.5 h-3.5" aria-hidden="true" />
        FALLING
      </span>
    );
  }
  if (trend === Trend.Stable) {
    return (
      <span className="inline-flex items-center gap-0.5 text-emerald-700 font-mono">
        <ArrowRight className="w-3.5 h-3.5" aria-hidden="true" />
        STABLE
      </span>
    );
  }
  return <span className="text-slate-500 font-mono">UNKNOWN</span>;
}

export const RuntimeConsoleView: React.FC<RuntimeConsoleViewProps> = ({
  profile,
  capabilities,
  currentSample,
  history,
  policyState,
  decision,
  pillarMode,
  lastNotification,
  effectiveSleepSec,
  sysfsNodes,
  parsedStatus,
  statusSummary,
  pipelineStages,
  onOpenSysfs,
  onOpenPolicy,
}) => {
  const d = parsedStatus?.data;
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
  const svgHeight = 190;
  const padX = 36;
  const padY = 18;
  const plotW = svgWidth - padX * 2;
  const plotH = svgHeight - padY * 2;

  const tempToY = (tempC: number) => {
    const clamped = Math.min(54, Math.max(24, tempC));
    return padY + plotH - ((clamped - 24) / (54 - 24)) * plotH;
  };

  const pctToY = (pct: number) => {
    const clamped = Math.min(100, Math.max(0, pct));
    return padY + plotH - (clamped / 100) * plotH;
  };

  const tempPoints = chartSamples
    .map((s, idx) => {
      const x =
        chartSamples.length === 1
          ? padX + plotW / 2
          : padX + (idx / (chartSamples.length - 1)) * plotW;
      const y = tempToY(s.thermalMillidegrees / 1000);
      return `${x.toFixed(1)},${y.toFixed(1)}`;
    })
    .join(' ');

  const cpuPoints = chartSamples
    .map((s, idx) => {
      const x =
        chartSamples.length === 1
          ? padX + plotW / 2
          : padX + (idx / (chartSamples.length - 1)) * plotW;
      const y = pctToY(s.cpuUsagePct);
      return `${x.toFixed(1)},${y.toFixed(1)}`;
    })
    .join(' ');

  const guardEnterY = tempToY(43);
  const guardExitY = tempToY(40);

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

  return (
    <div className="space-y-6">
      {/* Device Header Context */}
      <div className="flex flex-col sm:flex-row sm:items-end justify-between gap-3 pb-4 border-b border-slate-200">
        <div>
          <div className="text-xs text-slate-500">
            <span>
              {d?.device.present
                ? `${d.device.manufacturer} ${d.device.model} (Android ${d.device.androidRelease})`
                : `Android ${profile.androidRelease}`}
            </span>
            <span className="mx-1.5" aria-hidden="true">·</span>
            <span className="font-mono">{d?.device.abi || profile.abi}</span>
            <span className="mx-1.5" aria-hidden="true">·</span>
            <span className="font-mono">
              kernel {d?.device.kernelRelease || profile.kernelRelease}
            </span>
          </div>
          <h1 className="mt-1 text-2xl font-bold tracking-tight text-slate-900">
            Autonomous Runtime Pipeline
          </h1>
        </div>
        <div className="text-xs text-slate-600 font-mono tabular-nums">
          <span>Sample #{d?.sample ?? currentSample.sampleIndex}</span>
          <span className="mx-2" aria-hidden="true">·</span>
          <span>CoreFlow v{d?.version || '3.0.1'}</span>
          <span className="mx-2" aria-hidden="true">·</span>
          <span>Poll {d?.interval ?? effectiveSleepSec}s</span>
        </div>
      </div>

      {/* Live C++ StatusSnapshot Blocker / Authority Summary */}
      {statusSummary && (
        <div className="bg-white border border-slate-200 rounded-xl p-4 flex flex-col sm:flex-row sm:items-center justify-between gap-3">
          <div>
            <div className="text-xs font-mono text-slate-500">
              status.json blocker={statusSummary.code} · {parsedStatus?.stale ? 'STALE' : 'FRESH'}
            </div>
            <div className="text-sm font-semibold text-slate-900 mt-0.5">
              {statusSummary.title} —{' '}
              <span className="font-normal text-slate-600">{statusSummary.detail}</span>
            </div>
          </div>
          {statusSummary.code === 'OBSERVE_MODE' && (
            <button
              type="button"
              onClick={onOpenPolicy}
              className="px-3.5 py-1.5 text-xs font-semibold text-blue-700 bg-blue-50 border border-blue-200 rounded-lg hover:bg-blue-100 transition-colors whitespace-nowrap shrink-0"
            >
              Configure Adaptive Policy &rarr;
            </button>
          )}
        </div>
      )}

      {/* Full-Width 4-Column Metric Strip */}
      <section
        aria-label="Primary runtime metrics"
        className="bg-white border border-slate-200 rounded-xl grid grid-cols-1 sm:grid-cols-2 xl:grid-cols-4 divide-y sm:divide-y-0 sm:divide-x divide-slate-200"
      >
        <div className="p-5">
          <div className="text-xs font-medium text-slate-500">Adaptive Policy State</div>
          <div className={`mt-2 flex items-center gap-2 text-lg font-bold font-mono ${stateMeta.colorClass}`}>
            <StateIcon className="w-5 h-5 shrink-0" aria-hidden="true" />
            <span>{activeState}</span>
          </div>
          <div className="mt-1.5 text-xs text-slate-500">{stateMeta.note}</div>
        </div>

        <div className="p-5">
          <div className="text-xs font-medium text-slate-500">Authority &amp; Plan Action</div>
          <div className="mt-2 text-lg font-bold font-mono text-slate-900">
            {d ? `${d.brain.mode.toUpperCase()}${d.brain.armed ? ' (ARMED)' : ''}` : pillarMode}
          </div>
          <div className="mt-1.5 text-xs text-slate-500">
            <span>Plan: </span>
            <span className="font-mono font-medium text-slate-700">
              {d?.brain.planAction || decision}
            </span>
          </div>
        </div>

        <div className="p-5">
          <div className="text-xs font-medium text-slate-500">Primary &amp; Hottest Thermal</div>
          <div className="mt-2 text-lg font-bold font-mono text-slate-900 tabular-nums">
            {displayThermal.toFixed(1)} °C
            <span className="mx-1.5 text-slate-300 font-normal">/</span>
            <span className="text-sm font-semibold text-slate-600">
              {displayHottest.toFixed(1)} °C peak
            </span>
          </div>
          <div className="mt-1.5 text-xs text-slate-500 flex items-center gap-1.5">
            <span>Trend:</span>
            {renderTrendInline(d?.eyes.thermalTrend || currentSample.thermalTrend)}
          </div>
        </div>

        <div className="p-5">
          <div className="text-xs font-medium text-slate-500">Confidence &amp; Hands Ledger</div>
          <div className="mt-2 text-lg font-bold font-mono text-slate-900 tabular-nums">
            {(displayConfidence * 100).toFixed(0)}%
            <span className="ml-2 text-xs font-normal text-slate-500">
              ({displayConfidence.toFixed(2)} / 1.00)
            </span>
          </div>
          <div className="mt-1.5 text-xs text-slate-500 font-mono tabular-nums">
            {d
              ? `${d.hands.verified} verified · ${d.hands.rolledBack} rolled back · ${d.device.discoveredResources} nodes`
              : `${mutatedCount} mutated · ${clampedCount} clamped`}
          </div>
        </div>
      </section>

      {/* C++ 4-Stage Pipeline (Eyes -> Context -> Authority -> Hands) */}
      {pipelineStages.length > 0 && (
        <section className="bg-white border border-slate-200 rounded-xl grid grid-cols-1 sm:grid-cols-2 lg:grid-cols-4 divide-y sm:divide-y-0 sm:divide-x divide-slate-200">
          {pipelineStages.map((stage, i) => (
            <div key={stage.id} className="p-4">
              <div className="text-xs text-slate-500">
                0{i + 1}. {stage.name}
              </div>
              <div
                className={`mt-1 text-sm font-bold font-mono ${
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
              <div className="mt-1 text-xs text-slate-600">{stage.note}</div>
            </div>
          ))}
        </section>
      )}

      {/* Telemetry Horizon & Signal Trends Grid */}
      <div className="grid grid-cols-1 xl:grid-cols-12 gap-6 items-stretch">
        <section className="xl:col-span-7 bg-white border border-slate-200 rounded-xl p-5 flex flex-col justify-between">
          <div className="flex flex-wrap items-center justify-between gap-2 mb-4">
            <div>
              <h2 className="text-base font-semibold text-slate-900">
                Thermal Hysteresis &amp; CPU Utilization Trace
              </h2>
              <p className="text-xs text-slate-500">
                ThermalGuard enters at 43.0 °C and holds until &le; 40.0 °C.
              </p>
            </div>
            <div className="flex items-center gap-4 text-xs text-slate-600">
              <span className="inline-flex items-center gap-1.5">
                <span className="w-3 h-0.5 bg-red-600 inline-block" />
                SoC Temp (°C)
              </span>
              <span className="inline-flex items-center gap-1.5">
                <span className="w-3 h-0.5 bg-blue-600 inline-block" />
                CPU Active (%)
              </span>
            </div>
          </div>

          <div className="w-full overflow-x-auto">
            <svg
              viewBox={`0 0 ${svgWidth} ${svgHeight}`}
              className="w-full h-48 select-none"
              role="img"
              aria-label="Line chart of SoC temperature and CPU utilization over recent ticks"
            >
              <rect
                x={padX}
                y={guardEnterY}
                width={plotW}
                height={guardExitY - guardEnterY}
                fill="#FEF3C7"
                opacity={0.55}
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
                43°C Guard Enter
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
                y={guardExitY + 11}
                className="fill-amber-700 text-[10px] font-mono"
              >
                40°C Hysteresis Exit
              </text>

              <line
                x1={padX}
                y1={padY + plotH}
                x2={padX + plotW}
                y2={padY + plotH}
                stroke="#CBD5E1"
                strokeWidth={1}
              />

              {chartSamples.length > 1 && (
                <polyline
                  fill="none"
                  stroke="#2563EB"
                  strokeWidth={2}
                  points={cpuPoints}
                />
              )}

              {chartSamples.length > 1 && (
                <polyline
                  fill="none"
                  stroke="#DC2626"
                  strokeWidth={2.25}
                  points={tempPoints}
                />
              )}

              {chartSamples.map((s, idx) => {
                const x =
                  chartSamples.length === 1
                    ? padX + plotW / 2
                    : padX + (idx / (chartSamples.length - 1)) * plotW;
                const yT = tempToY(s.thermalMillidegrees / 1000);
                return (
                  <circle
                    key={`pt-${s.sampleIndex}-${idx}`}
                    cx={x}
                    cy={yT}
                    r={3}
                    fill="#DC2626"
                  />
                );
              })}
            </svg>
          </div>

          <div className="mt-3 pt-3 border-t border-slate-100 flex flex-wrap items-center justify-between gap-2 text-xs text-slate-600">
            <div>
              <span>Baseline Window: </span>
              <span className="font-mono font-medium text-slate-900 tabular-nums">
                {d
                  ? `${d.brain.baselineSamples} / ${d.brain.baselineTarget} pre-mutation samples`
                  : `${Math.min(history.length, 6)} / 6 samples`}
              </span>
            </div>
            <div>
              <span>Notification / Hold: </span>
              <span className="font-mono font-medium text-slate-900">
                {d?.brain.hold && d.brain.hold !== 'NONE' ? d.brain.hold : lastNotification}
              </span>
            </div>
          </div>
        </section>

        {/* Observer Signal Breakdown */}
        <section className="xl:col-span-5 bg-white border border-slate-200 rounded-xl flex flex-col justify-between overflow-hidden">
          <div className="p-5 border-b border-slate-200 flex items-center justify-between">
            <div>
              <h2 className="text-base font-semibold text-slate-900">
                Observer &amp; Context Telemetry
              </h2>
              <p className="text-xs text-slate-500">
                Live readings from src/observe/ &amp; src/understand/.
              </p>
            </div>
            <button
              type="button"
              onClick={onOpenSysfs}
              className="text-xs font-semibold text-blue-600 hover:text-blue-800 whitespace-nowrap"
            >
              Inspect Resources &rarr;
            </button>
          </div>

          <div className="divide-y divide-slate-200 text-xs">
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
                <div className="font-medium text-slate-900">Available Memory</div>
                <div className="text-slate-500">/proc/meminfo &amp; PSI headroom</div>
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

      {/* CPU Cluster Hardware Topology Table */}
      <section className="bg-white border border-slate-200 rounded-xl overflow-hidden">
        <div className="px-5 py-4 border-b border-slate-200">
          <h2 className="text-base font-semibold text-slate-900">
            CPUFreq Actuators &amp; Cluster Frequency Bounds
          </h2>
        </div>
        <div className="overflow-x-auto">
          <table className="w-full text-left border-collapse">
            <thead>
              <tr className="border-b border-slate-200 bg-slate-50 text-xs font-medium text-slate-600">
                <th className="py-2.5 px-5">Cluster Policy</th>
                <th className="py-2.5 px-4">Topology</th>
                <th className="py-2.5 px-4">Stock Governor</th>
                <th className="py-2.5 px-4">Active Governor</th>
                <th className="py-2.5 px-4 text-right">Min Freq</th>
                <th className="py-2.5 px-5 text-right">Active Max Cap</th>
              </tr>
            </thead>
            <tbody className="divide-y divide-slate-200 text-xs">
              {profile.cpuPolicies.map((cluster) => {
                const govNode = sysfsNodes.find((n) =>
                  n.path.includes(`policy${cluster.id}/scaling_governor`)
                );
                const maxNode = sysfsNodes.find((n) =>
                  n.path.includes(`policy${cluster.id}/scaling_max_freq`)
                );
                const activeMaxKhz =
                  maxNode && maxNode.currentValue !== 'N/A'
                    ? Number(maxNode.currentValue)
                    : cluster.maxFreqKhz;
                const isClamped = activeMaxKhz < cluster.maxFreqKhz;

                return (
                  <tr key={cluster.id} className="hover:bg-slate-50/80 transition-colors">
                    <td className="py-3 px-5 font-medium text-slate-900">
                      {cluster.label}
                    </td>
                    <td className="py-3 px-4 text-slate-600">{cluster.cores}</td>
                    <td className="py-3 px-4 font-mono text-slate-600">
                      {cluster.stockGovernor}
                    </td>
                    <td className="py-3 px-4 font-mono font-semibold text-slate-900">
                      {govNode?.currentValue ?? cluster.stockGovernor}
                    </td>
                    <td className="py-3 px-4 text-right font-mono text-slate-600 tabular-nums">
                      {(cluster.minFreqKhz / 1_000_000).toFixed(2)} GHz
                    </td>
                    <td className="py-3 px-5 text-right font-mono tabular-nums">
                      <span
                        className={
                          isClamped
                            ? 'font-semibold text-red-700'
                            : 'font-medium text-slate-900'
                        }
                      >
                        {(activeMaxKhz / 1_000_000).toFixed(2)} GHz
                      </span>
                      {isClamped && (
                        <span className="ml-1.5 text-slate-500">
                          (from {(cluster.maxFreqKhz / 1_000_000).toFixed(2)} GHz)
                        </span>
                      )}
                    </td>
                  </tr>
                );
              })}
            </tbody>
          </table>
        </div>
      </section>
    </div>
  );
};
