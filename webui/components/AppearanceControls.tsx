import { useEffect, useState } from 'react';
import { Monitor, Moon, Sun } from 'lucide-react';
import { applyAppearance, Appearance, readAppearance, saveAppearance, Theme } from '../bridge/appearance';
export function AppearanceControls({ compact = false }: { compact?: boolean }) {
  const [value, setValue] = useState<Appearance>(readAppearance);
  const [temporary, setTemporary] = useState(false);
  useEffect(() => {
    applyAppearance(value);
    const media = window.matchMedia?.('(prefers-color-scheme: dark)');
    const systemChanged = () => applyAppearance(value);
    media?.addEventListener?.('change', systemChanged);
    return () => media?.removeEventListener?.('change', systemChanged);
  }, [value]);
  useEffect(() => {
    const changed = (event: Event) => setValue(event.type === 'coreflow-appearance' ? (event as CustomEvent<Appearance>).detail : readAppearance());
    window.addEventListener('storage', changed);
    window.addEventListener('coreflow-appearance', changed);
    return () => { window.removeEventListener('storage', changed); window.removeEventListener('coreflow-appearance', changed); };
  }, []);
  const update = (next: Appearance) => { setValue(next); setTemporary(!saveAppearance(next)); };
  const Icon = value.theme === 'dark' ? Moon : value.theme === 'light' ? Sun : Monitor;
  const labels: Record<Theme, string> = { system: 'Ikuti sistem', light: 'Terang', dark: 'Gelap' };
  if (compact) return <button type="button" aria-label={`Tema: ${labels[value.theme]}. Ganti tema`} title={`Tema: ${labels[value.theme]}`} onClick={() => update({ ...value, theme: value.theme === 'system' ? 'light' : value.theme === 'light' ? 'dark' : 'system' })} className="inline-flex items-center justify-center h-11 w-11 rounded-xl border border-slate-200 text-slate-700 bg-white mr-2"><Icon className="w-4 h-4" aria-hidden="true" /></button>;
  return <section className="bg-white border border-slate-200 rounded-2xl p-5 space-y-4">
    <div><h2 className="font-semibold">Tampilan</h2><p className="text-xs text-slate-500 mt-1">Tersimpan di WebUI ini, terpisah dari pengaturan engine.</p></div>
    <div className="flex flex-wrap gap-2">{(['system', 'light', 'dark'] as Theme[]).map(theme => <button key={theme} type="button" aria-pressed={value.theme === theme} onClick={() => update({ ...value, theme })} className={`px-4 py-2 rounded-xl border text-xs font-semibold ${value.theme === theme ? 'bg-blue-600 text-white border-blue-600' : 'bg-slate-50 border-slate-200 text-slate-700'}`}>{labels[theme]}</button>)}</div>
    <label className="flex items-center justify-between gap-4 text-sm"><span>Kurangi animasi<span className="block text-xs text-slate-500 mt-1">Preferensi gerakan aksesibilitas sistem juga selalu dihormati.</span></span><input type="checkbox" checked={value.reduceMotion} onChange={e => update({ ...value, reduceMotion: e.target.checked })} /></label>
    {temporary && <p role="status" className="text-xs text-amber-700">Penyimpanan browser tidak tersedia; pilihan berlaku selama halaman ini terbuka.</p>}
  </section>;
}
