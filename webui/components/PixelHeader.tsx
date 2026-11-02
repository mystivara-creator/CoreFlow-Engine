import React from 'react';
import { Cpu, Moon, Sun, Power, ShieldCheck, Sparkles } from 'lucide-react';

interface PixelHeaderProps {
  engineActive: boolean;
  onToggleEngine: () => void;
  isDark: boolean;
  onToggleTheme: () => void;
  epollEventsPerSec: number;
  modeLabel: string;
}

export const PixelHeader: React.FC<PixelHeaderProps> = ({
  engineActive,
  onToggleEngine,
  isDark,
  onToggleTheme,
  epollEventsPerSec,
  modeLabel,
}) => {
  return (
    <header className="sticky top-0 z-30 backdrop-blur-md bg-white/80 dark:bg-[#090d16]/85 border-b border-slate-200/70 dark:border-slate-800/80 transition-colors duration-200">
      <div className="max-w-6xl mx-auto px-4 sm:px-6 py-3 flex flex-wrap items-center justify-between gap-3">
        {/* Brand & Subtext */}
        <div className="flex items-center gap-3">
          <div className="w-10 h-10 rounded-2xl bg-gradient-to-tr from-sky-500 to-teal-400 flex items-center justify-center text-slate-950 font-bold shadow-md shadow-sky-500/20">
            <Cpu className="w-5 h-5 text-slate-950" />
          </div>
          <div>
            <div className="flex items-center gap-2">
              <span className="font-bold text-base sm:text-lg tracking-tight text-slate-900 dark:text-white">
                CoreFlow
              </span>
              <span className="text-[11px] font-semibold tracking-wide px-2 py-0.5 rounded-full bg-sky-500/15 text-sky-600 dark:text-sky-400 border border-sky-500/20">
                v2.0 Modern
              </span>
            </div>
            <p className="text-xs text-slate-500 dark:text-slate-400">
              Android 15+ · Linux epoll zero-polling daemon
            </p>
          </div>
        </div>

        {/* Center / Right controls */}
        <div className="flex items-center gap-2 sm:gap-3 ml-auto">
          {/* Autonomous Status Pill */}
          <div className="hidden md:flex items-center gap-2 px-3.5 py-1.5 rounded-full bg-slate-100 dark:bg-slate-900 border border-slate-200 dark:border-slate-800 text-xs font-medium">
            <span
              className={`w-2 h-2 rounded-full ${
                engineActive
                  ? 'bg-emerald-500 shadow-sm shadow-emerald-500/50 animate-pulse'
                  : 'bg-slate-400'
              }`}
            />
            <span className="text-slate-700 dark:text-slate-300">
              {engineActive ? `Autonomous: ${modeLabel}` : 'Stock OEM Mode'}
            </span>
            {engineActive && (
              <span className="text-slate-400 dark:text-slate-500 font-mono text-[11px] tabular-nums">
                ({epollEventsPerSec} ev/s)
              </span>
            )}
          </div>

          {/* Master Autonomous Toggle Switch */}
          <button
            onClick={onToggleEngine}
            className={`flex items-center gap-2 px-3.5 py-1.5 rounded-full text-xs font-semibold transition-all duration-200 active:scale-95 shadow-sm ${
              engineActive
                ? 'bg-sky-500 hover:bg-sky-400 text-slate-950 shadow-sky-500/25'
                : 'bg-slate-200 dark:bg-slate-800 hover:bg-slate-300 dark:hover:bg-slate-700 text-slate-700 dark:text-slate-300'
            }`}
            title={engineActive ? 'Suspend autonomous tuning' : 'Resume autonomous engine'}
          >
            <Power className="w-3.5 h-3.5" />
            <span>{engineActive ? 'Engine Active' : 'Engine Paused'}</span>
          </button>

          {/* Theme Toggle (Dark/Light) */}
          <button
            onClick={onToggleTheme}
            className="p-2 rounded-full bg-slate-100 dark:bg-slate-900 border border-slate-200 dark:border-slate-800 text-slate-600 dark:text-slate-300 hover:bg-slate-200 dark:hover:bg-slate-800 transition-colors"
            aria-label="Toggle display theme"
          >
            {isDark ? <Sun className="w-4 h-4 text-amber-400" /> : <Moon className="w-4 h-4 text-slate-700" />}
          </button>
        </div>
      </div>
    </header>
  );
};
