import React from 'react';
import { ShieldCheck, Fingerprint, LockKeyhole, RefreshCw } from 'lucide-react';

interface Props {
  nodesSnapshotted: number;
  nativeMode: boolean;
  baselineStatus: 'restored' | 'protected';
}

export const BaselineGuardianCard: React.FC<Props> = ({ nodesSnapshotted, nativeMode, baselineStatus }) => (
  <section className="m3-card space-y-5 p-5 sm:p-7" aria-label="Baseline ownership and recovery">
    <div className="flex items-start gap-3">
      <div className="flex h-11 w-11 shrink-0 items-center justify-center rounded-2xl bg-[#d3e3fd] text-[#041e49] dark:bg-[#0842a0] dark:text-[#d3e3fd]"><ShieldCheck className="h-5 w-5" /></div>
      <div className="min-w-0 flex-1">
        <div className="flex flex-wrap items-center gap-2">
          <h3 className="text-base font-semibold text-slate-900 dark:text-slate-100">Ownership Guard & Baseline Recovery</h3>
          <span className={'rounded-full px-2.5 py-1 text-[10px] font-bold ' + (nativeMode ? 'bg-emerald-100 text-emerald-800 dark:bg-emerald-950/60 dark:text-emerald-300' : 'bg-amber-100 text-amber-800 dark:bg-amber-950/60 dark:text-amber-300')}>{nativeMode ? 'NATIVE JOURNAL' : 'PREVIEW'}</span>
        </div>
        <p className="mt-1 text-xs leading-relaxed text-slate-600 dark:text-slate-400">CoreFlow records factory and prior values before mutation, then restores only values it can still prove it owns. Recovery is driven by the daemon journal, not by a dashboard tuning control.</p>
      </div>
    </div>
    <div className="grid grid-cols-1 gap-3 sm:grid-cols-3">
      <div className="m3-tonal-block p-4"><Fingerprint className="mb-3 h-4 w-4 text-[#0b57d0] dark:text-[#a8c7fa]"/><div className="text-[11px] text-slate-500 dark:text-slate-400">Journal format</div><div className="mt-1 font-mono text-sm font-semibold text-slate-900 dark:text-slate-100">CFJ4 · W/A</div></div>
      <div className="m3-tonal-block p-4"><LockKeyhole className="mb-3 h-4 w-4 text-[#0b57d0] dark:text-[#a8c7fa]"/><div className="text-[11px] text-slate-500 dark:text-slate-400">Ownership state</div><div className="mt-1 text-sm font-semibold text-slate-900 dark:text-slate-100">{baselineStatus === 'protected' ? 'Recovery protected' : 'No pending recovery'}</div></div>
      <div className="m3-tonal-block p-4"><RefreshCw className="mb-3 h-4 w-4 text-[#0b57d0] dark:text-[#a8c7fa]"/><div className="text-[11px] text-slate-500 dark:text-slate-400">Verified snapshots</div><div className="mt-1 font-mono text-sm font-semibold text-slate-900 dark:text-slate-100">{nativeMode ? nodesSnapshotted : `${nodesSnapshotted} (sim)`}</div></div>
    </div>
    <p className="border-t border-slate-900/[0.06] pt-4 text-[11px] leading-relaxed text-slate-500 dark:border-white/[0.07] dark:text-slate-400">Manual policy mutation is intentionally absent. If a safety recovery is required, the native recovery lifecycle and module-level kill switch remain authoritative.</p>
  </section>
);
