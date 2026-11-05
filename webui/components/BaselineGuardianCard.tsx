import React, { useState } from 'react';
import { ShieldCheck, RotateCcw, CheckCircle2 } from 'lucide-react';

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
    <div className="m3-card p-6 sm:p-8">
      <div className="flex flex-col lg:flex-row lg:items-center justify-between gap-6">
        <div className="flex items-start gap-4 max-w-2xl">
          <div className="w-11 h-11 rounded-2xl bg-[#d3e3fd] dark:bg-[#0842a0] text-[#041e49] dark:text-[#d3e3fd] flex items-center justify-center shrink-0">
            <ShieldCheck className="w-5 h-5" />
          </div>
          <div className="space-y-1.5">
            <div className="flex flex-wrap items-center gap-2">
              <h3 className="font-semibold text-base sm:text-lg text-slate-900 dark:text-slate-100">
                Ownership Guard & Idempotent Baseline Recovery
              </h3>
              <span className="text-xs text-slate-400" aria-hidden="true">·</span>
              <span className="text-xs font-mono text-[#0b57d0] dark:text-[#a8c7fa]">
                {nativeMode
                  ? 'Write-Ahead Journal CFJ4'
                  : baselineStatus === 'protected'
                  ? 'Journal CFJ4 Armed'
                  : 'Baseline Restored'}
              </span>
            </div>
            <p className="text-xs sm:text-sm text-slate-600 dark:text-slate-400 leading-relaxed">
              {nativeMode
                ? 'Records factory & prior baseline values, validated boot_id/boot_epoch, object identity (st_dev:st_ino), and last verified write prior to mutation. Rollback restores only values still owned by CoreFlow and never overwrites external OEM/Android adjustments.'
                : `Tracking ${nodesSnapshotted} capability baselines with CFJ4 st_dev:st_ino ownership & boot_epoch verification; zero physical writes in browser preview.`}
            </p>
          </div>
        </div>

        {/* M3 Tonal / Confirmation Action */}
        <div className="self-start lg:self-center shrink-0">
          {!showConfirm ? (
            <button
              onClick={() => setShowConfirm(true)}
              disabled={isRestoring}
              className="inline-flex items-center gap-2 px-5 py-2.5 rounded-full bg-[#d3e3fd] hover:bg-[#c2d7fa] text-[#041e49] dark:bg-[#0842a0] dark:hover:bg-[#0b57d0] dark:text-[#d3e3fd] font-medium text-xs disabled:opacity-50"
            >
              <RotateCcw className="w-3.5 h-3.5" />
              <span>Restore OEM Baseline</span>
            </button>
          ) : (
            <div className="inline-flex items-center gap-2 p-1.5 rounded-full bg-slate-900/[0.06] dark:bg-white/[0.08]">
              <span className="text-xs font-medium text-slate-700 dark:text-slate-200 pl-3">
                Confirm rollback?
              </span>
              <button
                onClick={handleConfirm}
                className="px-4 py-1.5 rounded-full bg-[#0b57d0] dark:bg-[#a8c7fa] text-white dark:text-[#062e6f] font-semibold text-xs"
              >
                Restore
              </button>
              <button
                onClick={() => setShowConfirm(false)}
                className="px-3 py-1.5 rounded-full text-xs text-slate-600 dark:text-slate-400 hover:text-slate-900 dark:hover:text-white"
              >
                Cancel
              </button>
            </div>
          )}
        </div>
      </div>

      {restoreFeedback && (
        <div className="mt-5 p-4 m3-tonal-block flex items-center gap-2.5 text-xs text-slate-800 dark:text-slate-200 font-medium">
          <CheckCircle2 className="w-4 h-4 text-[#0b57d0] dark:text-[#a8c7fa] shrink-0" />
          <span>{restoreFeedback}</span>
        </div>
      )}
    </div>
  );
};
