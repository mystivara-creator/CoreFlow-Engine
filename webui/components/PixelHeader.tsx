import React from 'react';
import { Cpu, Globe, Moon, ShieldCheck, Sun } from 'lucide-react';

interface Props {
  isDark: boolean;
  onToggleTheme: () => void;
  lang: 'id' | 'en';
  onToggleLang: () => void;
  modeLabel: string;
  version: string;
  healthScore?: number;
  nativeMode?: boolean;
  nativeConnected?: boolean;
}

export const PixelHeader: React.FC<Props> = ({
  isDark,
  onToggleTheme,
  lang,
  onToggleLang,
  modeLabel,
  version,
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
      : 'WAIT'
    : 'DEMO';

  return (
    <header className="md3-top-app-bar sticky top-0 z-30">
      <div className="mx-auto flex h-14 max-w-4xl items-center justify-between gap-2 px-3.5 sm:px-5">
        <div className="flex min-w-0 flex-1 items-center gap-2.5">
          <div
            className="flex h-9 w-9 shrink-0 items-center justify-center rounded-xl"
            style={{
              backgroundColor: 'var(--md-sys-color-primary-container)',
              color: 'var(--md-sys-color-on-primary-container)',
            }}
          >
            <Cpu className="h-4 w-4" />
          </div>

          <div className="min-w-0">
            <div className="flex items-center gap-1.5">
              <span className="truncate text-sm font-bold tracking-tight">
                CoreFlow
              </span>
              <span className="md3-badge md3-badge-neutral font-mono">v{version}</span>
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
              className="truncate text-[11px] leading-tight"
              style={{ color: 'var(--md-sys-color-on-surface-variant)' }}
            >
              <span className="font-semibold" style={{ color: 'var(--md-sys-color-on-surface)' }}>
                {modeLabel}
              </span>
              <span> · NextGen 4-Pillar</span>
            </p>
          </div>
        </div>

        <div className="flex shrink-0 items-center gap-1.5">
          {typeof healthScore === 'number' && (
            <div
              className="hidden sm:inline-flex items-center gap-1 rounded-xl px-2.5 py-1 text-[11px] font-semibold"
              style={{
                backgroundColor: 'var(--md-sys-color-surface-container)',
                border: '1px solid var(--md-sys-color-outline-variant)',
              }}
            >
              <ShieldCheck className="h-3.5 w-3.5" style={{ color: 'var(--md-sys-color-primary)' }} />
              <span className="font-mono tabular-nums">{healthScore}%</span>
            </div>
          )}

          <button
            type="button"
            onClick={onToggleLang}
            className="md3-icon-btn w-auto px-2 gap-1 text-[11px] font-bold"
            aria-label="Switch language"
            title="ID / EN"
          >
            <Globe className="h-3.5 w-3.5" />
            <span>{lang.toUpperCase()}</span>
          </button>

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
