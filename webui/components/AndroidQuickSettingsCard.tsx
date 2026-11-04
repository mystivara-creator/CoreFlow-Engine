import React from 'react';
import { Smartphone, Zap, Battery, Cpu, CheckCircle2, ArrowRight } from 'lucide-react';
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
    <div className="m3-card p-6 sm:p-8 space-y-6">
      {/* Header */}
      <div className="flex flex-col sm:flex-row sm:items-start justify-between gap-4 pb-5 border-b border-slate-900/[0.06] dark:border-white/[0.06]">
        <div className="space-y-1.5 max-w-2xl">
          <div className="flex flex-wrap items-center gap-2 text-xs text-slate-500 dark:text-slate-400">
            <Smartphone className="w-4 h-4 text-[#0b57d0] dark:text-[#a8c7fa]" />
            <span className="text-base sm:text-lg font-semibold text-slate-900 dark:text-slate-100">
              Android Kotlin Quick Settings Integration
            </span>
            <span aria-hidden="true">·</span>
            <span className="font-mono">Kotlin 2.1 · API 35</span>
            <span aria-hidden="true">·</span>
            <span>{nativeMode ? 'Root Bridge Connected' : 'Interactive Preview'}</span>
          </div>
          <p className="text-xs sm:text-sm text-slate-600 dark:text-slate-400 leading-relaxed">
            Provides native Android Quick Settings tile integration (`CoreFlowTileService.kt`) to switch optimization intent profiles directly from the system shade.
          </p>
        </div>
      </div>

      <div className="grid grid-cols-1 md:grid-cols-3 gap-5">
        {/* Android 15 Material 3 Quick Settings Interactive Tile */}
        <div className="m3-tonal-block p-6 flex flex-col items-center justify-center text-center space-y-4">
          <div className="text-xs font-medium text-slate-500 dark:text-slate-400">
            Android 15 Quick Settings Tile
          </div>

          <button
            type="button"
            onClick={() => {
              const sequence: AutonomousConfig['adaptiveBias'][] = [
                'balanced',
                'responsiveness',
                'battery',
              ];
              const next = sequence[(sequence.indexOf(currentBias) + 1) % sequence.length];
              onSelectBias(next);
            }}
            className="w-56 px-5 py-4 rounded-[28px] bg-[#d3e3fd] hover:bg-[#c2d7fa] text-[#041e49] dark:bg-[#0842a0] dark:hover:bg-[#0b57d0] dark:text-[#d3e3fd] transition-all flex items-center gap-3.5 text-left shadow-xs"
          >
            <div className="w-10 h-10 rounded-full bg-[#0b57d0] text-white dark:bg-[#a8c7fa] dark:text-[#062e6f] flex items-center justify-center shrink-0">
              {currentBias === 'responsiveness' ? (
                <Zap className="w-4 h-4" />
              ) : currentBias === 'battery' ? (
                <Battery className="w-4 h-4" />
              ) : (
                <Cpu className="w-4 h-4" />
              )}
            </div>
            <div className="min-w-0">
              <div className="text-[11px] opacity-80 font-medium">CoreFlow</div>
              <div className="text-sm font-semibold truncate">
                {currentBias === 'responsiveness'
                  ? 'Responsive'
                  : currentBias === 'battery'
                  ? 'Battery Saver'
                  : 'Balanced'}
              </div>
              <div className="text-[11px] font-mono tabular-nums opacity-75 mt-0.5">
                {socTemp > 0 ? `${socTemp.toFixed(1)} °C` : 'Active'}
              </div>
            </div>
          </button>

          <p className="text-xs text-slate-500 dark:text-slate-400 max-w-xs leading-relaxed">
            Tap the M3 pill tile to cycle profiles via <code className="font-mono text-[#0b57d0] dark:text-[#a8c7fa]">CoreFlowManager.kt</code>.
          </p>
        </div>

        {/* Profile Selector List */}
        <div className="md:col-span-2 space-y-3">
          {[
            {
              id: 'balanced' as const,
              title: 'Balanced Equilibrium',
              desc: 'Default coordinated baseline (UClamp floor 320). Maintains smooth interactive frame delivery while conserving thermal headroom.',
              Icon: Cpu,
            },
            {
              id: 'responsiveness' as const,
              title: 'Interactive Responsiveness',
              desc: 'Elevates foreground scheduling floor (UClamp floor 400–640) when thermal and PSI admission checks pass.',
              Icon: Zap,
            },
            {
              id: 'battery' as const,
              title: 'Endurance & Efficiency',
              desc: 'Lowers baseline capacity floor (UClamp floor 64–240) to minimize power draw during reading, media, and light tasks.',
              Icon: Battery,
            },
          ].map(({ id, title, desc, Icon }) => {
            const active = currentBias === id;
            return (
              <button
                key={id}
                type="button"
                onClick={() => onSelectBias(id)}
                className={
                  'w-full text-left p-4 rounded-[20px] border transition-all flex items-start justify-between gap-4 ' +
                  (active
                    ? 'bg-[#d3e3fd]/60 dark:bg-[#0842a0]/40 border-[#0b57d0]/30 dark:border-[#a8c7fa]/30'
                    : 'm3-tonal-block hover:bg-slate-900/[0.04] dark:hover:bg-white/[0.04]')
                }
              >
                <div className="flex items-start gap-3.5">
                  <div className="w-9 h-9 rounded-xl bg-[#d3e3fd] dark:bg-[#0842a0] text-[#041e49] dark:text-[#d3e3fd] flex items-center justify-center shrink-0 mt-0.5">
                    <Icon className="w-4 h-4" />
                  </div>
                  <div className="space-y-1">
                    <div className="text-xs sm:text-sm font-semibold text-slate-900 dark:text-slate-100">
                      {title}
                    </div>
                    <p className="text-xs text-slate-600 dark:text-slate-400 leading-relaxed">
                      {desc}
                    </p>
                  </div>
                </div>
                {active && (
                  <CheckCircle2 className="w-4 h-4 text-[#0b57d0] dark:text-[#a8c7fa] shrink-0 mt-1" />
                )}
              </button>
            );
          })}
        </div>
      </div>

      {/* Integration Instructions */}
      <div className="m3-tonal-block p-5 text-xs space-y-2">
        <div className="font-semibold text-slate-800 dark:text-slate-200 flex items-center gap-2">
          <ArrowRight className="w-3.5 h-3.5 text-[#0b57d0] dark:text-[#a8c7fa]" />
          Adding the Quick Settings Tile on Android:
        </div>
        <ol className="list-decimal list-inside text-slate-600 dark:text-slate-400 space-y-1 leading-relaxed">
          <li>Swipe down twice from the status bar to open the full Quick Settings shade.</li>
          <li>Tap the pencil edit action and locate <strong>CoreFlow</strong> in the tile tray.</li>
          <li>Drag the tile into your active group to toggle optimization intents anytime.</li>
        </ol>
      </div>
    </div>
  );
};
