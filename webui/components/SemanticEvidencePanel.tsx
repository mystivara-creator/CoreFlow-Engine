import React from 'react';
import {
  AlertCircle,
  CheckCircle2,
  Clock3,
  Fingerprint,
  ShieldAlert,
  ShieldCheck,
} from 'lucide-react';
import { SemanticEvidence } from '../semanticEvidence';

interface Props {
  evidence: SemanticEvidence | null;
  nativeConnected: boolean;
  simulationMode: boolean;
}

const statusClass = (state: 'candidate' | 'validated') =>
  state === 'validated'
    ? 'bg-emerald-100 text-emerald-800 dark:bg-emerald-950/60 dark:text-emerald-300'
    : 'bg-amber-100 text-amber-800 dark:bg-amber-950/60 dark:text-amber-300';

export const SemanticEvidencePanel: React.FC<Props> = ({
  evidence,
  nativeConnected,
  simulationMode,
}) => (
  <section aria-label="Native semantic evidence" className="m3-card p-5 sm:p-6 space-y-4">
    <div className="flex flex-wrap items-start justify-between gap-3">
      <div className="flex items-center gap-2">
        <Fingerprint className="w-4 h-4 text-[#0b57d0] dark:text-[#a8c7fa]" />
        <div>
          <h3 className="text-sm font-semibold text-slate-900 dark:text-slate-100">
            Native Semantic Evidence
          </h3>
          <p className="text-xs text-slate-500 dark:text-slate-400">
            Daemon-owned CapabilityResolver assessment · schema v1
          </p>
        </div>
      </div>
      {nativeConnected ? (
        <span className="inline-flex items-center gap-1.5 rounded-full px-2.5 py-1 text-[11px] font-semibold bg-emerald-100 text-emerald-800 dark:bg-emerald-950/60 dark:text-emerald-300">
          <CheckCircle2 className="w-3.5 h-3.5" /> Connected
        </span>
      ) : (
        <span className="inline-flex items-center gap-1.5 rounded-full px-2.5 py-1 text-[11px] font-semibold bg-amber-100 text-amber-800 dark:bg-amber-950/60 dark:text-amber-300">
          <AlertCircle className="w-3.5 h-3.5" />
          {simulationMode ? 'Simulation Mode' : 'Awaiting Official Data'}
        </span>
      )}
    </div>

    {simulationMode ? (
      <div role="note" className="rounded-xl border border-amber-300/70 bg-amber-50 px-4 py-3 text-xs leading-relaxed text-amber-900 dark:border-amber-800/70 dark:bg-amber-950/30 dark:text-amber-200">
        <strong>Simulation Mode.</strong> Preview metrics are synthetic and labelled (sim).
        No kernel semantic binding or audit evidence is inferred in the browser.
      </div>
    ) : evidence?.status === 'unavailable' ? (
      <div className="rounded-xl bg-slate-100/80 dark:bg-white/[0.04] px-4 py-3 flex gap-2.5 text-xs text-slate-600 dark:text-slate-300">
        <ShieldAlert className="w-4 h-4 shrink-0 text-amber-600 dark:text-amber-400" />
        <span>
          The daemon returned schema v1 with status unavailable because the adapter has
          not produced semantic evidence. No binding state or functional domain is fabricated.
        </span>
      </div>
    ) : !nativeConnected ? (
      <div className="rounded-xl bg-slate-100/80 dark:bg-white/[0.04] px-4 py-3 text-xs text-slate-600 dark:text-slate-300">
        Native bridge detected, but an official schema v1 daemon status payload has not
        been received. Semantic bindings remain unavailable until the daemon responds.
      </div>
    ) : !evidence ? (
      <div className="rounded-xl bg-slate-100/80 dark:bg-white/[0.04] px-4 py-3 text-xs text-slate-600 dark:text-slate-300">
        No official semantic telemetry is available.
      </div>
    ) : (
      <>
        <div className="grid grid-cols-2 sm:grid-cols-4 gap-2 text-xs">
          <div className="rounded-xl bg-slate-100/80 dark:bg-white/[0.04] p-3">
            <div className="text-slate-500 dark:text-slate-400">Generation</div>
            <div className="mt-1 font-mono font-semibold tabular-nums text-slate-900 dark:text-slate-100">
              {evidence.generation ?? 'N/A'}
            </div>
          </div>
          <div className="rounded-xl bg-slate-100/80 dark:bg-white/[0.04] p-3">
            <div className="text-slate-500 dark:text-slate-400">Observed · monotonic ns</div>
            <div className="mt-1 font-mono font-semibold tabular-nums text-slate-900 dark:text-slate-100 break-all">
              {evidence.observedAtMonotonicNs ?? 'N/A'}
            </div>
          </div>
          <div className="rounded-xl bg-slate-100/80 dark:bg-white/[0.04] p-3">
            <div className="text-slate-500 dark:text-slate-400">Age</div>
            <div className="mt-1 font-mono font-semibold tabular-nums text-slate-900 dark:text-slate-100">
              {evidence.ageMs === null ? 'N/A' : evidence.ageMs.toFixed(3) + ' ms'}
            </div>
          </div>
          <div className="rounded-xl bg-slate-100/80 dark:bg-white/[0.04] p-3">
            <div className="text-slate-500 dark:text-slate-400">Freshness</div>
            <div className="mt-1 flex items-center gap-1.5 font-semibold">
              <Clock3 className="w-3.5 h-3.5" />
              <span className={evidence.freshness === 'fresh' ? 'text-emerald-700 dark:text-emerald-300' : 'text-amber-700 dark:text-amber-300'}>
                {evidence.freshness}
              </span>
            </div>
          </div>
        </div>

        <div className="space-y-2">
          {evidence.bindings.map((binding, index) => (
            <article
              key={binding.generation + '-' + binding.observedAtMonotonicNs + '-' + index}
              className="rounded-xl border border-slate-900/[0.06] dark:border-white/[0.08] p-3.5 space-y-3"
            >
              <div className="flex flex-wrap items-center justify-between gap-2">
                <div className="flex items-center gap-2 text-xs font-semibold text-slate-800 dark:text-slate-100">
                  <Fingerprint className="w-3.5 h-3.5" />
                  Semantic Binding {index + 1}
                </div>
                <span className={'rounded-full px-2.5 py-1 text-[10px] font-bold uppercase tracking-wide ' + statusClass(binding.bindingState)}>
                  {binding.bindingState}
                </span>
              </div>
              <div className="grid grid-cols-2 sm:grid-cols-4 gap-3 text-xs">
                <div>
                  <div className="text-slate-500 dark:text-slate-400">Domain</div>
                  <div className="mt-1 font-semibold text-slate-900 dark:text-slate-100">{binding.domain}</div>
                </div>
                <div>
                  <div className="text-slate-500 dark:text-slate-400">Assessment</div>
                  <div className="mt-1 font-semibold text-slate-900 dark:text-slate-100">{binding.domainAssessment.state}</div>
                </div>
                <div>
                  <div className="text-slate-500 dark:text-slate-400">Confidence</div>
                  <div className="mt-1 font-mono font-semibold tabular-nums text-slate-900 dark:text-slate-100">
                    {(binding.domainAssessment.confidence * 100).toFixed(1)}%
                  </div>
                </div>
                <div>
                  <div className="text-slate-500 dark:text-slate-400">Corroborating relations</div>
                  <div className="mt-1 font-mono font-semibold tabular-nums text-slate-900 dark:text-slate-100">
                    {binding.domainAssessment.corroboratingRelations}/3
                  </div>
                </div>
              </div>
              <div className="flex flex-wrap items-center justify-between gap-2 border-t border-slate-900/[0.05] dark:border-white/[0.06] pt-2 text-[11px] text-slate-500 dark:text-slate-400">
                <span>
                  Binding freshness: <strong className={binding.freshness === 'fresh' ? 'text-emerald-700 dark:text-emerald-300' : 'text-amber-700 dark:text-amber-300'}>{binding.freshness}</strong>
                  {binding.ageMs === null ? '' : ' · ' + binding.ageMs.toFixed(3) + ' ms'}
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
