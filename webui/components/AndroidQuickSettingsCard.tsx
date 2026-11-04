import React from 'react';
import { Smartphone, Zap, Battery, Sparkles, CheckCircle2, ArrowRight } from 'lucide-react';
import { AutonomousConfig } from '../engine/autonomousEngine';

interface Props {
  currentBias: AutonomousConfig['adaptiveBias'];
  onSelectBias: (bias: AutonomousConfig['adaptiveBias']) => void;
  nativeMode: boolean;
  socTemp: number;
}

export const AndroidQuickSettingsCard: React.FC<Props> = ({
  currentBias,
  onSelectBias,
  nativeMode,
  socTemp,
}) => {
  return (
    <div className="rounded-3xl bg-white dark:bg-[#111827] border border-slate-200 dark:border-slate-800 p-5 sm:p-7 shadow-sm space-y-6">
      {/* Header */}
      <div className="flex flex-col sm:flex-row sm:items-center justify-between gap-3">
        <div>
          <div className="flex items-center gap-2 mb-1">
            <Smartphone className="w-4 h-4 text-sky-500" />
            <h3 className="text-base sm:text-lg font-bold text-slate-900 dark:text-white">
              Android Kotlin & Quick Settings Tile
            </h3>
            <span className="px-2.5 py-0.5 rounded-full text-xs font-semibold bg-sky-500/10 text-sky-500 border border-sky-500/20">
              Kotlin 2.1 · API 35
            </span>
            <span className="px-2 py-0.5 rounded-full text-[10px] font-semibold bg-slate-100 dark:bg-slate-800 text-slate-600 dark:text-slate-400 border border-slate-300 dark:border-slate-700">
              {nativeMode ? 'Root Service Live' : 'Demo Mode'}
            </span>
          </div>
          <p className="text-xs text-slate-500 dark:text-slate-400">
            Control CFAE System Framework directly from the Android Quick Settings status shade without opening an app.
          </p>
        </div>
      </div>

      <div className="grid grid-cols-1 md:grid-cols-3 gap-5">
        {/* Android Quick Settings Interactive Tile Simulation */}
        <div className="p-5 rounded-2xl bg-slate-900 text-white border border-slate-800 flex flex-col items-center justify-center text-center space-y-4">
          <div className="text-[11px] uppercase tracking-wider text-slate-400 font-semibold">
            Quick Settings Tile Preview
          </div>

          <div
            onClick={() => {
              const sequence: AutonomousConfig['adaptiveBias'][] = ['balanced', 'responsiveness', 'battery'];
              const next = sequence[(sequence.indexOf(currentBias) + 1) % sequence.length];
              onSelectBias(next);
            }}
            className="w-48 p-4 rounded-3xl bg-sky-500 text-white cursor-pointer hover:bg-sky-400 transition-all transform active:scale-95 shadow-lg shadow-sky-500/20 select-none flex items-center gap-3.5"
          >
            <div className="p-2.5 rounded-2xl bg-white/20">
              {currentBias === 'responsiveness' ? (
                <Zap className="w-5 h-5" />
              ) : currentBias === 'battery' ? (
                <Battery className="w-5 h-5" />
              ) : (
                <Sparkles className="w-5 h-5" />
              )}
            </div>
            <div className="text-left">
              <div className="text-[11px] text-white/80 font-medium">CoreFlow Mode</div>
              <div className="text-sm font-bold capitalize">
                {currentBias === 'responsiveness'
                  ? 'Performance'
                  : currentBias === 'battery'
                  ? 'Battery Saver'
                  : 'Balanced'}
              </div>
              <div className="text-[10px] text-white/70 font-mono mt-0.5">
                {socTemp > 0 ? `${socTemp.toFixed(1)}°C` : 'Active'}
              </div>
            </div>
          </div>

          <div className="text-[11px] text-slate-400 max-w-xs">
            Tap tile to cycle profiles. Syncs with <code className="text-sky-400">CoreFlowTileService.kt</code>.
          </div>
        </div>

        {/* Profile Details & Selector */}
        <div className="md:col-span-2 space-y-3">
          <div className="text-xs font-semibold text-slate-700 dark:text-slate-300">
            Select Active Profile:
          </div>

          <div
            onClick={() => onSelectBias('balanced')}
            className={`p-4 rounded-2xl border cursor-pointer transition-all flex items-start justify-between ${
              currentBias === 'balanced'
                ? 'bg-teal-500/10 border-teal-500/40 text-teal-900 dark:text-teal-100'
                : 'bg-slate-50 dark:bg-slate-900/40 border-slate-200 dark:border-slate-800 hover:border-slate-300'
            }`}
          >
            <div className="flex items-start gap-3">
              <div className="p-2 rounded-xl bg-teal-500/15 text-teal-500 mt-0.5">
                <Sparkles className="w-4 h-4" />
              </div>
              <div>
                <div className="text-xs font-bold text-slate-900 dark:text-white">
                  Balanced Equilibrium
                </div>
                <div className="text-[11px] text-slate-500 dark:text-slate-400 mt-0.5">
                  Top-app UClamp floor: 320. Smooth 120Hz scrolling with thermal equilibrium.
                </div>
              </div>
            </div>
            {currentBias === 'balanced' && <CheckCircle2 className="w-4 h-4 text-teal-500 mt-1" />}
          </div>

          <div
            onClick={() => onSelectBias('responsiveness')}
            className={`p-4 rounded-2xl border cursor-pointer transition-all flex items-start justify-between ${
              currentBias === 'responsiveness'
                ? 'bg-sky-500/10 border-sky-500/40 text-sky-900 dark:text-sky-100'
                : 'bg-slate-50 dark:bg-slate-900/40 border-slate-200 dark:border-slate-800 hover:border-slate-300'
            }`}
          >
            <div className="flex items-start gap-3">
              <div className="p-2 rounded-xl bg-sky-500/15 text-sky-500 mt-0.5">
                <Zap className="w-4 h-4" />
              </div>
              <div>
                <div className="text-xs font-bold text-slate-900 dark:text-white">
                  Performance & Peak Responsiveness
                </div>
                <div className="text-[11px] text-slate-500 dark:text-slate-400 mt-0.5">
                  Top-app UClamp floor: 640. Rapid scheduler ramp-up for competitive gaming.
                </div>
              </div>
            </div>
            {currentBias === 'responsiveness' && <CheckCircle2 className="w-4 h-4 text-sky-500 mt-1" />}
          </div>

          <div
            onClick={() => onSelectBias('battery')}
            className={`p-4 rounded-2xl border cursor-pointer transition-all flex items-start justify-between ${
              currentBias === 'battery'
                ? 'bg-emerald-500/10 border-emerald-500/40 text-emerald-900 dark:text-emerald-100'
                : 'bg-slate-50 dark:bg-slate-900/40 border-slate-200 dark:border-slate-800 hover:border-slate-300'
            }`}
          >
            <div className="flex items-start gap-3">
              <div className="p-2 rounded-xl bg-emerald-500/15 text-emerald-500 mt-0.5">
                <Battery className="w-4 h-4" />
              </div>
              <div>
                <div className="text-xs font-bold text-slate-900 dark:text-white">
                  Battery Saver
                </div>
                <div className="text-[11px] text-slate-500 dark:text-slate-400 mt-0.5">
                  Top-app UClamp floor: 64. Extends endurance during reading, media, and idle.
                </div>
              </div>
            </div>
            {currentBias === 'battery' && <CheckCircle2 className="w-4 h-4 text-emerald-500 mt-1" />}
          </div>
        </div>
      </div>

      {/* Integration Instructions */}
      <div className="p-4 rounded-2xl bg-slate-50 dark:bg-slate-900/40 border border-slate-200/80 dark:border-slate-800 text-xs space-y-2">
        <div className="font-semibold text-slate-800 dark:text-slate-200 flex items-center gap-2">
          <ArrowRight className="w-3.5 h-3.5 text-sky-500" />
          How to add Quick Settings Tile to your Android Status Bar:
        </div>
        <ol className="list-decimal list-inside text-slate-500 dark:text-slate-400 space-y-1 text-[11px]">
          <li>Swipe down twice from top of screen to expand Quick Settings.</li>
          <li>Tap the <strong>Edit (Pencil)</strong> icon in the Quick Settings panel.</li>
          <li>Find <strong>CoreFlow Mode</strong> in available tiles and drag it into active tiles.</li>
          <li>Tap the tile anytime to switch between Balanced, Performance, and Battery.</li>
        </ol>
      </div>
    </div>
  );
};
