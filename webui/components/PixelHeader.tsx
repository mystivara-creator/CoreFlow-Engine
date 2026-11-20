import React from 'react';
import { Cpu, Moon, Palette, ShieldCheck, Sun } from 'lucide-react';
import { AccentSeed } from '../bridge/appearance';

interface Props {
  isDark: boolean;
  onToggleTheme: () => void;
  accent?: AccentSeed;
  onSelectAccent?: (accent: AccentSeed) => void;
  modeLabel: string;
  healthScore?: number;
  nativeMode?: boolean;
  nativeConnected?: boolean;
}

const ACCENT_OPTIONS: { id: AccentSeed; label: string; swatch: string }[] = [
  { id: 'ocean', label: 'Ocean', swatch: '#0b57d0' },
  { id: 'emerald', label: 'Emerald', swatch: '#006c4c' },
  { id: 'amber', label: 'Amber', swatch: '#b27b00' },
];

export const PixelHeader: React.FC<Props> = ({
  isDark,
  onToggleTheme,
  accent = 'ocean',
  onSelectAccent,
  modeLabel,
  healthScore,
  nativeMode = false,
  nativeConnected = false,
}) => {
  const statusBadgeClass =
    nativeMode && nativeConnected
      ? 'md3-badge md3-badge-success'
      : 'md3-badge md3-badge-warning';

  const statusText = nativeMode
    ? nativeConnected
      ? 'LIVE'
      : 'CONNECTING'
    : 'DEMO';

  return (
    <header className="md3-top-app-bar sticky top-0 z-30">
      <div className="mx-auto flex min-h-[4rem] max-w-5xl items-center justify-between gap-3 px-4 py-2 sm:px-6">
        <div className="flex min-w-0 flex-1 items-center gap-3">
          <div
            className="flex h-10 w-10 shrink-0 items-center justify-center rounded-2xl shadow-xs"
            style={{
              backgroundColor: 'var(--md-sys-color-primary-container)',
              color: 'var(--md-sys-color-on-primary-container)',
            }}
          >
            <Cpu className="h-5 w-5" />
          </div>

          <div className="min-w-0">
            <div className="flex min-w-0 flex-wrap items-center gap-2">
              <span className="truncate text-sm font-bold tracking-tight sm:text-base">
                CoreFlow Engine
              </span>
              <span className="md3-badge md3-badge-neutral font-mono">v5.0.2</span>
              <span className={statusBadgeClass}>
                <span
                  className="h-1.5 w-1.5 rounded-full"
                  style={{
                    backgroundColor:
                      nativeMode && nativeConnected
                        ? 'var(--md-sys-color-success)'
                        : 'var(--md-sys-color-warning)',
                  }}
                />
                {statusText}
              </span>
            </div>
            <p
              className="mt-0.5 truncate text-xs"
              style={{ color: 'var(--md-sys-color-on-surface-variant)' }}
            >
              <span className="font-semibold" style={{ color: 'var(--md-sys-color-on-surface)' }}>
                {modeLabel}
              </span>
              <span> · Lean 4-Pillar Adaptive Runtime</span>
            </p>
          </div>
        </div>

        <div className="flex shrink-0 items-center gap-2">
          {typeof healthScore === 'number' && (
            <div
              className="hidden sm:inline-flex items-center gap-1.5 rounded-full px-3 py-1.5 text-xs font-semibold"
              style={{
                backgroundColor: 'var(--md-sys-color-surface-container)',
                border: '1px solid var(--md-sys-color-outline-variant)',
              }}
            >
              <ShieldCheck className="h-3.5 w-3.5" style={{ color: 'var(--md-sys-color-primary)' }} />
              <span className="font-mono tabular-nums">{healthScore}%</span>
            </div>
          )}

          {onSelectAccent && (
            <div
              className="hidden sm:inline-flex items-center gap-1 rounded-full px-2 py-1"
              style={{
                backgroundColor: 'var(--md-sys-color-surface-container)',
                border: '1px solid var(--md-sys-color-outline-variant)',
              }}
              role="group"
              aria-label="Material You seed color palette"
            >
              <Palette
                className="h-3.5 w-3.5 ml-1 mr-0.5"
                style={{ color: 'var(--md-sys-color-on-surface-variant)' }}
              />
              {ACCENT_OPTIONS.map((opt) => {
                const active = accent === opt.id;
                return (
                  <button
                    key={opt.id}
                    type="button"
                    aria-pressed={active}
                    aria-label={`${opt.label} palette`}
                    onClick={() => onSelectAccent(opt.id)}
                    className="h-5 w-5 rounded-full transition-transform"
                    style={{
                      backgroundColor: opt.swatch,
                      boxShadow: active
                        ? '0 0 0 2px var(--md-sys-color-surface), 0 0 0 4px var(--md-sys-color-primary)'
                        : 'none',
                    }}
                  />
                );
              })}
            </div>
          )}

          <button
            type="button"
            onClick={onToggleTheme}
            className="md3-icon-btn"
            aria-label="Toggle color theme"
            title={isDark ? 'Light mode' : 'Dark mode'}
          >
            {isDark ? <Sun className="h-4 w-4" /> : <Moon className="h-4 w-4" />}
          </button>
        </div>
      </div>
    </header>
  );
};
