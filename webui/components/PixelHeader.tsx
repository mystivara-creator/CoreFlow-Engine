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
  <header className="sticky top-0 z-30 border-b border-slate-900/[0.06] bg-[#f7f9fc]/90 backdrop-blur-2xl transition-colors duration-200 dark:border-white/[0.06] dark:bg-[#0e1117]/90">
    <div className="mx-auto flex min-h-[4.25rem] max-w-7xl items-center justify-between gap-3 px-4 py-2.5 sm:px-6">
      <div className="flex min-w-0 flex-1 items-center gap-3">
        <div className="flex h-10 w-10 shrink-0 items-center justify-center rounded-2xl bg-[#d3e3fd] text-[#041e49] dark:bg-[#0842a0] dark:text-[#d3e3fd]">
          <Cpu className="h-5 w-5" />
        </div>
        <div className="min-w-0">
          <div className="flex min-w-0 flex-wrap items-center gap-2">
            <span className="truncate text-sm font-bold tracking-tight text-slate-900 dark:text-slate-100 sm:text-base">CFAE System Framework</span>
            <span className="rounded-md bg-slate-900/[0.05] px-1.5 py-0.5 font-mono text-[10px] text-slate-600 dark:bg-white/[0.07] dark:text-slate-300">v3.1.0</span>
            <span className={'inline-flex items-center gap-1.5 rounded-full px-2 py-1 text-[10px] font-bold ' + (nativeMode && nativeConnected ? 'bg-emerald-100 text-emerald-800 dark:bg-emerald-950/70 dark:text-emerald-300' : 'bg-amber-100 text-amber-900 dark:bg-amber-950/70 dark:text-amber-200')}>
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
        <span className="hidden items-center gap-1.5 rounded-full border border-slate-900/[0.07] px-3 py-2 text-[11px] font-medium text-slate-600 dark:border-white/[0.08] dark:text-slate-300 sm:inline-flex">
          <ShieldCheck className="h-3.5 w-3.5 text-emerald-600 dark:text-emerald-400" />
          Autonomous
        </span>
        <button onClick={onOpenArchitectureStudio} title="Open Architecture Studio" aria-label="Open Architecture Studio" className="inline-flex h-9 items-center justify-center gap-2 rounded-full bg-[#d3e3fd] px-3 text-xs font-semibold text-[#041e49] transition-colors hover:bg-[#c2d7fa] dark:bg-[#0842a0] dark:text-[#d3e3fd] dark:hover:bg-[#0b57d0]">
          <Layers className="h-4 w-4" /><span className="hidden sm:inline">Studio</span>
        </button>
        <button onClick={onToggleTheme} className="flex h-9 w-9 items-center justify-center rounded-full bg-slate-900/[0.05] text-slate-700 hover:bg-slate-900/[0.09] dark:bg-white/[0.06] dark:text-slate-200 dark:hover:bg-white/[0.11]" aria-label="Toggle color theme">
          {isDark ? <Sun className="h-4 w-4" /> : <Moon className="h-4 w-4" />}
        </button>
      </div>
    </div>
  </header>
);
