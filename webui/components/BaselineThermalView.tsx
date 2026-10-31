import React from 'react';
import {
  AlertTriangle,
  CheckCircle2,
  Play,
  RotateCcw,
  ShieldAlert,
} from 'lucide-react';
import { ParsedEngineStatus } from '../bridge/daemonBridge';
import {
  BaselineAction,
  BaselineExperimentState,
  BaselineOutcome,
  DeviceHardwareProfile,
  HybridBrainEvaluation,
  ThermalPrediction,
} from '../engine/types';

interface BaselineThermalViewProps {
  isNativeBridge?: boolean;
  profile: DeviceHardwareProfile;
  experiment: BaselineExperimentState;
  prediction: ThermalPrediction;
  hybridBrain?: HybridBrainEvaluation;
  parsedStatus: ParsedEngineStatus | null;
  onSelectGovernor: (gov: string) => void;
  onToggleInjectRegression: (val: boolean) => void;
  onStartExperiment: () => void;
  onResetExperiment: () => void;
}

function ParetoAxisBar({
  label,
  weight,
  color,
}: {
  label: string;
  weight: number;
  color: string;
}) {
  const pct = Math.min(100, Math.max(0, Number((weight * 100).toFixed(1))));
  return (
    <div className="p-3 bg-slate-50 border border-slate-200 rounded-xl space-y-1.5">
      <div className="flex items-center justify-between text-xs">
        <span className="font-medium text-slate-700">{label}</span>
        <span className="font-mono font-semibold text-slate-900 tabular-nums">
          {pct.toFixed(1)}%
        </span>
      </div>
      <svg
        viewBox="0 0 100 4"
        preserveAspectRatio="none"
        className="w-full h-1.5 rounded-full overflow-hidden"
        aria-hidden="true"
      >
        <rect x={0} y={0} width={100} height={4} rx={2} fill="#E2E8F0" />
        <rect
          x={0}
          y={0}
          width={pct}
          height={4}
          rx={2}
          fill={color}
          className="cf-svg-metric"
        />
      </svg>
    </div>
  );
}

