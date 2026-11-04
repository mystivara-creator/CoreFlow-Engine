import React from 'react';
import { Cpu, Moon, Sun, Power, Globe } from 'lucide-react';
import { LANGUAGES, SupportedLanguage, translate } from '../bridge/appearance';

interface Props {
  engineActive: boolean;
  onToggleEngine: () => void;
  isDark: boolean;
  onToggleTheme: () => void;
  modeLabel: string;
  nativeMode?: boolean;
  onOpenArchitectureStudio: () => void;
  language: SupportedLanguage;
  onSelectLanguage: (lang: SupportedLanguage) => void;
}

export const PixelHeader: React.FC<Props> = ({
  engineActive,
  onToggleEngine,
  isDark,
  onToggleTheme,
  modeLabel,
  onOpenArchitectureStudio,
  language,
  onSelectLanguage,
}) => (
  <header className="sticky top-0 z-30 backdrop-blur-md bg-white/80 dark:bg-[#090d16]/85 border-b border-slate-200/70 dark:border-slate-800/80 transition-colors duration-200">
    <div className="max-w-6xl mx-auto px-4 sm:px-6 py-3 flex flex-wrap items-center justify-between gap-3">
      <div className="flex items-center gap-3">
        <div className="w-10 h-10 rounded-2xl bg-gradient-to-tr from-sky-500 to-teal-400 flex items-center justify-center text-slate-950 shadow-md shadow-sky-500/20">
          <Cpu className="w-5 h-5" />
        </div>
        <div>
          <div className="flex items-center gap-2">
            <span className="font-bold text-base sm:text-lg tracking-tight text-slate-900 dark:text-white">
              {translate('appTitle', language)}
            </span>
            <span className="text-[11px] font-semibold px-2 py-0.5 rounded-full bg-sky-500/15 text-sky-600 dark:text-sky-400 border border-sky-500/20 font-mono">
              v3.0.R
            </span>
          </div>
          <p className="text-xs text-slate-500 dark:text-slate-400">
            {translate('appSubtitle', language)}
          </p>
        </div>
      </div>
      <div className="flex items-center gap-2 sm:gap-3 ml-auto">
        {/* Language Selector */}
        <div className="relative flex items-center">
          <select
            value={language}
            onChange={(e) => onSelectLanguage(e.target.value as SupportedLanguage)}
            className="appearance-none bg-slate-100 hover:bg-slate-200 dark:bg-slate-800/80 dark:hover:bg-slate-700/80 text-slate-700 dark:text-slate-300 text-xs font-semibold pl-2.5 pr-6 py-1.5 rounded-full border border-slate-200 dark:border-slate-700 transition-all cursor-pointer focus:outline-none focus:ring-1 focus:ring-sky-500"
            aria-label="Select Language"
          >
            {LANGUAGES.map((l) => (
              <option key={l.code} value={l.code} className="bg-white dark:bg-slate-900 text-slate-900 dark:text-slate-100">
                {l.flag} {l.label}
              </option>
            ))}
          </select>
          <Globe className="w-3.5 h-3.5 absolute right-2 pointer-events-none text-slate-400" />
        </div>

        <button
          onClick={onOpenArchitectureStudio}
          className="hidden sm:flex items-center gap-1.5 px-3 py-1.5 rounded-full bg-slate-100 hover:bg-slate-200 dark:bg-slate-800/80 dark:hover:bg-slate-700/80 text-slate-700 dark:text-slate-300 text-xs font-semibold border border-slate-200 dark:border-slate-700 transition-all"
        >
          {translate('studioModalBtn', language)}
        </button>

        <div className="hidden md:flex items-center gap-2 px-3.5 py-1.5 rounded-full bg-slate-100 dark:bg-slate-900 border border-slate-200 dark:border-slate-800 text-xs font-medium">
          <span className={'w-2 h-2 rounded-full ' + (engineActive ? 'bg-emerald-500 animate-pulse' : 'bg-slate-400')} />
          <span>{engineActive ? modeLabel : translate('oemBaseline', language)}</span>
        </div>

        <button
          onClick={onToggleEngine}
          className={'flex items-center gap-2 px-3.5 py-1.5 rounded-full text-xs font-semibold transition-all active:scale-95 ' +
            (engineActive ? 'bg-sky-500 text-slate-950 shadow-sm' : 'bg-slate-200 dark:bg-slate-800 text-slate-700 dark:text-slate-300')}
        >
          <Power className="w-3.5 h-3.5" />
          {engineActive ? translate('engineActive', language) : translate('enginePaused', language)}
        </button>

        <button
          onClick={onToggleTheme}
          className="p-2 rounded-full bg-slate-100 dark:bg-slate-900 border border-slate-200 dark:border-slate-800"
          aria-label="Toggle display theme"
        >
          {isDark ? <Sun className="w-4 h-4 text-amber-400" /> : <Moon className="w-4 h-4" />}
        </button>
      </div>
    </div>
  </header>
);
