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
  const [feedback, setFeedback] = useState<{ kind: 'ok' | 'err'; text: string } | null>(null);
  const [saving, setSaving] = useState(false);

  const handleSaveClick = async (confirmed = false) => {
    setSaving(true);
    setFeedback(null);
    try {
      const res = await onCommitConfig(confirmed);
      if (res.needsConfirm) {
        setFeedback({ kind: 'err', text: 'Use Start in Controls to resume the stopped engine.' });
      } else if (res.error) {
        setFeedback({ kind: 'err', text: res.error });
      } else if (res.saved) {
        setFeedback({
          kind: 'ok',
          text: 'Settings saved. Restart the engine to apply.',
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
            Operational settings · applied after restarting the engine
          </div>
          <h1 className="mt-1 text-2xl font-bold tracking-tight text-slate-900">
            Engine Settings
          </h1>
        </div>
        <div className="text-xs font-mono text-slate-600">
          Engine: <strong className="text-slate-900">{currentDaemonMode === 'adaptive' ? 'Active' : currentDaemonMode === 'stopped' ? 'Stopped' : currentDaemonMode === 'unknown' ? 'Awaiting status' : 'Held — see runtime reason'}</strong>
        </div>
      </div>

      {probe?.safe && (
        <div className="p-4 bg-red-50 border border-red-200 rounded-xl flex items-start gap-3 text-xs text-red-800">
          <ShieldAlert className="w-4 h-4 shrink-0 mt-0.5 text-red-700" aria-hidden="true" />
          <div>
            <div className="font-semibold">Recovery protection is active</div>
            <p className="mt-0.5 text-red-700">
              Bootloop / crash recovery protection is active. New tuning is paused by recovery protection. Saving settings does not clear this marker.
            </p>
          </div>
        </div>
      )}

      <section className="bg-white border border-slate-200 rounded-xl p-5 space-y-4">
        <div>
          <h2 className="text-base font-semibold text-slate-900">Automatic optimization</h2>
          <p className="text-xs text-slate-500 mt-0.5">
            CoreFlow starts after boot and applies justified, bounded changes automatically.
            Discovery, outcome measurement, thermal protection and rollback remain active.
            Use Controls to start, stop or restart the daemon.
          </p>
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
              Restart Engine
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