export const BaselineThermalView: React.FC<BaselineThermalViewProps> = ({
  isNativeBridge = false,
  profile,
  experiment,
  prediction,
  hybridBrain,
  parsedStatus,
  onSelectGovernor,
  onToggleInjectRegression,
  onStartExperiment,
  onResetExperiment,
}) => {
  const availableGovernors = profile.cpuPolicies[0].availableGovernors;
  const stockGov = profile.cpuPolicies[0].stockGovernor;
  const liveBrain = parsedStatus?.data?.brain;
  const completedVerificationTicks =
    experiment.baselineSamples.length + experiment.mutatedSamples.length;

  return (
    <div className="space-y-6">
      <div className="pb-4 border-b border-slate-200 flex flex-col sm:flex-row sm:items-end justify-between gap-2">
        <div>
          <h1 className="text-2xl font-bold tracking-tight text-slate-900">
            Hybrid AI Brain, Baseline Intelligence &amp; 18f ONNX Predictor
          </h1>
          <p className="text-sm text-slate-600 mt-1">
            Contextual UCB1-Tuned Multi-Armed Bandit (<span className="font-mono">experience.hpp</span>), 4-Axis Pareto reasoning (<span className="font-mono">decision_agent.cpp</span>), and frozen 18-feature ONNX thermal horizon (<span className="font-mono">thermal_features.hpp</span>).
          </p>
        </div>
        <div className="text-xs font-mono text-slate-500 tabular-nums bg-white border border-slate-200 rounded-lg px-3 py-1.5 self-start sm:self-auto">
          UCB1-Tuned · Pareto 4D · ONNX 18f v1.2
        </div>
      </div>

      {/* Live C++ Daemon Baseline Window Counters */}
      {liveBrain && (
        <section className="cf-stagger-1 cf-card bg-white border border-slate-200 rounded-xl p-5">
          <div className="flex flex-col sm:flex-row sm:items-center justify-between gap-2 pb-4 border-b border-slate-100">
            <div>
              <h2 className="text-sm font-semibold text-slate-900">
                Daemon Window Verification State (<span className="font-mono">status.json -&gt; brain</span>)
              </h2>
              <p className="text-xs text-slate-500 mt-0.5">
                Live sample counters exported by <span className="font-mono">coreflowd</span> before and after verified kernel writes.
              </p>
            </div>
            <span className="px-2.5 py-1 rounded-md bg-slate-100 border border-slate-200 text-xs font-mono font-semibold text-slate-800">
              {liveBrain.observing
                ? 'OBSERVING_MUTATION'
                : liveBrain.baselineReady
                  ? 'BASELINE_READY'
                  : 'COLLECTING_BASELINE'}
            </span>
          </div>

          <div className="grid grid-cols-2 sm:grid-cols-4 gap-3 pt-4 font-mono tabular-nums">
            <div className="p-3.5 bg-slate-50 border border-slate-200 rounded-xl">
              <div className="text-xs text-slate-500 font-sans">Pre-Mutation Baseline</div>
              <div className="text-lg font-bold text-slate-900 mt-1">
                {liveBrain.baselineSamples} / {liveBrain.baselineTarget || 4}
              </div>
              <div className="text-[11px] text-slate-500 mt-0.5">
                {liveBrain.baselineReady ? 'Window locked' : 'Sampling baseline'}
              </div>
            </div>
            <div className="p-3.5 bg-slate-50 border border-slate-200 rounded-xl">
              <div className="text-xs text-slate-500 font-sans">Post-Mutation Observation</div>
              <div className="text-lg font-bold text-slate-900 mt-1">
                {liveBrain.observationSamples} / {liveBrain.observationTarget || 4}
              </div>
              <div className="text-[11px] text-slate-500 mt-0.5">
                {liveBrain.observing ? 'Evaluating outcome' : 'Idle'}
              </div>
            </div>
            <div className="p-3.5 bg-slate-50 border border-slate-200 rounded-xl">
              <div className="text-xs text-slate-500 font-sans">Cooldown Remaining</div>
              <div className="text-lg font-bold text-slate-900 mt-1">
                {liveBrain.cooldown} ticks
              </div>
              <div className="text-[11px] text-slate-500 mt-0.5">
                Rollback / oscillation guard
              </div>
            </div>
            <div className="p-3.5 bg-slate-50 border border-slate-200 rounded-xl">
              <div className="text-xs text-slate-500 font-sans">Permit Authority</div>
              <div className="text-lg font-bold text-slate-900 mt-1">
                {liveBrain.permitResource || liveBrain.permitCpu ? 'GRANTED' : 'HELD'}
              </div>
              <div className="text-[11px] text-slate-500 mt-0.5">
                engine={liveBrain.mode === 'adaptive' && liveBrain.armed ? 'active' : 'held'}
              </div>
            </div>
          </div>
        </section>
      )}

      {isNativeBridge ? (
        <p className="p-4 bg-white border border-slate-200 rounded-xl text-sm text-slate-600">
          Live verification runs automatically in the daemon. Manual governor experiments and thermal forecasts are available only in browser preview.
        </p>
      ) : (
        <>
          {hybridBrain && (
            <section className="cf-stagger-2 cf-card bg-white border border-slate-200 rounded-xl overflow-hidden">
              <div className="p-5 border-b border-slate-200 flex flex-col lg:flex-row lg:items-center justify-between gap-3">
                <div>
                  <h2 className="text-sm font-semibold text-slate-900">
                    On-Device Hybrid Decision Brain (Contextual UCB1-Tuned Bandit + 4D Pareto)
                  </h2>
                  <p className="text-xs text-slate-500 mt-0.5">
                    Zero cloud calls, zero hardcoded parameters: learns empirical reward posteriors per workload context and balances 4 competing hardware objectives in real time.
                  </p>
                </div>
                <div className="text-xs font-mono text-slate-600 tabular-nums">
                  Selected: <strong className="text-blue-700">{hybridBrain.selectedArmLabel}</strong> · Silicon Unlock{' '}
                  <strong className="text-emerald-700">{hybridBrain.siliconGapPct}%</strong>
                </div>
              </div>

              <div className="p-5 grid grid-cols-1 sm:grid-cols-2 lg:grid-cols-4 gap-3 border-b border-slate-100">
                <ParetoAxisBar
                  label="Compute Throughput"
                  weight={hybridBrain.paretoWeights.throughput}
                  color="#2563EB"
                />
                <ParetoAxisBar
                  label="Frame & I/O Latency"
                  weight={hybridBrain.paretoWeights.frameLatency}
                  color="#059669"
                />
                <ParetoAxisBar
                  label="Thermal Horizon Safety"
                  weight={hybridBrain.paretoWeights.thermalSafety}
                  color="#DC2626"
                />
                <ParetoAxisBar
                  label="Power Conservation"
                  weight={hybridBrain.paretoWeights.powerEfficiency}
                  color="#D97706"
                />
              </div>

              <div className="overflow-x-auto">
                <table className="w-full text-left border-collapse">
                  <thead>
                    <tr className="border-b border-slate-200 bg-slate-50/70 text-[11px] font-semibold uppercase tracking-wider text-slate-500">
                      <th className="py-2.5 px-5">Contextual Bandit Strategy Arm</th>
                      <th className="py-2.5 px-4 text-right">Pulls (n)</th>
                      <th className="py-2.5 px-4 text-right">Posterior Mean (μ)</th>
                      <th className="py-2.5 px-4 text-right">Variance (σ²)</th>
                      <th className="py-2.5 px-4 text-right">UCB Explore</th>
                      <th className="py-2.5 px-5 text-right">UCB1-Tuned Score</th>
                    </tr>
                  </thead>
                  <tbody className="divide-y divide-slate-100 text-xs">
                    {hybridBrain.arms.map((arm) => {
                      const isWinner = arm.id === hybridBrain.selectedArm;
                      return (
                        <tr
                          key={arm.id}
                          className={
                            isWinner
                              ? 'bg-blue-50/60 transition-colors'
                              : 'hover:bg-slate-50/80 transition-colors'
                          }
                        >
                          <td className="py-2.5 px-5">
                            <div className="font-medium text-slate-900">
                              {arm.label}
                              {isWinner && (
                                <span className="ml-2 font-mono text-[11px] text-blue-700 font-semibold">
                                  · ACTIVE
                                </span>
                              )}
                            </div>
                            <div className="text-[11px] text-slate-500">{arm.description}</div>
                          </td>
                          <td className="py-2.5 px-4 text-right font-mono tabular-nums text-slate-700">
                            {arm.pulls}
                          </td>
                          <td className="py-2.5 px-4 text-right font-mono tabular-nums text-slate-700">
                            {arm.meanReward.toFixed(3)}
                          </td>
                          <td className="py-2.5 px-4 text-right font-mono tabular-nums text-slate-500">
                            {arm.variance.toFixed(4)}
                          </td>
                          <td className="py-2.5 px-4 text-right font-mono tabular-nums text-emerald-700">
                            +{arm.explorationBonus.toFixed(3)}
                          </td>
                          <td className="py-2.5 px-5 text-right font-mono font-bold tabular-nums text-slate-900">
                            {arm.ucbScore.toFixed(3)}
                          </td>
                        </tr>
                      );
                    })}
                  </tbody>
                </table>
              </div>
            </section>
          )}

          <div className="cf-stagger-2 grid grid-cols-1 xl:grid-cols-12 gap-6 items-start">
          {/* Left: Governor Verification Experiment */}
          <section className="xl:col-span-5 cf-card bg-white border border-slate-200 rounded-xl p-5 space-y-5">
            <div>
              <h2 className="text-sm font-semibold text-slate-900">
                Governor Outcome Verification Loop
              </h2>
              <p className="text-xs text-slate-500 mt-1 leading-relaxed">
                Captures 4 factory baseline ticks (<span className="font-mono">{stockGov}</span>), applies candidate governor, and evaluates 4 post-mutation samples. Regressions &le; -3.0% automatically roll back via <span className="font-mono">MutationJournal</span>.
              </p>
            </div>

            <div className="space-y-3 pt-3 border-t border-slate-100">
              <div>
                <label
                  htmlFor="candidate-gov-select"
                  className="block text-xs font-medium text-slate-700 mb-1"
                >
                  Candidate CPUFreq Governor
                </label>
                <select
                  id="candidate-gov-select"
                  value={experiment.candidateGovernor}
                  disabled={
                    experiment.active &&
                    (experiment.action === BaselineAction.CollectingBaseline ||
                      experiment.action === BaselineAction.ObservingMutation)
                  }
                  onChange={(e) => onSelectGovernor(e.target.value)}
                  className="w-full px-3 py-2 text-xs font-mono font-medium text-slate-900 bg-slate-50 border border-slate-200 rounded-lg focus:outline-none focus:ring-2 focus:ring-blue-600 disabled:opacity-60"
                >
                  {availableGovernors.map((gov) => (
                    <option key={gov} value={gov}>
                      {gov} {gov === stockGov ? '(OEM Factory Stock)' : ''}
                    </option>
                  ))}
                </select>
              </div>

              <label className="flex items-center justify-between gap-3 p-3 bg-slate-50 border border-slate-200 rounded-xl cursor-pointer hover:bg-slate-100/70">
                <div>
                  <div className="text-xs font-medium text-slate-900">
                    Simulate Efficiency Regression (-16%)
                  </div>
                  <div className="text-[11px] text-slate-500">
                    Forces post-mutation degradation to test automatic OEM baseline rollback
                  </div>
                </div>
                <input
                  type="checkbox"
                  checked={experiment.injectRegression}
                  onChange={(e) => onToggleInjectRegression(e.target.checked)}
                  className="w-4 h-4 rounded border-slate-300 text-blue-600 focus:ring-blue-600 shrink-0"
                />
              </label>

              <div className="flex items-center gap-2">
                <button
                  type="button"
                  onClick={onStartExperiment}
                  className="flex-1 inline-flex items-center justify-center gap-2 px-4 py-2.5 text-xs font-semibold text-white bg-blue-600 hover:bg-blue-700 rounded-lg"
                >
                  <Play className="w-3.5 h-3.5" aria-hidden="true" />
                  <span>Run 8-Sample Verification</span>
                </button>
                <button
                  type="button"
                  onClick={onResetExperiment}
                  className="inline-flex items-center justify-center gap-1.5 px-3 py-2.5 text-xs font-semibold text-slate-700 bg-slate-100 hover:bg-slate-200 rounded-lg"
                >
                  <RotateCcw className="w-3.5 h-3.5" aria-hidden="true" />
                  <span>Reset</span>
                </button>
              </div>
            </div>

            <div className="p-4 rounded-xl bg-slate-50 border border-slate-200 space-y-3">
              <div className="flex items-center justify-between text-xs">
                <span className="text-slate-500 font-medium">Verification Progress</span>
                <span className="font-mono font-semibold text-slate-900">
                  {experiment.action}
                </span>
              </div>

              {/* Smooth 8-tick SVG progress bar */}
              <svg
                viewBox="0 0 100 5"
                preserveAspectRatio="none"
                className="w-full h-2 rounded-full overflow-hidden"
                aria-hidden="true"
              >
                <rect x={0} y={0} width={100} height={5} rx={2.5} fill="#E2E8F0" />
                <rect
                  x={0}
                  y={0}
                  width={Number(((completedVerificationTicks / 8) * 100).toFixed(1))}
                  height={5}
                  rx={2.5}
                  fill={
                    experiment.outcome === BaselineOutcome.Regression
                      ? '#DC2626'
                      : experiment.outcome === BaselineOutcome.Beneficial
                        ? '#059669'
                        : '#2563EB'
                  }
                  className="cf-svg-metric"
                />
              </svg>

              <div className="grid grid-cols-2 gap-3 pt-1 font-mono tabular-nums">
                <div className="p-2.5 bg-white border border-slate-200 rounded-lg">
                  <div className="text-[11px] font-sans text-slate-500">
                    Baseline ({experiment.baselineSamples.length}/4)
                  </div>
                  <div className="text-base font-bold text-slate-900 mt-0.5">
                    {experiment.baselineMeanScore !== null
                      ? `${experiment.baselineMeanScore.toFixed(2)} pts`
                      : '—'}
                  </div>
                </div>
                <div className="p-2.5 bg-white border border-slate-200 rounded-lg">
                  <div className="text-[11px] font-sans text-slate-500">
                    Mutated ({experiment.mutatedSamples.length}/4)
                  </div>
                  <div className="text-base font-bold text-slate-900 mt-0.5">
                    {experiment.mutatedMeanScore !== null
                      ? `${experiment.mutatedMeanScore.toFixed(2)} pts`
                      : '—'}
                  </div>
                </div>
              </div>

              {experiment.deltaPct !== null && (
                <div className="flex items-center justify-between pt-2 border-t border-slate-200 text-xs">
                  <span className="font-medium text-slate-600">Measured Outcome</span>
                  <span
                    className={`inline-flex items-center gap-1 font-mono font-bold tabular-nums ${
                      experiment.outcome === BaselineOutcome.Beneficial
                        ? 'text-emerald-700'
                        : experiment.outcome === BaselineOutcome.Regression
                          ? 'text-red-700'
                          : 'text-amber-700'
                    }`}
                  >
                    {experiment.outcome === BaselineOutcome.Beneficial && (
                      <CheckCircle2 className="w-3.5 h-3.5" aria-hidden="true" />
                    )}
                    {experiment.outcome === BaselineOutcome.Regression && (
                      <ShieldAlert className="w-3.5 h-3.5" aria-hidden="true" />
                    )}
                    {experiment.outcome !== BaselineOutcome.Beneficial &&
                      experiment.outcome !== BaselineOutcome.Regression && (
                        <AlertTriangle className="w-3.5 h-3.5" aria-hidden="true" />
                      )}
                    {experiment.outcome} ({experiment.deltaPct >= 0 ? '+' : ''}
                    {experiment.deltaPct.toFixed(2)}%)
                  </span>
                </div>
              )}

              <p className="text-xs text-slate-600 leading-relaxed">{experiment.summary}</p>
            </div>
          </section>

          {/* Right: 18-Feature ONNX Thermal Predictor Vector */}
          <section className="xl:col-span-7 cf-card bg-white border border-slate-200 rounded-xl overflow-hidden">
            <div className="p-5 border-b border-slate-200 flex flex-col sm:flex-row sm:items-center justify-between gap-4">
              <div>
                <h2 className="text-sm font-semibold text-slate-900">
                  18-Feature Thermal Predictor Contract (v1.2)
                </h2>
                <p className="text-xs text-slate-500 mt-0.5">
                  Exact feature ordering verified against <span className="font-mono">include/coreflow/thermal_features.hpp</span> · Preemptive guard at &ge; 55.0 °C
                </p>
              </div>
              <div className="flex items-center gap-5 font-mono tabular-nums">
                <div>
                  <div className="text-[11px] font-sans text-slate-500">
                    +{prediction.horizonSeconds}s Forecast
                  </div>
                  <div
                    className={`text-lg font-bold ${
                      prediction.predictedTempC >= 55
                        ? 'text-red-700'
                        : prediction.predictedTempC >= 48
                          ? 'text-amber-700'
                          : 'text-slate-900'
                    }`}
                  >
                    {prediction.predictedTempC.toFixed(2)} °C (
                    {prediction.deltaHorizonC >= 0 ? '+' : ''}
                    {prediction.deltaHorizonC.toFixed(2)} °C)
                  </div>
                </div>
                <div className="text-right">
                  <div className="text-[11px] font-sans text-slate-500">Inference Mode</div>
                  <div className="text-xs font-semibold text-slate-800 mt-0.5">
                    {prediction.predictorMode}
                  </div>
                </div>
              </div>
            </div>

            {prediction.preemptiveTrigger && (
              <div className="px-5 py-2.5 bg-amber-50 border-b border-amber-200 text-xs text-amber-900 flex items-center gap-2">
                <AlertTriangle className="w-4 h-4 shrink-0" aria-hidden="true" />
                <span>
                  Preemptive ThermalGuard horizon crossed (&ge; 55.0 °C forecast); proactive boost suppressed ahead of sensor breach.
                </span>
              </div>
            )}

            <div className="overflow-x-auto">
              <table className="w-full text-left border-collapse">
                <thead>
                  <tr className="border-b border-slate-200 bg-slate-50/70 text-[11px] font-semibold uppercase tracking-wider text-slate-500">
                    <th className="py-2.5 px-4 w-14">Idx</th>
                    <th className="py-2.5 px-4">Feature Name</th>
                    <th className="py-2.5 px-4 text-right">Tensor Float</th>
                    <th className="py-2.5 px-4 text-right">Telemetry Value</th>
                    <th className="py-2.5 px-4">C++ Source Semantics</th>
                  </tr>
                </thead>
                <tbody className="divide-y divide-slate-100 text-xs">
                  {prediction.features.map((feat) => (
                    <tr key={feat.index} className="hover:bg-slate-50/80 transition-colors">
                      <td className="py-2 px-4 font-mono text-slate-400 tabular-nums">
                        #{feat.index}
                      </td>
                      <td className="py-2 px-4 font-mono font-medium text-slate-900">
                        {feat.name}
                      </td>
                      <td className="py-2 px-4 font-mono text-right text-slate-700 tabular-nums">
                        {feat.rawValue}
                      </td>
                      <td className="py-2 px-4 font-mono font-semibold text-right text-slate-900 tabular-nums">
                        {feat.formattedValue}
                      </td>
                      <td className="py-2 px-4 text-slate-600">{feat.description}</td>
                    </tr>
                  ))}
                </tbody>
              </table>
            </div>
          </section>
        </div>
        </>
      )}
    </div>
  );
};
