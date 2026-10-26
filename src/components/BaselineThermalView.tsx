import React from 'react';
import { AlertTriangle, CheckCircle2, Play, RotateCcw } from 'lucide-react';
import {
  BaselineAction,
  BaselineExperimentState,
  BaselineOutcome,
  DeviceHardwareProfile,
  ThermalPrediction,
} from '../engine/types';

interface BaselineThermalViewProps {
  profile: DeviceHardwareProfile;
  experiment: BaselineExperimentState;
  selectedCandidate: string;
  onSelectCandidate: (gov: string) => void;
  injectRegression: boolean;
  onToggleInjectRegression: (val: boolean) => void;
  onStartExperiment: () => void;
  onResetExperiment: () => void;
  prediction: ThermalPrediction;
}

export const BaselineThermalView: React.FC<BaselineThermalViewProps> = ({
  profile,
  experiment,
  selectedCandidate,
  onSelectCandidate,
  injectRegression,
  onToggleInjectRegression,
  onStartExperiment,
  onResetExperiment,
  prediction,
}) => {
  const governors = profile.cpuPolicies[1]?.availableGovernors ?? [
    'schedutil',
    'walt',
    'conservative',
    'powersave',
    'performance',
  ];

  return (
    <div className="space-y-6">
      <div className="pb-4 border-b border-slate-200">
        <div className="text-xs text-slate-500">
          5 Baseline Samples &rarr; Verified Mutation &rarr; 5 Observation Samples &rarr; Keep / Hold / Restore
        </div>
        <h1 className="mt-1 text-2xl font-bold tracking-tight text-slate-900">
          Baseline Intelligence &amp; 14-Feature Thermal Predictor
        </h1>
      </div>

      {/* Baseline Intelligence Verification Lab */}
      <section className="bg-white border border-slate-200 rounded-xl overflow-hidden">
        <div className="p-5 border-b border-slate-200 flex flex-col lg:flex-row lg:items-center justify-between gap-4">
          <div>
            <h2 className="text-base font-semibold text-slate-900">
              Baseline Intelligence Outcome Verifier
            </h2>
            <p className="text-xs text-slate-500 mt-0.5">
              Tests whether a candidate governor improves efficiency over the factory OEM snapshot, automatically rolling back regressions.
            </p>
          </div>

          <div className="flex flex-wrap items-center gap-3">
            <select
              aria-label="Candidate CPU governor"
              value={selectedCandidate}
              onChange={(e) => onSelectCandidate(e.target.value)}
              className="px-3 py-1.5 text-xs font-mono font-medium bg-slate-50 border border-slate-200 rounded-lg text-slate-900 focus:outline-none focus:ring-2 focus:ring-blue-600"
            >
              {governors.map((gov) => (
                <option key={gov} value={gov}>
                  Candidate: {gov}
                </option>
              ))}
            </select>

            <label className="inline-flex items-center gap-2 text-xs font-medium text-slate-700 cursor-pointer whitespace-nowrap">
              <input
                type="checkbox"
                checked={injectRegression}
                onChange={(e) => onToggleInjectRegression(e.target.checked)}
                className="w-4 h-4 rounded border-slate-300 text-blue-600 focus:ring-blue-600"
              />
              <span>Simulate Regression (-16%)</span>
            </label>

            <button
              type="button"
              onClick={onStartExperiment}
              className="inline-flex items-center gap-1.5 px-3.5 py-2 text-xs font-semibold text-white bg-blue-600 rounded-lg hover:bg-blue-700 transition-colors whitespace-nowrap shrink-0"
            >
              <Play className="w-3.5 h-3.5" aria-hidden="true" />
              <span>Run 10-Sample Verification</span>
            </button>

            {(experiment.baselineSamples.length > 0 || experiment.mutatedSamples.length > 0) && (
              <button
                type="button"
                onClick={onResetExperiment}
                className="inline-flex items-center gap-1 px-3 py-2 text-xs font-medium text-slate-700 bg-slate-100 rounded-lg hover:bg-slate-200 transition-colors whitespace-nowrap shrink-0"
              >
                <RotateCcw className="w-3.5 h-3.5" aria-hidden="true" />
                <span>Clear</span>
              </button>
            )}
          </div>
        </div>

        <div className="p-5 grid grid-cols-1 md:grid-cols-3 gap-6 border-b border-slate-200 bg-slate-50/50">
          <div>
            <div className="text-xs text-slate-500">Factory Baseline Window (5 Ticks)</div>
            <div className="mt-1 text-xl font-bold font-mono text-slate-900 tabular-nums">
              {experiment.baselineMeanScore !== null
                ? `${experiment.baselineMeanScore.toFixed(2)} pts`
                : `${experiment.baselineSamples.length} / 5 samples`}
            </div>
            <div className="mt-1 text-xs text-slate-500 font-mono">
              OEM Stock: {profile.cpuPolicies[1]?.stockGovernor ?? 'schedutil'}
            </div>
          </div>

          <div>
            <div className="text-xs text-slate-500">Post-Mutation Window (5 Ticks)</div>
            <div className="mt-1 text-xl font-bold font-mono text-slate-900 tabular-nums">
              {experiment.mutatedMeanScore !== null
                ? `${experiment.mutatedMeanScore.toFixed(2)} pts`
                : `${experiment.mutatedSamples.length} / 5 samples`}
            </div>
            <div className="mt-1 text-xs text-slate-500 font-mono">
              Candidate: {experiment.candidateGovernor}
              {experiment.deltaPct !== null && (
                <span>
                  {' '}
                  · {experiment.deltaPct >= 0 ? '+' : ''}
                  {experiment.deltaPct.toFixed(2)}%
                </span>
              )}
            </div>
          </div>

          <div>
            <div className="text-xs text-slate-500">Verification Verdict &amp; Action</div>
            <div className="mt-1 flex items-center gap-2 text-xl font-bold font-mono">
              {experiment.outcome === BaselineOutcome.Beneficial && (
                <span className="inline-flex items-center gap-1.5 text-emerald-700">
                  <CheckCircle2 className="w-5 h-5" aria-hidden="true" />
                  BENEFICIAL &rarr; KEEP
                </span>
              )}
              {experiment.outcome === BaselineOutcome.Regression && (
                <span className="inline-flex items-center gap-1.5 text-red-700">
                  <AlertTriangle className="w-5 h-5" aria-hidden="true" />
                  REGRESSION &rarr; RESTORE
                </span>
              )}
              {(experiment.outcome === BaselineOutcome.Neutral ||
                experiment.outcome === BaselineOutcome.Inconclusive) && (
                <span className="text-amber-700">
                  {experiment.outcome} &rarr; HOLD
                </span>
              )}
              {experiment.outcome === BaselineOutcome.Pending && (
                <span className="text-slate-700">{experiment.action}</span>
              )}
            </div>
            <div className="mt-1 text-xs text-slate-600">{experiment.summary}</div>
          </div>
        </div>

        {/* Sample Comparison Row */}
        <div className="grid grid-cols-1 md:grid-cols-2 divide-y md:divide-y-0 md:divide-x divide-slate-200 text-xs">
          <div className="p-5">
            <div className="font-semibold text-slate-900 mb-2">
              Factory Baseline Samples
            </div>
            {experiment.baselineSamples.length === 0 ? (
              <p className="text-slate-500">
                Click &ldquo;Run 10-Sample Verification&rdquo; (and Step Tick or Resume Loop) to capture 5 OEM baseline samples.
              </p>
            ) : (
              <div className="space-y-1.5 font-mono tabular-nums">
                {experiment.baselineSamples.map((s, i) => (
                  <div key={`base-${i}-${s.tick}`} className="flex justify-between text-slate-700">
                    <span>Sample #{i + 1} (tick {s.tick})</span>
                    <span>
                      {s.tempC.toFixed(1)}°C · CPU {s.cpuUsagePct.toFixed(0)}% ·{' '}
                      <strong className="text-slate-900">{s.efficiencyScore.toFixed(2)}</strong>
                    </span>
                  </div>
                ))}
              </div>
            )}
          </div>

          <div className="p-5">
            <div className="font-semibold text-slate-900 mb-2">
              Verified Mutation Samples ({experiment.candidateGovernor})
            </div>
            {experiment.mutatedSamples.length === 0 ? (
              <p className="text-slate-500">
                {experiment.action === BaselineAction.CollectingBaseline && experiment.active
                  ? 'Waiting for 5 baseline samples to finish before mutating governor...'
                  : 'No post-mutation samples recorded yet.'}
              </p>
            ) : (
              <div className="space-y-1.5 font-mono tabular-nums">
                {experiment.mutatedSamples.map((s, i) => (
                  <div key={`mut-${i}-${s.tick}`} className="flex justify-between text-slate-700">
                    <span>Sample #{i + 1} (tick {s.tick})</span>
                    <span>
                      {s.tempC.toFixed(1)}°C · CPU {s.cpuUsagePct.toFixed(0)}% ·{' '}
                      <strong className="text-slate-900">{s.efficiencyScore.toFixed(2)}</strong>
                    </span>
                  </div>
                ))}
              </div>
            )}
          </div>
        </div>
      </section>

      {/* 14-Feature Predictive Thermal Contract */}
      <section className="bg-white border border-slate-200 rounded-xl overflow-hidden">
        <div className="p-5 border-b border-slate-200 flex flex-col sm:flex-row sm:items-center justify-between gap-3">
          <div>
            <h2 className="text-base font-semibold text-slate-900">
              Thermal Predictor — 14-Feature Synchronized Contract
            </h2>
            <p className="text-xs text-slate-500 mt-0.5">
              Input tensor schema shared between the Python ONNX trainer and C++ heuristic runtime predictor.
            </p>
          </div>
          <div className="text-right font-mono text-xs tabular-nums">
            <div className="font-semibold text-slate-900">
              {prediction.horizonTicks}-Tick ({prediction.horizonSeconds}s) Forecast:{' '}
              <span className={prediction.predictedTempC >= 43 ? 'text-red-700' : 'text-slate-900'}>
                {prediction.predictedTempC.toFixed(2)} °C
              </span>{' '}
              ({prediction.deltaHorizonC >= 0 ? '+' : ''}
              {prediction.deltaHorizonC.toFixed(2)} °C)
            </div>
            <div className="text-slate-500">
              Preemptive Guard Trigger: {prediction.preemptiveTrigger ? 'ACTIVE' : 'INACTIVE'}
            </div>
          </div>
        </div>

        <div className="overflow-x-auto">
          <table className="w-full text-left border-collapse">
            <thead>
              <tr className="border-b border-slate-200 bg-slate-50 text-xs font-medium text-slate-600">
                <th className="py-2.5 px-5 w-16">Index</th>
                <th className="py-2.5 px-4">Feature Identifier</th>
                <th className="py-2.5 px-4 text-right">Tensor Float</th>
                <th className="py-2.5 px-4 text-right">Decoded Telemetry</th>
                <th className="py-2.5 px-5">Contract Specification</th>
              </tr>
            </thead>
            <tbody className="divide-y divide-slate-200 text-xs">
              {prediction.features.map((feat) => (
                <tr key={feat.index} className="hover:bg-slate-50/80 transition-colors">
                  <td className="py-2.5 px-5 font-mono text-slate-500 tabular-nums">
                    #{feat.index}
                  </td>
                  <td className="py-2.5 px-4 font-mono font-medium text-slate-900">
                    {feat.name}
                  </td>
                  <td className="py-2.5 px-4 text-right font-mono text-blue-700 font-medium tabular-nums">
                    {feat.rawValue}
                  </td>
                  <td className="py-2.5 px-4 text-right font-mono text-slate-900 tabular-nums">
                    {feat.formattedValue}
                  </td>
                  <td className="py-2.5 px-5 text-slate-600">{feat.description}</td>
                </tr>
              ))}
            </tbody>
          </table>
        </div>
      </section>
    </div>
  );
};
