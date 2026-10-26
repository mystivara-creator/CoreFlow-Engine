import React, { useState } from 'react';
import { AlertTriangle, CheckCircle2, Save, ShieldAlert } from 'lucide-react';
import { DaemonProbe, NumericFieldSpec, PolicyFormValues } from '../bridge/daemonBridge';

interface PolicyConfigViewProps {
  form: PolicyFormValues;
  numericSpecs: NumericFieldSpec[];
  probe: DaemonProbe | null;
  currentDaemonMode: string;
  onUpdateForm: (next: PolicyFormValues) => void;
  onCommitConfig: (confirmedGate?: boolean) => Promise<{
    needsConfirm?: 'resume' | 'arm' | 'observe';
    error?: string;
    saved?: boolean;
  }>;
  onRestartDaemon: () => void;
}

export const PolicyConfigView: React.FC<PolicyConfigViewProps> = ({
  form,
  numericSpecs,
  probe,
  currentDaemonMode,
  onUpdateForm,
  onCommitConfig,
  onRestartDaemon,
}) => {
  const [pendingConfirm, setPendingConfirm] = useState<'resume' | 'arm' | 'observe' | null>(null);
  const [feedback, setFeedback] = useState<{ kind: 'ok' | 'err'; text: string } | null>(null);
  const [saving, setSaving] = useState(false);

  const handleSaveClick = async (confirmed = false) => {
    setSaving(true);
    setFeedback(null);
    try {
      const res = await onCommitConfig(confirmed);
      if (res.needsConfirm) {
        setPendingConfirm(res.needsConfirm);
      } else if (res.error) {
        setPendingConfirm(null);
        setFeedback({ kind: 'err', text: res.error });
      } else if (res.saved) {
        setPendingConfirm(null);
        setFeedback({
          kind: 'ok',
          text: 'Committed to /data/adb/coreflow/config.ini (CONFIG_COMMIT_OK). Restart coreflowd to apply.',
        });
      }
    } finally {
      setSaving(false);
    }
  };

  const runtimeSpecs = numericSpecs.filter((f) => f.group === 'runtime');
  const interactivitySpecs = numericSpecs.filter((f) => f.group === 'interactivity');

  return (
    <div className="space-y-6">
      <div className="pb-4 border-b border-slate-200 flex flex-col sm:flex-row sm:items-end justify-between gap-3">
        <div>
          <div className="text-xs font-mono text-slate-500">
            /data/adb/coreflow/config.ini · Validated via CoreFlowControl.validate &amp; gate
          </div>
          <h1 className="mt-1 text-2xl font-bold tracking-tight text-slate-900">
            Daemon Policy &amp; Authorization Gate
          </h1>
        </div>
        <div className="text-xs font-mono text-slate-600">
          Active Engine Mode: <strong className="text-slate-900">{currentDaemonMode}</strong>
        </div>
      </div>

      {probe?.safe && (
        <div className="p-4 bg-red-50 border border-red-200 rounded-xl flex items-start gap-3 text-xs text-red-800">
          <ShieldAlert className="w-4 h-4 shrink-0 mt-0.5 text-red-700" aria-hidden="true" />
          <div>
            <div className="font-semibold">SAFE_MODE Marker Present (/data/adb/coreflow/SAFE_MODE)</div>
            <p className="mt-0.5 text-red-700">
              Bootloop / crash recovery protection is active. Switching to Adaptive mode is blocked by CoreFlowControl.gate() until SAFE_MODE is cleared.
            </p>
          </div>
        </div>
      )}

      {/* Mutation Authority Mode Selector */}
      <section className="bg-white border border-slate-200 rounded-xl p-5 space-y-4">
        <div>
          <h2 className="text-base font-semibold text-slate-900">
            Global Kernel Mutation Authority
          </h2>
          <p className="text-xs text-slate-500 mt-0.5">
            CoreFlow ships Observe-only by default. Adaptive mode sets mutation_armed=true and removes the DISABLE kill-switch only after explicit confirmation.
          </p>
        </div>

        <div className="grid grid-cols-1 md:grid-cols-2 gap-4">
          <button
            type="button"
            onClick={() => onUpdateForm({ ...form, mutation_mode: 'observe' })}
            className={`p-4 text-left rounded-xl border transition-colors ${
              form.mutation_mode === 'observe'
                ? 'border-blue-600 bg-blue-50/50'
                : 'border-slate-200 bg-white hover:bg-slate-50'
            }`}
          >
            <div className="text-sm font-semibold text-slate-900">
              Observe Mode (Read-Only Default)
            </div>
            <p className="mt-1 text-xs text-slate-600">
              Runs full 7-layer observation, 18-feature ONNX thermal prediction, and candidate scoring without writing any /sys or /proc tunable. Touches /data/adb/coreflow/DISABLE.
            </p>
          </button>

          <button
            type="button"
            onClick={() =>
              onUpdateForm({
                ...form,
                mutation_mode: 'adaptive',
                interactivity_mode: 'adaptive',
                runtime_refresh: true,
              })
            }
            className={`p-4 text-left rounded-xl border transition-colors ${
              form.mutation_mode === 'adaptive'
                ? 'border-blue-600 bg-blue-50/50'
                : 'border-slate-200 bg-white hover:bg-slate-50'
            }`}
          >
            <div className="text-sm font-semibold text-slate-900">
              Adaptive Mode (Armed &amp; Coupled)
            </div>
            <p className="mt-1 text-xs text-slate-600">
              Enables multi-domain StrategyCatalog coupling, GPU/KGSL/Vulkan headroom, interactivity boost, journaled writes, and read-back verification.
            </p>
          </button>
        </div>

        <div className="pt-3 border-t border-slate-100 grid grid-cols-1 sm:grid-cols-2 gap-4">
          <label className="flex items-center justify-between gap-3 p-3 bg-slate-50 border border-slate-200 rounded-lg cursor-pointer">
            <div>
              <div className="text-xs font-semibold text-slate-900">Runtime Capability Refresh</div>
              <div className="text-xs text-slate-500">Allow SIGUSR1 &amp; periodic re-validation</div>
            </div>
            <input
              type="checkbox"
              checked={Boolean(form.runtime_refresh)}
              onChange={(e) => onUpdateForm({ ...form, runtime_refresh: e.target.checked })}
              className="w-4 h-4 rounded border-slate-300 text-blue-600 focus:ring-blue-600"
            />
          </label>

          <label className="flex items-center justify-between gap-3 p-3 bg-slate-50 border border-slate-200 rounded-lg cursor-pointer">
            <div>
              <div className="text-xs font-semibold text-slate-900">
                Interactivity Boost Pipeline
              </div>
              <div className="text-xs text-slate-500">
                Bounded app-launch &amp; frame-jank uclamp/I/O boost
              </div>
            </div>
            <input
              type="checkbox"
              checked={form.interactivity_mode === 'adaptive'}
              onChange={(e) =>
                onUpdateForm({
                  ...form,
                  interactivity_mode: e.target.checked ? 'adaptive' : 'observe',
                })
              }
              className="w-4 h-4 rounded border-slate-300 text-blue-600 focus:ring-blue-600"
            />
          </label>
        </div>
      </section>

      {/* Validated EngineConfig Numeric Tunables */}
      <div className="grid grid-cols-1 lg:grid-cols-2 gap-6">
        <section className="bg-white border border-slate-200 rounded-xl p-5 space-y-4">
          <h2 className="text-base font-semibold text-slate-900">
            Autonomous Cadence &amp; Confidence Gates
          </h2>
          <div className="space-y-3">
            {runtimeSpecs.map((spec) => (
              <div key={spec.key} className="space-y-1">
                <div className="flex justify-between text-xs">
                  <label htmlFor={`cfg-${spec.key}`} className="font-medium text-slate-800">
                    {spec.label}{' '}
                    <span className="font-mono text-slate-400">({spec.key})</span>
                  </label>
                  <span className="font-mono text-slate-500 tabular-nums">
                    min {spec.min}
                    {spec.max != null ? ` · max ${spec.max}` : ''} {spec.unit}
                  </span>
                </div>
                <input
                  id={`cfg-${spec.key}`}
                  type="text"
                  inputMode="decimal"
                  value={String(form[spec.key] ?? '')}
                  onChange={(e) =>
                    onUpdateForm({ ...form, [spec.key]: e.target.value })
                  }
                  className="w-full px-3 py-1.5 text-xs font-mono bg-slate-50 border border-slate-200 rounded-lg text-slate-900 focus:outline-none focus:ring-2 focus:ring-blue-600 tabular-nums"
                />
                <p className="text-xs text-slate-500">{spec.help}</p>
              </div>
            ))}
          </div>
        </section>

        <section className="bg-white border border-slate-200 rounded-xl p-5 space-y-4">
          <h2 className="text-base font-semibold text-slate-900">
            Interactivity &amp; Frame Pacing Bounds
          </h2>
          <div className="grid grid-cols-1 sm:grid-cols-2 gap-3">
            {interactivitySpecs.map((spec) => (
              <div key={spec.key} className="space-y-1">
                <label
                  htmlFor={`cfg-${spec.key}`}
                  className="block text-xs font-medium text-slate-800 truncate"
                >
                  {spec.label} {spec.unit ? `(${spec.unit})` : ''}
                </label>
                <input
                  id={`cfg-${spec.key}`}
                  type="text"
                  inputMode="decimal"
                  value={String(form[spec.key] ?? '')}
                  onChange={(e) =>
                    onUpdateForm({ ...form, [spec.key]: e.target.value })
                  }
                  className="w-full px-3 py-1.5 text-xs font-mono bg-slate-50 border border-slate-200 rounded-lg text-slate-900 focus:outline-none focus:ring-2 focus:ring-blue-600 tabular-nums"
                />
              </div>
            ))}
          </div>
        </section>
      </div>

      {/* Inline Gate Confirmation Banner (no window.confirm) */}
      {pendingConfirm && (
        <div className="p-4 bg-amber-50 border border-amber-300 rounded-xl flex flex-col sm:flex-row sm:items-center justify-between gap-4">
          <div className="flex items-start gap-2.5 text-xs text-amber-900">
            <AlertTriangle className="w-4 h-4 shrink-0 mt-0.5 text-amber-700" aria-hidden="true" />
            <div>
              <div className="font-semibold">Authorization Gate Confirmation Required</div>
              <p className="mt-0.5">
                {pendingConfirm === 'resume'
                  ? 'Adaptive mode will clear /data/adb/coreflow/DISABLE and arm bounded kernel mutations upon restart. Confirm commit?'
                  : pendingConfirm === 'arm'
                    ? 'Enabling Adaptive mode sets mutation_armed=true in config.ini. Confirm arming?'
                    : 'Switching to Observe mode sets mutation_armed=false and creates the DISABLE kill-switch. Confirm?'}
              </p>
            </div>
          </div>
          <div className="flex items-center gap-2 shrink-0">
            <button
              type="button"
              onClick={() => setPendingConfirm(null)}
              className="px-3 py-1.5 text-xs font-medium text-slate-700 bg-white border border-slate-300 rounded-lg hover:bg-slate-50"
            >
              Cancel
            </button>
            <button
              type="button"
              onClick={() => handleSaveClick(true)}
              className="px-3.5 py-1.5 text-xs font-semibold text-white bg-amber-700 rounded-lg hover:bg-amber-800"
            >
              Confirm &amp; Write config.ini
            </button>
          </div>
        </div>
      )}

      {feedback && (
        <div
          className={`p-4 rounded-xl border flex flex-col sm:flex-row sm:items-center justify-between gap-3 text-xs ${
            feedback.kind === 'ok'
              ? 'bg-emerald-50 border-emerald-200 text-emerald-900'
              : 'bg-red-50 border-red-200 text-red-900'
          }`}
        >
          <div className="flex items-center gap-2">
            {feedback.kind === 'ok' ? (
              <CheckCircle2 className="w-4 h-4 text-emerald-700 shrink-0" aria-hidden="true" />
            ) : (
              <AlertTriangle className="w-4 h-4 text-red-700 shrink-0" aria-hidden="true" />
            )}
            <span>{feedback.text}</span>
          </div>
          {feedback.kind === 'ok' && (
            <button
              type="button"
              onClick={onRestartDaemon}
              className="px-3 py-1.5 text-xs font-semibold text-white bg-emerald-700 rounded-lg hover:bg-emerald-800 whitespace-nowrap shrink-0"
            >
              Restart coreflowd Now
            </button>
          )}
        </div>
      )}

      <div className="flex items-center justify-end gap-3">
        <button
          id="btn-apply-mode"
          type="button"
          disabled={saving}
          onClick={() => handleSaveClick(false)}
          className="inline-flex items-center gap-2 px-4 py-2 text-xs font-semibold text-white bg-blue-600 rounded-lg hover:bg-blue-700 disabled:opacity-50 transition-colors"
        >
          <Save className="w-3.5 h-3.5" aria-hidden="true" />
          <span>Validate &amp; Save config.ini</span>
        </button>
      </div>
    </div>
  );
};
