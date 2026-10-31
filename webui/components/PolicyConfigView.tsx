import React, { useState } from 'react';
import { AlertTriangle, CheckCircle2, Save, ShieldAlert } from 'lucide-react';
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
  onCommitConfig: (confirmedGate?: boolean) => Promise<{ needsConfirm?: 'resume' | 'arm' | 'observe'; error?: string; saved?: boolean }>;
  onRestartDaemon: () => void | Promise<void>;
}
export const PolicyConfigView: React.FC<PolicyConfigViewProps> = ({ form, savedForm, numericSpecs, probe, currentDaemonMode, onUpdateForm, onCommitConfig, onRestartDaemon }) => {
  const [feedback, setFeedback] = useState<{ kind: 'ok' | 'err'; text: string } | null>(null);
  const [saving, setSaving] = useState(false);
  const C = window.CoreFlowControl;
  const defaults = C?.formFromConfig(C.DEFAULTS);
  const validation = C?.validate(form);
  const changes = Object.keys(form).filter(key => savedForm && String(form[key]) !== String(savedForm[key]));
  const save = async () => {
    setSaving(true); setFeedback(null);
    try {
      const result = await onCommitConfig(false);
      if (result.error || result.needsConfirm) setFeedback({ kind: 'err', text: result.error || 'Gunakan Start di Controls untuk melanjutkan engine yang dihentikan.' });
      else if (result.saved) setFeedback({ kind: 'ok', text: 'Tersimpan ke disk. Restart engine untuk memakai pengaturan ini; memori pengalaman tetap disimpan.' });
    } catch (err) { setFeedback({ kind: 'err', text: err instanceof Error ? err.message : String(err) }); }
    finally { setSaving(false); }
  };
  const field = (spec: NumericFieldSpec) => {
    const guide = SETTINGS_GUIDE[spec.key];
    const error = validation?.errors[spec.key];
    return <div key={spec.key} className="p-4 rounded-xl bg-slate-50 border border-slate-200 space-y-2">
      <label htmlFor={`cfg-${spec.key}`} className="block text-sm font-semibold text-slate-800">{guide?.label || spec.label}{spec.unit ? ` (${spec.unit})` : ''}</label>
      <p id={`help-${spec.key}`} className="text-xs leading-relaxed text-slate-600">{guide?.purpose || spec.help}</p>
      <input id={`cfg-${spec.key}`} type="text" inputMode={spec.kind === 'int' ? 'numeric' : 'decimal'} aria-invalid={!!error} aria-describedby={`help-${spec.key}${error ? ` error-${spec.key}` : ''}`} value={String(form[spec.key] ?? '')} disabled={saving} onChange={e => onUpdateForm({ ...form, [spec.key]: e.target.value })} className={`w-full p-3 text-sm font-mono bg-white border rounded-xl text-slate-900 focus:outline-none focus:ring-2 focus:ring-blue-600 ${error ? 'border-red-500' : 'border-slate-200'}`} />
      {error && <p id={`error-${spec.key}`} className="text-xs text-red-700" role="alert">{error}</p>}
      <p className="text-xs text-slate-500">{guide?.tradeoff}</p>
      <details className="text-xs text-slate-500"><summary className="cursor-pointer">Nilai awal &amp; detail</summary><div className="mt-2 space-y-1"><p>Awal: <strong>{String(defaults?.[spec.key] ?? '—')}</strong> · Rentang: {spec.min}{spec.max == null ? ' ke atas' : `–${spec.max}`} {spec.unit}</p><p className="font-mono break-all">{spec.key}</p></div></details>
    </div>;
  };
  return <div className="space-y-6">
    <div className="pb-4 border-b border-slate-200 space-y-2"><div className="text-xs text-slate-500">Atur draf → periksa → simpan → restart engine</div><h1 className="text-2xl font-bold text-slate-900">Pengaturan engine</h1><p className="text-sm text-slate-600">Engine: <strong>{currentDaemonMode === 'adaptive' ? 'Aktif' : currentDaemonMode === 'stopped' ? 'Berhenti' : currentDaemonMode === 'unknown' ? 'Menunggu data' : 'Ditahan — lihat alasan runtime'}</strong>. Mengubah isian belum mengubah perilaku perangkat.</p></div>
    {probe?.safe && <div role="alert" className="p-4 bg-red-50 border border-red-200 rounded-2xl text-xs text-red-800 flex gap-3"><ShieldAlert className="w-4 h-4 shrink-0" /><p>Proteksi pemulihan aktif. Menyimpan pengaturan tidak menghapus SAFE_MODE atau membatalkan kewajiban rollback.</p></div>}
    <section className="bg-white border border-slate-200 rounded-2xl p-5 space-y-4"><h2 className="font-semibold">Mulai dari pengaturan yang dipahami</h2><p className="text-xs text-slate-500">Pilihan ini hanya mengisi draf; bukan jaminan performa atau suhu tertentu.</p><div className="flex flex-wrap gap-2">{([['balanced', 'Seimbang'], ['quiet', 'Lebih tenang'], ['responsive', 'Pengamatan lebih cepat']] as const).map(([id, label]) => <button type="button" key={id} disabled={!defaults || saving} onClick={() => { if (defaults) onUpdateForm(settingsPreset(id, defaults)); setFeedback(null); }} className="px-4 py-2 text-xs font-semibold rounded-xl bg-slate-50 border border-slate-200 text-slate-700">{label}</button>)}</div><p className="text-xs text-slate-500">Seimbang memakai nilai awal. Lebih tenang: interval 8 detik, kepercayaan 70%, bantuan interaktivitas dimatikan. Pengamatan lebih cepat: interval 2 detik dan kepercayaan 65%; overhead pembacaan bisa naik.</p></section>
    <section className="bg-white border border-slate-200 rounded-2xl p-5 space-y-4"><h2 className="font-semibold">Cara engine bekerja</h2><p className="text-xs text-slate-600 leading-relaxed">CoreFlow mengamati sistem, memilih perubahan yang layak, memeriksa izin, lalu mengukur hasil. Nilai yang membantu dapat dipertahankan; hasil buruk dipulihkan. Pengaturan tersimpan lintas reboot, tetapi baseline dan sampel pengujian diambil ulang agar sesuai kondisi terbaru.</p><div className="grid sm:grid-cols-2 gap-3">
      <label className="flex items-start justify-between gap-4 p-4 bg-slate-50 rounded-xl border border-slate-200 text-sm"><span className="font-medium">Pembaruan kapabilitas<span className="block text-xs font-normal text-slate-500 mt-2">Izinkan engine memeriksa ulang resource saat diperlukan. Umumnya biarkan aktif; bukan perintah untuk menulis semua node.</span></span><input type="checkbox" checked={Boolean(form.runtime_refresh)} disabled={saving} onChange={e => onUpdateForm({ ...form, runtime_refresh: e.target.checked })} /></label>
      <label className="flex items-start justify-between gap-4 p-4 bg-slate-50 rounded-xl border border-slate-200 text-sm"><span className="font-medium">Bantuan interaktivitas<span className="block text-xs font-normal text-slate-500 mt-2">Bantuan sementara untuk launch/frame saat ada bukti tekanan yang sesuai. Mematikannya tetap membiarkan optimasi utama bekerja; kapabilitas dan proteksi suhu selalu diperiksa.</span></span><input type="checkbox" checked={form.interactivity_mode === 'adaptive'} disabled={saving} onChange={e => onUpdateForm({ ...form, interactivity_mode: e.target.checked ? 'adaptive' : 'observe' })} /></label>
    </div></section>
    <section className="bg-white border border-slate-200 rounded-2xl p-5 space-y-4"><h2 className="font-semibold">Interval &amp; kepercayaan data</h2><p className="text-xs text-slate-500">Seberapa sering engine memeriksa kondisi dan seberapa yakin datanya harus tersedia.</p><div className="grid sm:grid-cols-2 gap-4">{numericSpecs.filter(s => s.group === 'runtime').map(field)}</div></section>
    <section className="bg-white border border-slate-200 rounded-2xl p-5 space-y-4"><h2 className="font-semibold">Respons aplikasi &amp; batas boost</h2><p className="text-xs text-slate-500">{form.interactivity_mode === 'adaptive' ? 'Batas bantuan sementara. Nilai hanya berpengaruh pada resource yang didukung dan diizinkan.' : 'Bantuan interaktivitas dimatikan. Nilai di bawah disimpan untuk saat bantuan diaktifkan lagi.'}</p><div className="grid sm:grid-cols-2 gap-4">{numericSpecs.filter(s => s.group === 'interactivity').map(field)}</div></section>
    <section className="bg-white border border-slate-200 rounded-2xl p-5 space-y-4"><div className="flex flex-wrap items-center justify-between gap-3"><h2 className="font-semibold">Periksa draf {savedForm ? `(${changes.length} perubahan)` : ''}</h2><button type="button" disabled={saving || !savedForm || changes.length === 0} onClick={() => savedForm && onUpdateForm({ ...savedForm })} className="text-xs text-blue-700 disabled:opacity-50">Batalkan perubahan draf</button></div>{changes.length ? <dl className="space-y-2 text-xs">{changes.map(key => <div key={key} className="flex flex-wrap justify-between gap-2 border-b border-slate-100 pb-2"><dt>{SETTINGS_GUIDE[key]?.label || (key === 'runtime_refresh' ? 'Pembaruan kapabilitas' : key === 'interactivity_mode' ? 'Bantuan interaktivitas' : key)}</dt><dd className="font-mono">{String(savedForm?.[key])} → {String(form[key])}</dd></div>)}</dl> : <p className="text-xs text-slate-500">Belum ada perubahan dari pengaturan yang dibaca terakhir.</p>}<p className="text-xs text-slate-500">Simpan menulis konfigurasi ke disk. Restart menerapkannya dan mengambil baseline baru; tidak menghapus pengalaman terverifikasi.</p><button type="button" id="btn-apply-mode" disabled={saving || !validation?.ok || (savedForm && !changes.length)} onClick={save} className="inline-flex items-center justify-center gap-2 px-5 py-3 rounded-xl bg-blue-600 text-white font-semibold text-sm disabled:opacity-50"><Save className="w-4 h-4" />{saving ? 'Menyimpan…' : 'Periksa & simpan'}</button></section>
    {feedback && <div role={feedback.kind === 'err' ? 'alert' : 'status'} className={`p-4 rounded-2xl text-sm space-y-3 ${feedback.kind === 'ok' ? 'bg-emerald-50 text-emerald-900' : 'bg-red-50 text-red-900'}`}><div className="flex gap-2">{feedback.kind === 'ok' ? <CheckCircle2 className="w-4 h-4 shrink-0" /> : <AlertTriangle className="w-4 h-4 shrink-0" />}<p>{feedback.text}</p></div>{feedback.kind === 'ok' && <button type="button" onClick={onRestartDaemon} className="px-4 py-2 rounded-xl text-white bg-emerald-700 text-xs font-semibold">Restart engine</button>}</div>}
    <AppearanceControls />
  </div>;
};
