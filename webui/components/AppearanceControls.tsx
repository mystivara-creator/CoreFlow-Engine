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
    const changed = (event: Event) =>
      setValue(
        event.type === 'coreflow-appearance'
          ? (event as CustomEvent<Appearance>).detail
          : readAppearance()
      );
    window.addEventListener('storage', changed);
    window.addEventListener('coreflow-appearance', changed);
    return () => {
      window.removeEventListener('storage', changed);
      window.removeEventListener('coreflow-appearance', changed);
    };
  }, []);

  const update = (next: Appearance) => {
    setValue(next);
    setTemporary(!saveAppearance(next));
  };

  const Icon = value.theme === 'dark' ? Moon : value.theme === 'light' ? Sun : Monitor;
  const labels: Record<Theme, string> = { system: 'Ikuti sistem', light: 'Terang', dark: 'Gelap' };
  const themeIndex = value.theme === 'system' ? 0 : value.theme === 'light' ? 1 : 2;
  const slideClass =
    themeIndex === 0
      ? 'translate-x-0'
      : themeIndex === 1
        ? 'translate-x-full'
        : 'translate-x-[200%]';

  if (compact) {
    return (
      <button
        type="button"
        aria-label={`Tema: ${labels[value.theme]}. Ganti tema`}
        title={`Tema: ${labels[value.theme]}`}
        onClick={() =>
          update({
            ...value,
            theme: value.theme === 'system' ? 'light' : value.theme === 'light' ? 'dark' : 'system',
          })
        }
        className="inline-flex items-center justify-center h-9 w-9 rounded-lg border border-slate-200 text-slate-700 bg-slate-50 hover:bg-slate-100 mr-2 shrink-0"
      >
        <Icon className="w-4 h-4" aria-hidden="true" />
      </button>
    );
  }

  return (
    <section className="cf-card bg-white border border-slate-200 rounded-xl p-5 space-y-4">
      <div>
        <h2 className="text-sm font-semibold text-slate-900">Tampilan &amp; Gerakan Antarmuka</h2>
        <p className="text-xs text-slate-500 mt-1">
          Tersimpan lokal di WebUI ini, terpisah dari file konfigurasi daemon.
        </p>
      </div>

      <div className="relative grid grid-cols-3 p-1 bg-slate-100 border border-slate-200 rounded-xl max-w-md">
        <span
          aria-hidden="true"
          className={`pointer-events-none absolute top-1 bottom-1 left-1 w-[calc((100%-0.5rem)/3)] rounded-lg bg-white shadow-xs border border-slate-200/80 transition-transform duration-300 ease-[cubic-bezier(0.16,1,0.3,1)] ${slideClass}`}
        />
        {(['system', 'light', 'dark'] as Theme[]).map((theme) => {
          const ThemeIcon = theme === 'dark' ? Moon : theme === 'light' ? Sun : Monitor;
          const active = value.theme === theme;
          return (
            <button
              key={theme}
              type="button"
              aria-pressed={active}
              onClick={() => update({ ...value, theme })}
              className={`relative z-10 inline-flex items-center justify-center gap-1.5 py-2 px-3 rounded-lg text-xs font-semibold transition-colors ${
                active ? 'text-slate-900' : 'text-slate-600 hover:text-slate-900'
              }`}
            >
              <ThemeIcon className="w-3.5 h-3.5 shrink-0" aria-hidden="true" />
              <span>{labels[theme]}</span>
            </button>
          );
        })}
      </div>

      <label className="flex items-center justify-between gap-4 p-3.5 bg-slate-50 border border-slate-200 rounded-xl text-sm cursor-pointer">
        <span>
          <span className="font-medium text-slate-900">Kurangi animasi</span>
          <span className="block text-xs text-slate-500 mt-0.5">
            Menonaktifkan transisi pegas &amp; animasi grafik. Preferensi gerakan sistem juga selalu dihormati.
          </span>
        </span>
        <input
          type="checkbox"
          checked={value.reduceMotion}
          onChange={(e) => update({ ...value, reduceMotion: e.target.checked })}
          className="w-4 h-4 rounded border-slate-300 text-blue-600 focus:ring-blue-600 shrink-0"
        />
      </label>

      {temporary && (
        <p role="status" className="text-xs text-amber-700">
          Penyimpanan browser tidak tersedia; pilihan berlaku selama halaman ini terbuka.
        </p>
      )}
    </section>
  );
}
