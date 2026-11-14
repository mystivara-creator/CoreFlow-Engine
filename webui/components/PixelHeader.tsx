import React from 'react';
import { Cpu, Moon, Sun, Layers, ShieldCheck } from 'lucide-react';

interface Props {
  isDark: boolean;
  onToggleTheme: () => void;
  modeLabel: string;
  nativeMode?: boolean;
  nativeConnected?: boolean;
  onOpenArchitectureStudio: () => void;
}

export const PixelHeader: React.FC<Props> = ({
  isDark,
  onToggleTheme,
  modeLabel,
  nativeMode = false,
  nativeConnected = false,
  onOpenArchitectureStudio,
}) => (
  <header className="pixel-header sticky top-0 z-30">
    <div className="pixel-header-inner mx-auto flex min-h-[4.25rem] max-w-7xl items-center justify-between gap-3 px-4 py-2.5 sm:px-6">
      <div className="flex min-w-0 flex-1 items-center gap-3">
        <div className="pixel-brandmark flex h-10 w-10 shrink-0 items-center justify-center rounded-2xl">
          <Cpu className="h-5 w-5" />
        </div>
        <div className="min-w-0">
          <div className="flex min-w-0 flex-wrap items-center gap-2">
            <span className="truncate text-sm font-bold tracking-tight text-slate-900 dark:text-slate-100 sm:text-base">CFAE System Framework</span>
            <span className="rounded-md bg-slate-900/[0.05] px-1.5 py-0.5 font-mono text-[10px] text-slate-600 dark:bg-white/[0.07] dark:text-slate-300">v4.0.0</span>
            <span className={'pixel-connection inline-flex items-center gap-1.5 rounded-full px-2 py-1 text-[10px] font-bold ' + (nativeMode && nativeConnected ? 'is-connected' : 'is-pending')}>
              <span className={'h-1.5 w-1.5 rounded-full ' + (nativeMode && nativeConnected ? 'bg-emerald-500' : 'bg-amber-500')} />
              {nativeMode ? (nativeConnected ? 'DAEMON CONNECTED' : 'AWAITING DATA') : 'PREVIEW MODE'}
            </span>
          </div>
          <p className="mt-0.5 truncate text-[11px] text-slate-500 dark:text-slate-400">
            <span className="font-medium text-slate-700 dark:text-slate-300">{modeLabel}</span>
            <span> · autonomous system optimization</span>
          </p>
        </div>
      </div>
      <div className="flex shrink-0 items-center gap-2">
        <span className="pixel-autonomous hidden items-center gap-1.5 rounded-full px-3 py-2 text-[11px] font-medium sm:inline-flex">
          <ShieldCheck className="h-3.5 w-3.5 text-emerald-600 dark:text-emerald-400" />
          Autonomous
        </span>
        <button onClick={onOpenArchitectureStudio} title="Open Architecture Studio" aria-label="Open Architecture Studio" className="pixel-studio inline-flex h-9 items-center justify-center gap-2 rounded-full px-3 text-xs font-semibold">
          <Layers className="h-4 w-4" /><span className="hidden sm:inline">Studio</span>
        </button>
        <button onClick={onToggleTheme} className="pixel-theme-toggle flex h-9 w-9 items-center justify-center rounded-full" aria-label="Toggle color theme">
          {isDark ? <Sun className="h-4 w-4" /> : <Moon className="h-4 w-4" />}
        </button>
      </div>
    </div>
  </header>
);
