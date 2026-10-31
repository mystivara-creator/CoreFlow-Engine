import React, { useState } from 'react';
import { AlertTriangle, CheckCircle2, RotateCcw, Save, ShieldAlert } from 'lucide-react';
import { DaemonProbe, NumericFieldSpec, PolicyFormValues } from '../bridge/daemonBridge';
import { SETTINGS_GUIDE, settingsPreset } from '../engine/settingsGuide';
import { AppearanceControls } from './AppearanceControls';

interface PolicyConfigViewProps {
  form: PolicyFormValues;
  savedForm?: PolicyFormValues;
  numericSpecs: NumericFieldSpec[];
  probe: DaemonProbe | null;
  currentDaemonMode: string;
  onUpdateForm: (next: PolicyFormValues) => void;
  onCommitConfig: (
    confirmedGate?: boolean
  ) => Promise<{ needsConfirm?: 'resume' | 'arm' | 'observe'; error?: string; saved?: boolean }>;
  onRestartDaemon: () => void | Promise<void>;
}

export const PolicyConfigView: React.FC<PolicyConfigViewProps> = ({
  form,
  savedForm,
  numericSpecs,
  probe,
  currentDaemonMode,
  onUpdateForm,
  onCommitConfig,
  onRestartDaemon,
}) => {
  const [feedback, setFeedback] = useState<{ kind: 'ok' | 'err'; text: string } | null>(null);
  const [saving, setSaving] = useState(false);

  const C = window.CoreFlowControl;
  const defaults = C?.formFromConfig(C.DEFAULTS);
  const validation = C?.validate(form);
  const changes = Object.keys(form).filter(
    (key) => savedForm && String(form[key]) !== String(savedForm[key])
  );

  const save = async () => {
    setSaving(true);
    setFeedback(null);
    try {
      const result = await onCommitConfig(false);
      if (result.error || result.needsConfirm) {
        setFeedback({
          kind: 'err',
          text:
            result.error || 'Gunakan Start di Controls untuk melanjutkan engine yang dihentikan.',
        });
      } else if (result.saved) {
        setFeedback({
          kind: 'ok',
          text: 'Tersimpan ke disk. Restart engine untuk memakai pengaturan ini; memori pengalaman tetap disimpan.',
        });
      }
    } catch (err) {
      setFeedback({ kind: 'err', text: err instanceof Error ? err.message : String(err) });
    } finally {
      setSaving(false);
    }
  };

  const field = (spec: NumericFieldSpec) => {
    const guide = SETTINGS_GUIDE[spec.key];
    const error = validation?.errors[spec.key];
    const changed = savedForm && String(form[spec.key]) !== String(savedForm[spec.key]);
    return (
      <div
        key={spec.key}
        className={`p-4 rounded-xl border space-y-2 transition-colors ${
          error
            ? 'bg-red-50/40 border-red-300'
            : changed
              ? 'bg-blue-50/40 border-blue-300'
              : 'bg-slate-50 border-slate-200'
        }`}
      >
        <div className="flex items-center justify-between gap-2">
          <label htmlFor={`cfg-${spec.key}`} className="block text-sm font-semibold text-slate-900">
            {guide?.label || spec.label}
            {spec.unit ? ` (${spec.unit})` : ''}
          </label>
          {changed && (
            <span className="px-2 py-0.5 text-[10px] font-mono font-semibold text-blue-800 bg-blue-50 border border-blue-200 rounded">
              DRAF
            </span>
          )}
        </div>
        <p id={`help-${spec.key}`} className="text-xs leading-relaxed text-slate-600">
          {guide?.purpose || spec.help}
        </p>
        <input
          id={`cfg-${spec.key}`}
          type="text"
          inputMode={spec.kind === 'int' ? 'numeric' : 'decimal'}
          aria-invalid={!!error}
          aria-describedby={`help-${spec.key}${error ? ` error-${spec.key}` : ''}`}
          value={String(form[spec.key] ?? '')}
          disabled={saving}
          onChange={(e) => onUpdateForm({ ...form, [spec.key]: e.target.value })}
          className={`w-full px-3.5 py-2.5 text-sm font-mono tabular-nums bg-white border rounded-xl text-slate-900 focus:outline-none focus:ring-2 focus:ring-blue-600 ${
            error ? 'border-red-500' : 'border-slate-200'
          }`}
        />
        {error && (
          <p id={`error-${spec.key}`} className="text-xs font-medium text-red-700" role="alert">
            {error}
          </p>
        )}
        <p className="text-xs text-slate-500">{guide?.tradeoff}</p>
        <details className="text-xs text-slate-500 pt-1">
          <summary className="cursor-pointer font-medium text-slate-600 hover:text-slate-900">
            Nilai awal &amp; detail kontrak
          </summary>
          <div className="mt-2 p-2.5 bg-white border border-slate-200 rounded-lg space-y-1 tabular-nums">
            <p>
              Awal: <strong>{String(defaults?.[spec.key] ?? '—')}</strong> · Rentang: {spec.min}
              {spec.max == null ? ' ke atas' : `–${spec.max}`} {spec.unit}
            </p>
            <p className="font-mono text-[11px] text-slate-400 break-all">{spec.key}</p>
          </div>
        </details>
      </div>
    );
  };

  return (
    <div className="space-y-6">
      <div className="pb-4 border-b border-slate-200 flex flex-col sm:flex-row sm:items-end justify-between gap-3">
        <div className="space-y-1">
          <div className="text-xs text-slate-500">Atur draf &rarr; periksa &rarr; simpan &rarr; restart engine</div>
          <h1 className="text-2xl font-bold tracking-tight text-slate-900">Pengaturan engine</h1>
          <p className="text-sm text-slate-600">
            Engine:{' '}
            <strong>
              {currentDaemonMode === 'adaptive'
                ? 'Aktif'
                : currentDaemonMode === 'stopped'
                  ? 'Berhenti'
                  : currentDaemonMode === 'unknown'
                    ? 'Menunggu data'
                    : 'Ditahan — lihat alasan runtime'}
            </strong>
            . Mengubah isian belum mengubah perilaku perangkat sebelum disimpan &amp; direstart.
          </p>
        </div>
        {savedForm && (
          <div className="text-xs font-mono tabular-nums px-3 py-1.5 rounded-lg bg-white border border-slate-200 text-slate-600 self-start sm:self-auto">
            {changes.length} perubahan draf
          </div>
        )}
      </div>

      {probe?.safe && (
        <div
          role="alert"
          className="p-4 bg-red-50 border border-red-200 rounded-xl text-xs text-red-800 flex gap-3"
        >
          <ShieldAlert className="w-4 h-4 shrink-0 mt-0.5" />
          <p>
            Proteksi pemulihan aktif. Menyimpan pengaturan tidak menghapus SAFE_MODE atau membatalkan kewajiban rollback.
          </p>
        </div>
      )}

      <section className="cf-stagger-1 cf-card bg-white border border-slate-200 rounded-xl p-5 space-y-4">
        <div>
          <h2 className="text-sm font-semibold text-slate-900">Mulai dari pengaturan yang dipahami</h2>
          <p className="text-xs text-slate-500 mt-0.5">
            Pilihan ini hanya mengisi draf; bukan jaminan performa atau suhu tertentu.
          </p>
        </div>
        <div className="flex flex-wrap gap-2">
          {(
            [
              ['balanced', 'Seimbang'],
              ['quiet', 'Lebih tenang'],
              ['responsive', 'Pengamatan lebih cepat'],
            ] as const
          ).map(([id, label]) => (
            <button
              type="button"
              key={id}
              disabled={!defaults || saving}
              onClick={() => {
                if (defaults) onUpdateForm(settingsPreset(id, defaults));
                setFeedback(null);
              }}
              className="px-4 py-2 text-xs font-semibold rounded-xl bg-slate-50 border border-slate-200 text-slate-700 hover:bg-slate-100 hover:text-slate-900 disabled:opacity-50"
            >
              {label}
            </button>
          ))}
        </div>
        <p className="text-xs text-slate-500 leading-relaxed">
          Seimbang memakai nilai awal. Lebih tenang: interval 8 detik, kepercayaan 70%, bantuan interaktivitas dimatikan. Pengamatan lebih cepat: interval 2 detik dan kepercayaan 65%; overhead pembacaan bisa naik.
        </p>
      </section>

      <section className="cf-stagger-2 cf-card bg-white border border-slate-200 rounded-xl p-5 space-y-4">
        <div>
          <h2 className="text-sm font-semibold text-slate-900">Cara engine bekerja</h2>
          <p className="text-xs text-slate-600 leading-relaxed mt-1">
            CoreFlow mengamati sistem (termasuk postur ZRAM OEM &amp; histeresis suhu SoC), memilih perubahan terukur, memeriksa izin, lalu mengevaluasi hasil. Nilai yang terbukti membantu dapat dipertahankan; regresi dipulihkan otomatis via <span className="font-mono">MutationJournal</span>.
          </p>
        </div>
        <div className="grid sm:grid-cols-2 gap-3">
          <label className="flex items-start justify-between gap-4 p-4 bg-slate-50 rounded-xl border border-slate-200 text-sm cursor-pointer hover:bg-slate-100/70">
            <span className="font-medium text-slate-900">
              Pembaruan kapabilitas
              <span className="block text-xs font-normal text-slate-500 mt-1.5 leading-relaxed">
                Izinkan engine memeriksa ulang resource saat diperlukan. Umumnya biarkan aktif; bukan perintah untuk menulis semua node.
              </span>
            </span>
            <input
              type="checkbox"
              checked={Boolean(form.runtime_refresh)}
              disabled={saving}
              onChange={(e) => onUpdateForm({ ...form, runtime_refresh: e.target.checked })}
              className="w-4 h-4 mt-0.5 rounded border-slate-300 text-blue-600 focus:ring-blue-600 shrink-0"
            />
          </label>
          <label className="flex items-start justify-between gap-4 p-4 bg-slate-50 rounded-xl border border-slate-200 text-sm cursor-pointer hover:bg-slate-100/70">
            <span className="font-medium text-slate-900">
              Bantuan interaktivitas
              <span className="block text-xs font-normal text-slate-500 mt-1.5 leading-relaxed">
                Bantuan sementara untuk launch/frame saat ada bukti tekanan yang sesuai. Mematikannya tetap membiarkan optimasi utama bekerja; kapabilitas dan proteksi suhu selalu diperiksa.
              </span>
            </span>
            <input
              type="checkbox"
              checked={form.interactivity_mode === 'adaptive'}
              disabled={saving}
              onChange={(e) =>
                onUpdateForm({
                  ...form,
                  interactivity_mode: e.target.checked ? 'adaptive' : 'observe',
                })
              }
              className="w-4 h-4 mt-0.5 rounded border-slate-300 text-blue-600 focus:ring-blue-600 shrink-0"
            />
          </label>
        </div>
      </section>

      <section className="cf-stagger-3 cf-card bg-white border border-slate-200 rounded-xl p-5 space-y-4">
        <div>
          <h2 className="text-sm font-semibold text-slate-900">Interval &amp; kepercayaan data</h2>
          <p className="text-xs text-slate-500 mt-0.5">
            Seberapa sering engine memeriksa kondisi dan seberapa yakin kelengkapan telemetri yang disyaratkan.
          </p>
        </div>
        <div className="grid sm:grid-cols-2 gap-4">
          {numericSpecs.filter((s) => s.group === 'runtime').map(field)}
        </div>
      </section>

      <section className="cf-stagger-3 cf-card bg-white border border-slate-200 rounded-xl p-5 space-y-4">
        <div>
          <h2 className="text-sm font-semibold text-slate-900">Respons aplikasi &amp; batas boost</h2>
          <p className="text-xs text-slate-500 mt-0.5">
            {form.interactivity_mode === 'adaptive'
              ? 'Batas bantuan sementara. Nilai hanya berpengaruh pada resource yang didukung dan diizinkan.'
              : 'Bantuan interaktivitas dimatikan. Nilai di bawah disimpan untuk saat bantuan diaktifkan lagi.'}
          </p>
        </div>
        <div className="grid sm:grid-cols-2 gap-4">
          {numericSpecs.filter((s) => s.group === 'interactivity').map(field)}
        </div>
      </section>

      <section className="cf-stagger-4 cf-card bg-white border border-slate-200 rounded-xl p-5 space-y-4">
        <div className="flex flex-wrap items-center justify-between gap-3">
          <h2 className="text-sm font-semibold text-slate-900">
            Periksa draf {savedForm ? `(${changes.length} perubahan)` : ''}
          </h2>
          <button
            type="button"
            disabled={saving || !savedForm || changes.length === 0}
            onClick={() => savedForm && onUpdateForm({ ...savedForm })}
            className="inline-flex items-center gap-1.5 text-xs font-semibold text-blue-700 hover:text-blue-900 disabled:opacity-50"
          >
            <RotateCcw className="w-3.5 h-3.5" aria-hidden="true" />
            <span>Batalkan perubahan draf</span>
          </button>
        </div>

        {changes.length ? (
          <dl className="space-y-2 text-xs bg-slate-50 border border-slate-200 rounded-xl p-3.5">
            {changes.map((key) => (
              <div
                key={key}
                className="flex flex-wrap justify-between gap-2 border-b border-slate-200/70 last:border-b-0 pb-2 last:pb-0"
              >
                <dt className="font-medium text-slate-700">
                  {SETTINGS_GUIDE[key]?.label ||
                    (key === 'runtime_refresh'
                      ? 'Pembaruan kapabilitas'
                      : key === 'interactivity_mode'
                        ? 'Bantuan interaktivitas'
                        : key)}
                </dt>
                <dd className="font-mono font-semibold text-slate-900 tabular-nums">
                  {String(savedForm?.[key])} &rarr; {String(form[key])}
                </dd>
              </div>
            ))}
          </dl>
        ) : (
          <p className="text-xs text-slate-500">
            Belum ada perubahan dari pengaturan yang dibaca terakhir.
          </p>
        )}

        <p className="text-xs text-slate-500">
          Simpan menulis konfigurasi ke disk. Restart menerapkannya dan mengambil baseline baru; tidak menghapus pengalaman terverifikasi.
        </p>

        <button
          type="button"
          id="btn-apply-mode"
          disabled={saving || !validation?.ok || (savedForm && !changes.length)}
          onClick={save}
          className="inline-flex items-center justify-center gap-2 px-5 py-2.5 rounded-xl bg-blue-600 hover:bg-blue-700 text-white font-semibold text-sm disabled:opacity-50"
        >
          <Save className="w-4 h-4" />
          {saving ? 'Menyimpan…' : 'Periksa & simpan'}
        </button>
      </section>

      {feedback && (
        <div
          role={feedback.kind === 'err' ? 'alert' : 'status'}
          className={`cf-view-stage p-4 rounded-xl border text-sm space-y-3 ${
            feedback.kind === 'ok'
              ? 'bg-emerald-50 border-emerald-200 text-emerald-900'
              : 'bg-red-50 border-red-200 text-red-900'
          }`}
        >
          <div className="flex gap-2.5 items-start">
            {feedback.kind === 'ok' ? (
              <CheckCircle2 className="w-4 h-4 shrink-0 mt-0.5" />
            ) : (
              <AlertTriangle className="w-4 h-4 shrink-0 mt-0.5" />
            )}
            <p>{feedback.text}</p>
          </div>
          {feedback.kind === 'ok' && (
            <button
              type="button"
              onClick={onRestartDaemon}
              className="px-4 py-2 rounded-lg text-white bg-emerald-700 hover:bg-emerald-800 text-xs font-semibold"
            >
              Restart engine
            </button>
          )}
        </div>
      )}

      <AppearanceControls />
    </div>
  );
};
