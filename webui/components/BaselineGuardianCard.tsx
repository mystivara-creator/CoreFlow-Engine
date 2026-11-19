import React from 'react';
import { Fingerprint, LockKeyhole, RefreshCw, ShieldCheck } from 'lucide-react';

interface Props {
  nodesSnapshotted: number;
  nativeMode: boolean;
  baselineStatus: 'restored' | 'protected';
}

export const BaselineGuardianCard: React.FC<Props> = ({
  nodesSnapshotted,
  nativeMode,
  baselineStatus,
}) => (
  <section
    className="md3-card-elevated space-y-5 p-6 sm:p-7"
    aria-label="Baseline ownership and recovery"
  >
    <div className="flex flex-col sm:flex-row sm:items-start justify-between gap-3">
      <div className="flex items-start gap-3.5">
        <div
          className="flex h-11 w-11 shrink-0 items-center justify-center rounded-2xl"
          style={{
            backgroundColor: 'var(--md-sys-color-primary-container)',
            color: 'var(--md-sys-color-on-primary-container)',
          }}
        >
          <ShieldCheck className="h-5 w-5" />
        </div>
        <div className="min-w-0 flex-1">
          <div className="flex flex-wrap items-center gap-2">
            <h3 className="text-base font-bold tracking-tight">
              Ownership Guard &amp; Baseline Recovery
            </h3>
            <span
              className={
                nativeMode
                  ? 'md3-badge md3-badge-success'
                  : 'md3-badge md3-badge-warning'
              }
            >
              {nativeMode ? 'NATIVE CFJ4 JOURNAL' : 'PREVIEW MODEL'}
            </span>
          </div>
          <p
            className="mt-1 text-xs leading-relaxed"
            style={{ color: 'var(--md-sys-color-on-surface-variant)' }}
          >
            CoreFlow snapshots factory and pre-mutation values into a write-ahead journal before any write, then restores only nodes whose inode and active value prove continued ownership.
          </p>
        </div>
      </div>
    </div>

    <div className="grid grid-cols-1 gap-3 sm:grid-cols-3">
      <div className="md3-card-filled p-4 space-y-1.5">
        <div className="flex items-center justify-between text-xs" style={{ color: 'var(--md-sys-color-on-surface-variant)' }}>
          <span>Journal Format</span>
          <Fingerprint className="h-4 w-4" style={{ color: 'var(--md-sys-color-primary)' }} />
        </div>
        <div className="font-mono text-sm font-bold">CFJ4 · Write-Ahead</div>
        <div className="text-[11px]" style={{ color: 'var(--md-sys-color-on-surface-variant)' }}>
          Inode + boot_id bound
        </div>
      </div>

      <div className="md3-card-filled p-4 space-y-1.5">
        <div className="flex items-center justify-between text-xs" style={{ color: 'var(--md-sys-color-on-surface-variant)' }}>
          <span>Ownership State</span>
          <LockKeyhole className="h-4 w-4" style={{ color: 'var(--md-sys-color-primary)' }} />
        </div>
        <div className="text-sm font-bold">
          {baselineStatus === 'protected' ? 'Recovery Protected' : 'Clean · Baseline Intact'}
        </div>
        <div className="text-[11px]" style={{ color: 'var(--md-sys-color-on-surface-variant)' }}>
          External OEM writes respected
        </div>
      </div>

      <div className="md3-card-filled p-4 space-y-1.5">
        <div className="flex items-center justify-between text-xs" style={{ color: 'var(--md-sys-color-on-surface-variant)' }}>
          <span>Verified Snapshots</span>
          <RefreshCw className="h-4 w-4" style={{ color: 'var(--md-sys-color-primary)' }} />
        </div>
        <div className="font-mono text-sm font-bold tabular-nums">
          {nativeMode ? nodesSnapshotted : `${nodesSnapshotted} (sim)`}
        </div>
        <div className="text-[11px]" style={{ color: 'var(--md-sys-color-on-surface-variant)' }}>
          Idempotent rollback ready
        </div>
      </div>
    </div>

    <p
      className="pt-3 text-[11px] leading-relaxed"
      style={{
        borderTop: '1px solid var(--md-sys-color-outline-variant)',
        color: 'var(--md-sys-color-on-surface-variant)',
      }}
    >
      Manual kernel policy sliders are intentionally absent. When a safety rollback is needed, the native recovery lifecycle and module kill switch remain authoritative.
    </p>
  </section>
);
