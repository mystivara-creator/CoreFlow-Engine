import React from 'react';
import { Cpu, Layers, Moon, Palette, ShieldCheck, Sun } from 'lucide-react';
import { AccentSeed } from '../bridge/appearance';

interface Props {
  isDark: boolean;
  onToggleTheme: () => void;
  accent?: AccentSeed;
  onSelectAccent?: (accent: AccentSeed) => void;
  modeLabel: string;
  equilibriumScore?: number;
  nativeMode?: boolean;
  nativeConnected?: boolean;
  onOpenArchitectureStudio: () => void;
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
  equilibriumScore,
  nativeMode = false,
  nativeConnected = false,
  onOpenArchitectureStudio,
}) => {
  const statusBadgeClass =
    nativeMode && nativeConnected
      ? 'md3-badge md3-badge-success'
      : 'md3-badge md3-badge-warning';

  const statusText = nativeMode
    ? nativeConnected
      ? 'DAEMON LINKED'
      : 'AWAITING PAYLOAD'
    : 'SIMULATION PREVIEW';

  return (
    <header className="md3-top-app-bar sticky top-0 z-30">
      <div className="mx-auto flex min-h-[4.25rem] max-w-7xl items-center justify-between gap-3 px-4 py-2.5 sm:px-6">
        {/* Brand Identity & Operating State */}
        <div className="flex min-w-0 flex-1 items-center gap-3.5">
          <div
            className="flex h-11 w-11 shrink-0 items-center justify-center rounded-2xl shadow-xs"
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
                CFAE System Framework
              </span>
              <span className="md3-badge md3-badge-neutral font-mono">
                v4.0.0
              </span>
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
              <span> · Coordinated Mutation Safety &amp; Autonomous Equilibrium</span>
            </p>
          </div>
        </div>

        {/* Material Design 3 Action Cluster & Dynamic Seed Selector */}
        <div className="flex shrink-0 items-center gap-2">
          {typeof equilibriumScore === 'number' && (
            <div
              className="hidden md:inline-flex items-center gap-1.5 rounded-full px-3 py-1.5 text-xs font-semibold"
              style={{
                backgroundColor: 'var(--md-sys-color-surface-container)',
                border: '1px solid var(--md-sys-color-outline-variant)',
              }}
              title="Composite equilibrium index derived from thermal headroom, PSI pressure, and ownership health"
            >
              <ShieldCheck
                className="h-3.5 w-3.5"
                style={{ color: 'var(--md-sys-color-primary)' }}
              />
              <span className="font-mono tabular-nums">{equilibriumScore}%</span>
              <span className="text-[11px]" style={{ color: 'var(--md-sys-color-on-surface-variant)' }}>
                Equilibrium
              </span>
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
                    id={`btn-accent-${opt.id}`}
                    aria-pressed={active}
                    aria-label={`${opt.label} Material 3 palette`}
                    title={`${opt.label} Material 3 palette`}
                    onClick={() => onSelectAccent(opt.id)}
                    className="h-5 w-5 rounded-full flex items-center justify-center transition-transform"
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
            id="btn-open-architecture-studio"
            onClick={onOpenArchitectureStudio}
            title="Open Architecture Studio"
            aria-label="Open Architecture Studio"
            className="md3-btn-tonal"
          >
            <Layers className="h-4 w-4" />
            <span className="hidden sm:inline">Studio</span>
          </button>

          <button
            type="button"
            id="btn-toggle-theme"
            onClick={onToggleTheme}
            className="md3-icon-btn"
            aria-label="Toggle color theme"
            title={isDark ? 'Switch to light theme' : 'Switch to dark theme'}
          >
            {isDark ? <Sun className="h-4 w-4" /> : <Moon className="h-4 w-4" />}
          </button>
        </div>
      </div>
    </header>
  );
};
