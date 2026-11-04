import React from 'react';
import { Cpu, Moon, Sun, Power, Layers } from 'lucide-react';

interface Props {
  engineActive: boolean;
  onToggleEngine: () => void;
  isDark: boolean;
  onToggleTheme: () => void;
  modeLabel: string;
  nativeMode?: boolean;
  onOpenArchitectureStudio: () => void;
}

export const PixelHeader: React.FC<Props> = ({
  engineActive,
  onToggleEngine,
  isDark,
  onToggleTheme,
  modeLabel,
  nativeMode = false,
  onOpenArchitectureStudio,
}) => (
  <header className="sticky top-0 z-30 backdrop-blur-2xl bg-[#f7f9fc]/80 dark:bg-[#0e1117]/80 border-b border-slate-900/[0.06] dark:border-white/[0.06] transition-colors duration-200">
    <div className="max-w-6xl mx-auto px-4 sm:px-6 h-16 flex items-center justify-between gap-4">
      {/* Zone 1: Brand Identity */}
      <div className="flex items-center gap-3.5 min-w-0">
        <div className="w-10 h-10 rounded-2xl bg-[#d3e3fd] dark:bg-[#0842a0] text-[#041e49] dark:text-[#d3e3fd] flex items-center justify-center shrink-0">
          <Cpu className="w-5 h-5" />
        </div>
        <div className="min-w-0">
          <div className="flex items-center gap-2">
            <span className="font-bold text-base tracking-tight text-slate-900 dark:text-slate-100 whitespace-nowrap">
              CFAE System Framework
            </span>
            <span className="text-xs text-slate-400 dark:text-slate-500" aria-hidden="true">·</span>
            <span className="text-xs font-mono text-slate-500 dark:text-slate-400 whitespace-nowrap">
              v3.0.R
            </span>
          </div>
          <p className="text-xs text-slate-500 dark:text-slate-400 truncate">
            {nativeMode ? 'Native C++23 · Rust 2024 · Kotlin Runtime' : 'Coordinated Mutation Safety · Preview'}
            {' · '}
            <span className="font-medium text-slate-700 dark:text-slate-300">
              {engineActive ? modeLabel : 'OEM Baseline'}
            </span>
          </p>
        </div>
      </div>

      {/* Zone 3: Material 3 Tonal & Filled Actions */}
      <div className="flex items-center gap-2 shrink-0">
        <button
          onClick={onOpenArchitectureStudio}
          className="inline-flex items-center gap-2 px-4 py-2 rounded-full bg-slate-900/[0.05] hover:bg-slate-900/[0.09] dark:bg-white/[0.06] dark:hover:bg-white/[0.11] text-slate-700 dark:text-slate-200 text-xs font-medium whitespace-nowrap"
        >
          <Layers className="w-3.5 h-3.5" />
          <span className="hidden sm:inline">Architecture Studio</span>
        </button>

        <button
          onClick={onToggleEngine}
          className={
            'inline-flex items-center gap-2 px-4 py-2 rounded-full text-xs font-medium whitespace-nowrap ' +
            (engineActive
              ? 'bg-[#0b57d0] hover:bg-[#0842a0] text-white dark:bg-[#a8c7fa] dark:hover:bg-[#8ab4f8] dark:text-[#062e6f]'
              : 'bg-slate-900/[0.06] hover:bg-slate-900/[0.1] dark:bg-white/[0.08] dark:hover:bg-white/[0.12] text-slate-700 dark:text-slate-300')
          }
        >
          <Power className="w-3.5 h-3.5" />
          <span>{engineActive ? 'Engine Active' : 'Engine Standby'}</span>
        </button>

        <button
          onClick={onToggleTheme}
          className="w-9 h-9 rounded-full bg-slate-900/[0.05] hover:bg-slate-900/[0.09] dark:bg-white/[0.06] dark:hover:bg-white/[0.11] flex items-center justify-center text-slate-700 dark:text-slate-200"
          aria-label="Toggle color theme"
        >
          {isDark ? <Sun className="w-4 h-4" /> : <Moon className="w-4 h-4" />}
        </button>
      </div>
    </div>
  </header>
);
