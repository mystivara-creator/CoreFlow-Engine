import React, { useState } from 'react';
import { ShieldCheck, RotateCcw, AlertTriangle, CheckCircle2 } from 'lucide-react';

interface BaselineGuardianCardProps {
  nodesSnapshotted: number;
  nativeMode?: boolean;
  baselineStatus: 'protected' | 'restored';
  onRestoreBaseline: () => void;
  isRestoring: boolean;
  restoreFeedback: string | null;
}

export const BaselineGuardianCard: React.FC<BaselineGuardianCardProps> = ({
  nodesSnapshotted,
  nativeMode = false,
  baselineStatus,
  onRestoreBaseline,
  isRestoring,
  restoreFeedback,
}) => {
  const [showConfirm, setShowConfirm] = useState(false);

  const handleConfirm = () => {
    setShowConfirm(false);
    onRestoreBaseline();
  };

  return (
    <div className="rounded-3xl bg-slate-100/80 dark:bg-[#111827]/90 border border-slate-200/90 dark:border-slate-800 p-5 sm:p-7 shadow-lg shadow-slate-900/5 transition-all">
      <div className="flex flex-col sm:flex-row sm:items-center justify-between gap-4">
        <div className="flex items-start gap-3.5">
          <div className="w-10 h-10 rounded-2xl bg-emerald-500/10 border border-emerald-500/20 flex items-center justify-center text-emerald-500 shrink-0 mt-0.5">
            <ShieldCheck className="w-5 h-5" />
          </div>
          <div>
            <div className="flex items-center gap-2">
              <h3 className="font-bold text-base sm:text-lg text-slate-900 dark:text-white">
                Verified Baseline Recovery
              </h3>
              <span className="text-[11px] font-semibold px-2 py-0.5 rounded-full bg-emerald-500/10 text-emerald-600 dark:text-emerald-400 border border-emerald-500/20">
                {nativeMode ? 'Write-ahead Journal' : (baselineStatus === 'protected' ? 'Preview Protected' : 'Preview Restored')}
              </span>
            </div>
            <p className="text-xs text-slate-500 dark:text-slate-400 mt-1 max-w-xl">
              {nativeMode ? 'CoreFlow records observed pre-mutation values before each write and verifies restoration. This stops the daemon and restores tracked pre-CoreFlow values.' : `${nodesSnapshotted} nodes simulated; no device writes in preview.`}
            </p>
          </div>
        </div>

        {/* Action Button */}
        <div className="self-start sm:self-center shrink-0">
          {!showConfirm ? (
            <button
              onClick={() => setShowConfirm(true)}
              disabled={isRestoring}
              className="flex items-center gap-2 px-4 py-2.5 rounded-full bg-slate-200 dark:bg-slate-800 hover:bg-slate-300 dark:hover:bg-slate-700 text-slate-800 dark:text-slate-200 font-semibold text-xs transition-all active:scale-95 disabled:opacity-50"
            >
              <RotateCcw className="w-3.5 h-3.5 text-emerald-500" />
              <span>Restore Pre-CoreFlow Values</span>
            </button>
          ) : (
            <div className="flex items-center gap-2 p-1 rounded-full bg-amber-500/10 border border-amber-500/30">
              <span className="text-xs font-medium text-amber-500 pl-2">Confirm recovery?</span>
              <button
                onClick={handleConfirm}
                className="px-3 py-1 rounded-full bg-emerald-500 hover:bg-emerald-400 text-slate-950 font-bold text-xs transition-colors"
              >
                Restore
              </button>
              <button
                onClick={() => setShowConfirm(false)}
                className="px-2.5 py-1 rounded-full text-xs text-slate-400 hover:text-white"
              >
                Cancel
              </button>
            </div>
          )}
        </div>
      </div>

      {restoreFeedback && (
        <div className="mt-4 p-3 rounded-2xl bg-amber-500/10 border border-amber-500/30 flex items-center gap-2 text-xs text-emerald-600 dark:text-emerald-400 font-medium animate-fade-in">
          <CheckCircle2 className="w-4 h-4 shrink-0" />
          <span>{restoreFeedback}</span>
        </div>
      )}
    </div>
  );
};
