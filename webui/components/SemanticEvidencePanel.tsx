import React from 'react';
import {
  AlertCircle,
  CheckCircle2,
  Clock3,
  Fingerprint,
  Network,
  ShieldAlert,
  ShieldCheck,
} from 'lucide-react';
import { SemanticDescriptorState, SemanticEvidence } from '../semanticEvidence';

interface Props {
  evidence: SemanticEvidence | null;
  nativeConnected: boolean;
  simulationMode?: boolean;
}

const badgeForBinding = (state: SemanticDescriptorState): string =>
  state === 'validated' ? 'md3-badge md3-badge-success' : 'md3-badge md3-badge-warning';

export const SemanticEvidencePanel: React.FC<Props> = ({
  evidence,
  nativeConnected,
  simulationMode = false,
}) => (
  <section
    aria-label="Native semantic evidence"
    className="md3-card-elevated p-6 sm:p-7 space-y-5"
  >
    <div className="flex flex-wrap items-start justify-between gap-3">
      <div className="flex items-center gap-3.5">
        <div
          className="w-11 h-11 rounded-2xl flex items-center justify-center shrink-0"
          style={{
            backgroundColor: 'var(--md-sys-color-primary-container)',
            color: 'var(--md-sys-color-on-primary-container)',
          }}
        >
          <Network className="w-5 h-5" />
        </div>
        <div>
          <h3 className="text-base font-bold tracking-tight">
            Native Semantic Evidence
          </h3>
          <p className="text-xs" style={{ color: 'var(--md-sys-color-on-surface-variant)' }}>
            Daemon-owned CapabilityResolver &amp; 3-relation DomainEvidenceAggregator · Schema v1
          </p>
        </div>
      </div>

      {nativeConnected ? (
        <span className="md3-badge md3-badge-success">
          <CheckCircle2 className="w-3.5 h-3.5" /> Connected
        </span>
      ) : (
        <span className="md3-badge md3-badge-warning">
          <AlertCircle className="w-3.5 h-3.5" />
          {simulationMode ? 'Simulation Mode' : 'Awaiting Official Data'}
        </span>
      )}
    </div>

    {simulationMode ? (
      <div
        role="note"
        className="rounded-2xl px-4 py-3.5 text-xs leading-relaxed"
        style={{
          backgroundColor: 'var(--md-sys-color-warning-container)',
          color: 'var(--md-sys-color-on-warning-container)',
        }}
      >
        <strong>Simulation Mode.</strong> Browser preview metrics are synthetic and labelled <code className="font-mono">(sim)</code>. No kernel semantic binding or corroboration evidence is fabricated in the browser.
      </div>
    ) : evidence?.status === 'unavailable' ? (
      <div className="md3-card-filled px-4 py-3.5 flex gap-3 text-xs leading-relaxed">
        <ShieldAlert
          className="w-4 h-4 shrink-0 mt-0.5"
          style={{ color: 'var(--md-sys-color-warning)' }}
        />
        <span style={{ color: 'var(--md-sys-color-on-surface-variant)' }}>
          The daemon returned schema v1 with status <code className="font-mono">unavailable</code> because the discovery adapter has not produced semantic evidence on this node set. No binding state or functional domain is fabricated.
        </span>
      </div>
    ) : !nativeConnected ? (
      <div
        className="md3-card-filled px-4 py-3.5 text-xs leading-relaxed"
        style={{ color: 'var(--md-sys-color-on-surface-variant)' }}
      >
        Native bridge detected, but an official schema v1 daemon status payload has not yet been received. Semantic bindings remain unavailable until the daemon responds.
      </div>
    ) : !evidence ? (
      <div
        className="md3-card-filled px-4 py-3.5 text-xs"
        style={{ color: 'var(--md-sys-color-on-surface-variant)' }}
      >
        No official semantic telemetry is available.
      </div>
    ) : (
      <>
        <div className="grid grid-cols-2 sm:grid-cols-4 gap-3 text-xs">
          <div className="md3-card-filled p-3.5">
            <div style={{ color: 'var(--md-sys-color-on-surface-variant)' }}>Generation</div>
            <div className="mt-1 font-mono text-sm font-bold tabular-nums">
              {evidence.generation ?? 'N/A'}
            </div>
          </div>
          <div className="md3-card-filled p-3.5">
            <div style={{ color: 'var(--md-sys-color-on-surface-variant)' }}>Observed · monotonic ns</div>
            <div className="mt-1 font-mono text-sm font-bold tabular-nums break-all">
              {evidence.observedAtMonotonicNs ?? 'N/A'}
            </div>
          </div>
          <div className="md3-card-filled p-3.5">
            <div style={{ color: 'var(--md-sys-color-on-surface-variant)' }}>Age</div>
            <div className="mt-1 font-mono text-sm font-bold tabular-nums">
              {evidence.ageMs === null ? 'N/A' : `${evidence.ageMs.toFixed(3)} ms`}
            </div>
          </div>
          <div className="md3-card-filled p-3.5">
            <div style={{ color: 'var(--md-sys-color-on-surface-variant)' }}>Freshness</div>
            <div className="mt-1 flex items-center gap-1.5 text-sm font-bold">
              <Clock3 className="w-3.5 h-3.5" />
              <span
                style={{
                  color:
                    evidence.freshness === 'fresh'
                      ? 'var(--md-sys-color-success)'
                      : 'var(--md-sys-color-warning)',
                }}
              >
                {evidence.freshness}
              </span>
            </div>
          </div>
        </div>

        <div className="space-y-2.5">
          {evidence.bindings.map((binding, index) => (
            <article
              key={`${binding.generation}-${binding.observedAtMonotonicNs}-${index}`}
              className="md3-card-outlined p-4 space-y-3"
            >
              <div className="flex flex-wrap items-center justify-between gap-2">
                <div className="flex items-center gap-2 text-xs font-bold">
                  <Fingerprint className="w-3.5 h-3.5" style={{ color: 'var(--md-sys-color-primary)' }} />
                  <span>Semantic Binding {index + 1}</span>
                </div>
                <span className={badgeForBinding(binding.bindingState)}>
                  {binding.bindingState.toUpperCase()}
                </span>
              </div>

              <div className="grid grid-cols-2 sm:grid-cols-4 gap-3 text-xs">
                <div>
                  <div style={{ color: 'var(--md-sys-color-on-surface-variant)' }}>Domain</div>
                  <div className="mt-1 font-bold uppercase">{binding.domain}</div>
                </div>
                <div>
                  <div style={{ color: 'var(--md-sys-color-on-surface-variant)' }}>Assessment</div>
                  <div className="mt-1 font-bold">{binding.domainAssessment.state}</div>
                </div>
                <div>
                  <div style={{ color: 'var(--md-sys-color-on-surface-variant)' }}>Confidence</div>
                  <div className="mt-1 font-mono font-bold tabular-nums">
                    {(binding.domainAssessment.confidence * 100).toFixed(1)}%
                  </div>
                </div>
                <div>
                  <div style={{ color: 'var(--md-sys-color-on-surface-variant)' }}>Corroborating relations</div>
                  <div className="mt-1 font-mono font-bold tabular-nums">
                    {binding.domainAssessment.corroboratingRelations}/3
                  </div>
                </div>
              </div>

              <div
                className="flex flex-wrap items-center justify-between gap-2 pt-2.5 text-[11px]"
                style={{
                  borderTop: '1px solid var(--md-sys-color-outline-variant)',
                  color: 'var(--md-sys-color-on-surface-variant)',
                }}
              >
                <span>
                  Binding freshness:{' '}
                  <strong
                    style={{
                      color:
                        binding.freshness === 'fresh'
                          ? 'var(--md-sys-color-success)'
                          : 'var(--md-sys-color-warning)',
                    }}
                  >
                    {binding.freshness}
                  </strong>
                  {binding.ageMs === null ? '' : ` · ${binding.ageMs.toFixed(3)} ms`}
                </span>
                <span className="inline-flex items-center gap-1">
                  <ShieldCheck className="w-3.5 h-3.5" />
                  Semantic validation is not mutation authority
                </span>
              </div>
            </article>
          ))}
        </div>
      </>
    )}
  </section>
);
