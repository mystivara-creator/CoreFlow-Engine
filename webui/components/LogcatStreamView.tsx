import React, { useMemo, useState } from 'react';
import { Download, FileSpreadsheet, RefreshCw, Search, Trash2 } from 'lucide-react';
import { ConscienceTraceRow } from '../bridge/daemonBridge';
import { exportHistoryToCsv } from '../engine/simulator';
import { LogcatEntry, RuntimeSample } from '../engine/types';

interface LogcatStreamViewProps {
  entries: LogcatEntry[];
  traceRows: ConscienceTraceRow[];
  nativeLogLines: string[];
  isNativeBridge: boolean;
  history?: RuntimeSample[];
  onRefreshLogs: () => void;
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
}) => {
  const [streamTab, setStreamTab] = useState<StreamTab>('trace');
  const [traceFilter, setTraceFilter] = useState<TraceFilter>('all');
  const [query, setQuery] = useState('');

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
    if (isNativeBridge && nativeLogLines.length > 0) {
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

  const handleExport = () => {
    const payload =
      streamTab === 'trace'
        ? JSON.stringify(filteredTrace, null, 2)
        : isNativeBridge && nativeLogLines.length > 0
          ? nativeLogLines.join('\n')
          : entries
              .map(
                (e) =>
                  `${e.timeFormatted} [tick=${e.sampleIndex}] ${e.level}/${e.tag}: ${e.message}`
              )
              .join('\n');

    const blob = new Blob([payload], { type: 'text/plain;charset=utf-8' });
    const url = URL.createObjectURL(blob);
    const a = document.createElement('a');
    a.href = url;
    a.download =
      streamTab === 'trace'
        ? 'coreflow-decision-trace.json'
        : 'coreflowd-logcat.txt';
    document.body.appendChild(a);
    a.click();
    document.body.removeChild(a);
    URL.revokeObjectURL(url);
  };

  const handleExportCsv = () => {
    if (!history || history.length === 0) return;
    const csvContent = exportHistoryToCsv(history);
    const blob = new Blob([csvContent], { type: 'text/csv;charset=utf-8' });
    const url = URL.createObjectURL(blob);
    const a = document.createElement('a');
    a.href = url;
    a.download = `coreflow-telemetry-history-${Date.now()}.csv`;
    document.body.appendChild(a);
    a.click();
    document.body.removeChild(a);
    URL.revokeObjectURL(url);
  };

  return (
    <div className="space-y-6">
      <div className="pb-4 border-b border-slate-200 flex flex-col sm:flex-row sm:items-end justify-between gap-4">
        <div>
          <h1 className="text-xl font-bold tracking-tight text-slate-900">
            Conscience Journal &amp; Daemon Logcat
          </h1>
          <p className="text-sm text-slate-600 mt-1">
            Structured event audit from <span className="font-mono">/data/adb/coreflow/decision_trace.jsonl</span> and runtime log stream (<span className="font-mono">logs/coreflowd.log</span>).
          </p>
        </div>

        <div className="flex items-center gap-2 shrink-0">
          <button
            type="button"
            onClick={onRefreshLogs}
            className="inline-flex items-center gap-1.5 px-3 py-1.5 text-xs font-medium text-slate-700 bg-white border border-slate-200 rounded-lg hover:bg-slate-50 transition-colors"
          >
            <RefreshCw className="w-3.5 h-3.5" aria-hidden="true" />
            <span>Reload</span>
          </button>
          <button
            type="button"
            onClick={handleExport}
            className="inline-flex items-center gap-1.5 px-3 py-1.5 text-xs font-medium text-slate-700 bg-white border border-slate-200 rounded-lg hover:bg-slate-50 transition-colors"
          >
            <Download className="w-3.5 h-3.5" aria-hidden="true" />
            <span>Export Logs</span>
          </button>
          {history && history.length > 0 && (
            <button
              type="button"
              onClick={handleExportCsv}
              title="Export historical telemetry samples into CSV format for diagnostics"
              className="inline-flex items-center gap-1.5 px-3 py-1.5 text-xs font-medium text-emerald-800 bg-emerald-50 border border-emerald-200 rounded-lg hover:bg-emerald-100 transition-colors"
            >
              <FileSpreadsheet className="w-3.5 h-3.5 text-emerald-600" aria-hidden="true" />
              <span>Download History CSV ({history.length})</span>
            </button>
          )}
          {!isNativeBridge && (
            <button
              type="button"
              onClick={onClearLogs}
              className="inline-flex items-center gap-1.5 px-3 py-1.5 text-xs font-medium text-slate-600 bg-white border border-slate-200 rounded-lg hover:bg-slate-50 transition-colors"
            >
              <Trash2 className="w-3.5 h-3.5" aria-hidden="true" />
              <span>Clear</span>
            </button>
          )}
        </div>
      </div>

      {/* Mode Switch + Filters */}
      <div className="flex flex-col md:flex-row md:items-center justify-between gap-3">
        <div className="flex flex-wrap items-center gap-2">
          <div className="inline-flex p-0.5 bg-slate-100 rounded-lg border border-slate-200">
            <button
              type="button"
              onClick={() => setStreamTab('trace')}
              className={`px-3 py-1.5 text-xs font-semibold rounded-md transition-colors ${
                streamTab === 'trace'
                  ? 'bg-white text-slate-900 shadow-xs'
                  : 'text-slate-600 hover:text-slate-900'
              }`}
            >
              Conscience Trace ({traceRows.length})
            </button>
            <button
              type="button"
              onClick={() => setStreamTab('logcat')}
              className={`px-3 py-1.5 text-xs font-semibold rounded-md transition-colors ${
                streamTab === 'logcat'
                  ? 'bg-white text-slate-900 shadow-xs'
                  : 'text-slate-600 hover:text-slate-900'
              }`}
            >
              Daemon Logcat (
              {isNativeBridge && nativeLogLines.length > 0
                ? nativeLogLines.length
                : entries.length}
              )
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
                  className={`px-2.5 py-1 text-xs font-medium rounded-md transition-colors ${
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
        <div className="bg-white border border-slate-200 rounded-xl overflow-hidden">
          {filteredTrace.length === 0 ? (
            <div className="p-10 text-center space-y-2">
              <div className="text-sm font-semibold text-slate-900">
                No matching Conscience trace events
              </div>
              <p className="text-xs text-slate-500 max-w-md mx-auto">
                Events from <span className="font-mono">decision_trace.jsonl</span> appear here as <span className="font-mono">coreflowd</span> evaluates scope gates, verifies sysfs readbacks, or enforces safety holds.
              </p>
              {traceFilter !== 'all' && (
                <button
                  type="button"
                  onClick={() => setTraceFilter('all')}
                  className="mt-2 inline-flex items-center px-3 py-1.5 text-xs font-medium text-blue-700 bg-blue-50 rounded-md hover:bg-blue-100"
                >
                  Show all trace events
                </button>
              )}
            </div>
          ) : (
            <div className="overflow-x-auto">
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
                          <div className="font-mono text-[11px] text-slate-400">
                            {row.event}
                          </div>
                        </td>
                        <td className="py-2.5 px-4 font-mono text-slate-600 whitespace-nowrap">
                          {row.scope || 'runtime'}
                        </td>
                        <td className={`py-2.5 px-4 font-mono whitespace-nowrap ${toneColor}`}>
                          {row.result}
                        </td>
                        <td className="py-2.5 px-4 text-slate-700 space-y-0.5">
                          <div>{row.reason || '—'}</div>
                          {row.target && (
                            <div className="font-mono text-[11px] text-slate-500">
                              {row.target}
                              {row.requested ? ` -> ${row.requested}` : ''}
                              {row.baselineValue
                                ? ` (baseline=${row.baselineValue})`
                                : ''}
                            </div>
                          )}
                        </td>
                      </tr>
                    );
                  })}
                </tbody>
              </table>
            </div>
          )}
        </div>
      ) : (
        <div className="bg-slate-950 text-slate-100 border border-slate-800 rounded-xl overflow-hidden">
          <div className="px-4 py-2.5 border-b border-slate-800 flex items-center justify-between text-xs font-mono text-slate-400">
            <span>
              {isNativeBridge
                ? '/data/adb/coreflow/logs/coreflowd.log'
                : 'android/log.h · CoreFlowAutonomous / CoreFlowConscience'}
            </span>
            <span className="tabular-nums">
              {Array.isArray(filteredLogcat) ? filteredLogcat.length : 0} lines
            </span>
          </div>
          <div className="p-4 max-h-[540px] overflow-y-auto font-mono text-xs space-y-1.5">
            {isNativeBridge && nativeLogLines.length > 0 ? (
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
                  <div
                    key={entry.id}
                    className="flex items-start gap-2.5 leading-relaxed"
                  >
                    <span className="text-slate-500 shrink-0 tabular-nums">
                      {entry.timeFormatted}
                    </span>
                    <span className="text-slate-600 shrink-0 tabular-nums">
                      [#{entry.sampleIndex}]
                    </span>
                    <span className={`font-bold shrink-0 ${levelColor}`}>
                      {entry.level}/{entry.tag}:
                    </span>
                    <span className="text-slate-200 break-words">
                      {entry.message}
                    </span>
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
