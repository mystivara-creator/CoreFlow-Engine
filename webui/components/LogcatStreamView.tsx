import React, { useMemo, useState } from 'react';
import { Download, FileSpreadsheet, RefreshCw, Search, Trash2 } from 'lucide-react';
import { ConscienceTraceRow } from '../bridge/daemonBridge';
import { exportHistoryToCsv } from '../engine/simulator';
import { nativeHistoryToCsv, saveDiagnosticFile, TelemetryRecord } from '../bridge/diagnostics';
import { LogcatEntry, RuntimeSample } from '../engine/types';

interface LogcatStreamViewProps {
  entries: LogcatEntry[];
  traceRows: ConscienceTraceRow[];
  nativeLogLines: string[];
  isNativeBridge: boolean;
  history?: RuntimeSample[];
  onRefreshLogs: () => Promise<void>;
  nativeHistory?: TelemetryRecord[];
  loading?: boolean;
  readError?: string | null;
  refreshedAt?: number | null;
  onClearLogs: () => void;
}

type StreamTab = 'trace' | 'logcat';
type TraceFilter = 'all' | 'block' | 'change' | 'safety' | 'error';

export const LogcatStreamView: React.FC<LogcatStreamViewProps> = ({
  entries,
  traceRows,
  nativeLogLines,
  isNativeBridge,
  history,
  onRefreshLogs,
  onClearLogs,
  nativeHistory = [],
  loading = false,
  readError,
  refreshedAt,
}) => {
  // Keep hook order [streamTab, traceFilter, query, busy, feedback] intact for test_webui_diagnostics.cjs
  const [streamTab, setStreamTab] = useState<StreamTab>('trace');
  const [traceFilter, setTraceFilter] = useState<TraceFilter>('all');
  const [query, setQuery] = useState('');
  const [busy, setBusy] = useState(false);
  const [feedback, setFeedback] = useState<{ error: boolean; text: string } | null>(null);

  const historyCount = isNativeBridge ? nativeHistory.length : history?.length ?? 0;

  const runAction = async (action: () => Promise<string>) => {
    setBusy(true);
    setFeedback(null);
    try {
      setFeedback({ error: false, text: await action() });
    } catch (err) {
      setFeedback({ error: true, text: String(err instanceof Error ? err.message : err) });
    } finally {
      setBusy(false);
    }
  };

  const filteredTrace = useMemo(() => {
    const C = window.CoreFlowControl;
    const base = C ? C.filterTrace(traceRows, traceFilter) : traceRows;
    const q = query.trim().toLowerCase();
    if (!q) return base;
    return base.filter(
      (r) =>
        r.event.toLowerCase().includes(q) ||
        r.scope.toLowerCase().includes(q) ||
        r.reason.toLowerCase().includes(q) ||
        r.target.toLowerCase().includes(q) ||
        r.result.toLowerCase().includes(q)
    );
  }, [traceRows, traceFilter, query]);

  const filteredLogcat = useMemo(() => {
    const q = query.trim().toLowerCase();
    if (isNativeBridge) {
      if (!q) return nativeLogLines;
      return nativeLogLines.filter((line) => line.toLowerCase().includes(q));
    }
    if (!q) return entries;
    return entries.filter(
      (e) =>
        e.message.toLowerCase().includes(q) ||
        e.tag.toLowerCase().includes(q) ||
        e.level.toLowerCase() === q
    );
  }, [entries, nativeLogLines, isNativeBridge, query]);

  const handleExport = () =>
    runAction(async () => {
      const payload =
        streamTab === 'trace'
          ? JSON.stringify(filteredTrace, null, 2)
          : isNativeBridge
            ? (filteredLogcat as string[]).join('\n')
            : (filteredLogcat as LogcatEntry[])
                .map((e) => `${e.timeFormatted} [tick=${e.sampleIndex}] ${e.level}/${e.tag}: ${e.message}`)
                .join('\n');
      const filename = `coreflow-${streamTab === 'trace' ? 'decision-trace' : 'daemon-logcat'}-${Date.now()}.${
        streamTab === 'trace' ? 'json' : 'txt'
      }`;
      return saveDiagnosticFile(
        filename,
        payload,
        streamTab === 'trace' ? 'application/json;charset=utf-8' : 'text/plain;charset=utf-8'
      );
    });

  const handleExportCsv = () =>
    runAction(async () => {
      if (!historyCount) throw new Error('No measured telemetry samples are available yet.');
      const content = isNativeBridge
        ? nativeHistoryToCsv(nativeHistory)
        : exportHistoryToCsv(history ?? []);
      return saveDiagnosticFile(
        `coreflow-telemetry-history-${Date.now()}.csv`,
        content,
        'text/csv;charset=utf-8'
      );
    });

  return (
    <div className="space-y-6 min-w-0">
      <div className="pb-4 border-b border-slate-200 flex flex-col sm:flex-row sm:items-end justify-between gap-4">
        <div>
          <h1 className="text-2xl font-bold tracking-tight text-slate-900">
            Conscience Journal &amp; Daemon Logcat
          </h1>
          <p className="text-sm text-slate-600 mt-1">
            Structured event audit from{' '}
            <span className="font-mono break-all">/data/adb/coreflow/decision_trace.jsonl</span> and tagged Android logcat, with supervisor stdout/stderr from{' '}
            <span className="font-mono break-all">logs/coreflowd.log</span>.
          </p>
        </div>

        <div className="flex flex-wrap items-center gap-2 shrink-0">
          <button
            type="button"
            disabled={busy || loading}
            onClick={() =>
              runAction(async () => {
                await onRefreshLogs();
                return isNativeBridge ? 'Journal and logcat refreshed.' : 'Preview records refreshed.';
              })
            }
            className="inline-flex items-center gap-1.5 px-3 py-1.5 text-xs font-semibold text-slate-700 bg-white border border-slate-200 rounded-lg hover:bg-slate-50 disabled:opacity-50 disabled:cursor-wait"
          >
            <RefreshCw className={`w-3.5 h-3.5 ${loading ? 'animate-spin' : ''}`} aria-hidden="true" />
            <span>{loading ? 'Loading…' : 'Reload'}</span>
          </button>
          <button
            type="button"
            onClick={handleExport}
            disabled={busy || loading}
            className="inline-flex items-center gap-1.5 px-3 py-1.5 text-xs font-semibold text-slate-700 bg-white border border-slate-200 rounded-lg hover:bg-slate-50 disabled:opacity-50 disabled:cursor-wait"
          >
            <Download className="w-3.5 h-3.5" aria-hidden="true" />
            <span>Export Logs</span>
          </button>
          {(isNativeBridge || historyCount > 0) && (
            <button
              type="button"
              onClick={handleExportCsv}
              disabled={busy || historyCount === 0}
              title="Export historical telemetry samples into CSV format for diagnostics"
              className="inline-flex items-center gap-1.5 px-3 py-1.5 text-xs font-semibold text-emerald-800 bg-emerald-50 border border-emerald-200 rounded-lg hover:bg-emerald-100 disabled:opacity-50 disabled:cursor-wait tabular-nums"
            >
              <FileSpreadsheet className="w-3.5 h-3.5 text-emerald-600" aria-hidden="true" />
              <span>Download History CSV ({historyCount})</span>
            </button>
          )}
          {!isNativeBridge && (
            <button
              type="button"
              onClick={onClearLogs}
              className="inline-flex items-center gap-1.5 px-3 py-1.5 text-xs font-medium text-slate-600 bg-white border border-slate-200 rounded-lg hover:bg-slate-50 disabled:opacity-50"
            >
              <Trash2 className="w-3.5 h-3.5" aria-hidden="true" />
              <span>Clear</span>
            </button>
          )}
        </div>
      </div>

      {readError && (
        <div role="alert" className="p-3 rounded-xl border border-red-200 text-xs break-words bg-red-50 text-red-800">
          {readError} · Previously loaded records may be stale.
        </div>
      )}
      {feedback && (
        <div
          role={feedback.error ? 'alert' : 'status'}
          className={`cf-view-stage p-3 rounded-xl border text-xs break-words ${
            feedback.error
              ? 'bg-red-50 border-red-200 text-red-800'
              : 'bg-emerald-50 border-emerald-200 text-emerald-800'
          }`}
        >
          {feedback.text}
        </div>
      )}
      <p className="text-xs text-slate-500" aria-live="polite">
        {isNativeBridge
          ? `Exports save to Download. History contains up to 300 measured samples from this WebUI session; unavailable values stay blank.${
              refreshedAt ? ` Last read: ${new Date(refreshedAt).toLocaleTimeString()}.` : ''
            }`
          : 'Browser preview: records and telemetry are simulated.'}
      </p>

      {isNativeBridge &&
        traceRows.some((row) => row.event === 'mutation_journal_commit' && row.result === 'FAILED') && (
          <div className="p-3 text-xs bg-amber-50 border border-amber-200 text-amber-900 rounded-xl">
            A recovery journal commit failed. The daemon blocked that actuator write; export the journal and logcat to diagnose the failure.
          </div>
        )}

      {/* Sliding Mode Switch + Filters */}
      <div className="cf-stagger-1 flex flex-col md:flex-row md:items-center justify-between gap-3">
        <div className="flex flex-wrap items-center gap-2">
          <div className="relative grid grid-cols-2 p-1 bg-slate-100 rounded-xl border border-slate-200/80 min-w-[290px]">
            <span
              aria-hidden="true"
              className={`pointer-events-none absolute top-1 bottom-1 left-1 w-[calc(50%-0.25rem)] rounded-lg bg-white shadow-xs border border-slate-200/80 transition-transform duration-300 ease-[cubic-bezier(0.16,1,0.3,1)] ${
                streamTab === 'trace' ? 'translate-x-0' : 'translate-x-full'
              }`}
            />
            <button
              type="button"
              onClick={() => setStreamTab('trace')}
              className={`relative z-10 px-3 py-1.5 text-xs font-semibold rounded-lg tabular-nums ${
                streamTab === 'trace' ? 'text-slate-900' : 'text-slate-600 hover:text-slate-900'
              }`}
            >
              Conscience Trace ({traceRows.length})
            </button>
            <button
              type="button"
              onClick={() => setStreamTab('logcat')}
              className={`relative z-10 px-3 py-1.5 text-xs font-semibold rounded-lg tabular-nums ${
                streamTab === 'logcat' ? 'text-slate-900' : 'text-slate-600 hover:text-slate-900'
              }`}
            >
              Daemon Logcat ({isNativeBridge ? nativeLogLines.length : entries.length})
            </button>
          </div>

          {streamTab === 'trace' && (
            <div className="flex flex-wrap items-center gap-1">
              {(
                [
                  ['all', 'All Events'],
                  ['change', 'Verified Writes'],
                  ['block', 'Policy Denials'],
                  ['safety', 'Safety / Rollbacks'],
                  ['error', 'Errors'],
                ] as const
              ).map(([key, label]) => (
                <button
                  key={key}
                  type="button"
                  onClick={() => setTraceFilter(key)}
                  className={`px-2.5 py-1.5 text-xs font-semibold rounded-lg ${
                    traceFilter === key
                      ? 'bg-slate-900 text-white'
                      : 'bg-white border border-slate-200 text-slate-600 hover:text-slate-900'
                  }`}
                >
                  {label}
                </button>
              ))}
            </div>
          )}
        </div>

        <div className="relative w-full md:w-72">
          <Search
            className="w-3.5 h-3.5 text-slate-400 absolute left-3 top-1/2 -translate-y-1/2"
            aria-hidden="true"
          />
          <input
            type="search"
            value={query}
            onChange={(e) => setQuery(e.target.value)}
            placeholder="Filter event, path, or reason..."
            className="w-full pl-8 pr-3 py-1.5 text-xs bg-white border border-slate-200 rounded-lg text-slate-900 placeholder:text-slate-400 focus:outline-none focus:ring-2 focus:ring-blue-600"
          />
        </div>
      </div>

      {/* Content */}
      {streamTab === 'trace' ? (
        <div className="cf-stagger-2 cf-card bg-white border border-slate-200 rounded-xl overflow-hidden">
          {filteredTrace.length === 0 ? (
            <div className="p-10 text-center space-y-2">
              <div className="text-sm font-semibold text-slate-900">
                No matching Conscience trace events
              </div>
              <p className="text-xs text-slate-500 max-w-md mx-auto">
                Events from <span className="font-mono">decision_trace.jsonl</span> appear here as{' '}
                <span className="font-mono">coreflowd</span> evaluates scope gates, verifies sysfs readbacks, or enforces safety holds.
              </p>
              {traceFilter !== 'all' && (
                <button
                  type="button"
                  onClick={() => setTraceFilter('all')}
                  className="mt-2 inline-flex items-center px-3 py-1.5 text-xs font-semibold text-blue-700 bg-blue-50 rounded-lg hover:bg-blue-100"
                >
                  Show all trace events
                </button>
              )}
            </div>
          ) : (
            <>
              <div className="md:hidden divide-y divide-slate-200">
                {filteredTrace.map((row, idx) => (
                  <article key={`mobile-${idx}`} className="p-4 space-y-2 text-xs min-w-0">
                    <div className="flex flex-wrap justify-between gap-2 font-mono text-slate-500 tabular-nums">
                      <span>{row.time || '—'}</span>
                      <span
                        className={
                          row.kind === 'error'
                            ? 'text-red-700 font-semibold'
                            : row.kind === 'safety'
                              ? 'text-amber-700 font-semibold'
                              : row.kind === 'change'
                                ? 'text-emerald-700 font-semibold'
                                : 'text-slate-700'
                        }
                      >
                        {row.result}
                      </span>
                    </div>
                    <div className="font-semibold break-all text-slate-900">{row.event}</div>
                    <div className="text-slate-500 break-all">Scope: {row.scope || 'runtime'}</div>
                    <div className="break-words [overflow-wrap:anywhere] text-slate-700">
                      {row.reason || '—'}
                    </div>
                    {(row.target || row.requested || row.baselineValue || row.currentValue) && (
                      <details className="text-slate-600">
                        <summary className="cursor-pointer font-medium text-blue-700">
                          Target and values
                        </summary>
                        <dl className="mt-2 space-y-1 font-mono break-all bg-slate-50 p-2.5 rounded-lg border border-slate-200">
                          <dt className="text-slate-500">Target</dt>
                          <dd>{row.target || '—'}</dd>
                          <dt className="text-slate-500">Requested</dt>
                          <dd>{row.requested || '—'}</dd>
                          <dt className="text-slate-500">Before</dt>
                          <dd>{row.currentValue || '—'}</dd>
                          <dt className="text-slate-500">Factory baseline</dt>
                          <dd>{row.baselineValue || '—'}</dd>
                          <dt className="text-slate-500">Writes verified / attempted</dt>
                          <dd>
                            {row.writesVerified ?? '—'} / {row.writesAttempted ?? '—'}
                          </dd>
                        </dl>
                      </details>
                    )}
                  </article>
                ))}
              </div>

              <div className="hidden md:block overflow-x-auto">
                <table className="w-full text-left border-collapse">
                  <thead>
                    <tr className="border-b border-slate-200 bg-slate-50/70 text-[11px] font-semibold uppercase tracking-wider text-slate-500">
                      <th className="py-2.5 px-4 w-24">Time</th>
                      <th className="py-2.5 px-4">Conscience Event</th>
                      <th className="py-2.5 px-4">Scope</th>
                      <th className="py-2.5 px-4">Result</th>
                      <th className="py-2.5 px-4">Target &amp; Audit Rationale</th>
                    </tr>
                  </thead>
                  <tbody className="divide-y divide-slate-100 text-xs">
                    {filteredTrace.map((row, idx) => {
                      const desc = window.CoreFlowControl
                        ? window.CoreFlowControl.describeEvent(row.event)
                        : row.event;
                      const toneColor =
                        row.kind === 'error'
                          ? 'text-red-700 font-semibold'
                          : row.kind === 'safety'
                            ? 'text-amber-700 font-semibold'
                            : row.kind === 'change'
                              ? 'text-emerald-700 font-semibold'
                              : row.kind === 'block'
                                ? 'text-slate-600'
                                : 'text-slate-800';

                      return (
                        <tr
                          key={`${row.time}-${row.event}-${idx}`}
                          className="hover:bg-slate-50/80 transition-colors"
                        >
                          <td className="py-2.5 px-4 font-mono text-slate-500 tabular-nums whitespace-nowrap">
                            {row.time || '—'}
                          </td>
                          <td className="py-2.5 px-4">
                            <div className="font-medium text-slate-900">{desc}</div>
                            <div className="font-mono text-[11px] text-slate-400">{row.event}</div>
                          </td>
                          <td className="py-2.5 px-4 font-mono text-slate-600 whitespace-nowrap">
                            {row.scope || 'runtime'}
                          </td>
                          <td className={`py-2.5 px-4 font-mono whitespace-nowrap ${toneColor}`}>
                            {row.result}
                          </td>
                          <td className="py-2.5 px-4 text-slate-700 space-y-0.5 break-all min-w-64">
                            <div>{row.reason || '—'}</div>
                            {row.target && (
                              <div className="font-mono text-[11px] text-slate-500">
                                {row.target}
                                {row.requested ? ` -> ${row.requested}` : ''}
                                {row.baselineValue ? ` (baseline=${row.baselineValue})` : ''}
                              </div>
                            )}
                          </td>
                        </tr>
                      );
                    })}
                  </tbody>
                </table>
              </div>
            </>
          )}
        </div>
      ) : (
        <div className="cf-stagger-2 bg-slate-950 text-slate-100 border border-slate-800 rounded-xl overflow-hidden">
          <div className="px-4 py-2.5 border-b border-slate-800 flex items-center justify-between text-xs font-mono text-slate-400">
            <span>
              {isNativeBridge
                ? 'Tagged Android logcat + supervisor output'
                : 'android/log.h · CoreFlowAutonomous / CoreFlowConscience'}
            </span>
            <span className="tabular-nums">
              {Array.isArray(filteredLogcat) ? filteredLogcat.length : 0} lines
            </span>
          </div>
          <div className="p-4 max-h-[540px] overflow-y-auto font-mono text-xs space-y-1.5">
            {isNativeBridge && filteredLogcat.length === 0 ? (
              <div className="py-8 text-center text-slate-400">
                No native logcat lines match the current filter.
              </div>
            ) : isNativeBridge ? (
              (filteredLogcat as string[]).map((line, idx) => (
                <div key={`nlog-${idx}`} className="text-slate-200 break-all">
                  {line}
                </div>
              ))
            ) : (filteredLogcat as LogcatEntry[]).length === 0 ? (
              <div className="py-8 text-center text-slate-500 font-sans">
                No logcat lines match the current filter.
              </div>
            ) : (
              (filteredLogcat as LogcatEntry[]).map((entry) => {
                const levelColor =
                  entry.level === 'E'
                    ? 'text-red-400'
                    : entry.level === 'W'
                      ? 'text-amber-300'
                      : 'text-emerald-400';
                return (
                  <div key={entry.id} className="flex items-start gap-2.5 leading-relaxed">
                    <span className="text-slate-500 shrink-0 tabular-nums">
                      {entry.timeFormatted}
                    </span>
                    <span className="text-slate-600 shrink-0 tabular-nums">
                      [#{entry.sampleIndex}]
                    </span>
                    <span className={`font-bold shrink-0 ${levelColor}`}>
                      {entry.level}/{entry.tag}:
                    </span>
                    <span className="text-slate-200 break-words">{entry.message}</span>
                  </div>
                );
              })
            )}
          </div>
        </div>
      )}
    </div>
  );
};
