import React, { useState } from 'react';
import { Download, Search, StepForward, Trash2 } from 'lucide-react';
import { LogcatEntry } from '../engine/types';

interface LogcatStreamViewProps {
  logs: LogcatEntry[];
  onClearLogs: () => void;
  onStepTick: () => void;
}

const TAG_FILTERS = ['ALL', 'CoreFlowAutonomous', 'CoreFlowAI', 'ThermalGuardian'] as const;

export const LogcatStreamView: React.FC<LogcatStreamViewProps> = ({
  logs,
  onClearLogs,
  onStepTick,
}) => {
  const [tagFilter, setTagFilter] = useState<(typeof TAG_FILTERS)[number]>('ALL');
  const [query, setQuery] = useState('');

  const filteredLogs = logs.filter((entry) => {
    if (tagFilter !== 'ALL' && entry.tag !== tagFilter) return false;
    if (query.trim() !== '') {
      const q = query.toLowerCase();
      return (
        entry.message.toLowerCase().includes(q) ||
        entry.tag.toLowerCase().includes(q)
      );
    }
    return true;
  });

  const handleExport = () => {
    const content = filteredLogs
      .map(
        (l) =>
          `${l.timeFormatted} ${l.level}/${l.tag}( 1402): ${l.message}`
      )
      .join('\n');
    const blob = new Blob([content], { type: 'text/plain;charset=utf-8' });
    const url = URL.createObjectURL(blob);
    const a = document.createElement('a');
    a.href = url;
    a.download = `coreflow_logcat_${Date.now()}.log`;
    a.click();
    URL.revokeObjectURL(url);
  };

  return (
    <div className="space-y-6">
      <div className="pb-4 border-b border-slate-200 flex flex-col sm:flex-row sm:items-end justify-between gap-3">
        <div>
          <div className="text-xs font-mono text-slate-500">
            su -c &apos;logcat -d -s CoreFlowAutonomous:I CoreFlowAI:I ThermalGuardian:I *:S&apos;
          </div>
          <h1 className="mt-1 text-2xl font-bold tracking-tight text-slate-900">
            Daemon Logcat Verification Stream
          </h1>
        </div>
        <div className="flex items-center gap-2">
          <button
            type="button"
            onClick={handleExport}
            disabled={filteredLogs.length === 0}
            className="inline-flex items-center gap-1.5 px-3 py-1.5 text-xs font-medium text-slate-700 bg-white border border-slate-200 rounded-lg hover:bg-slate-50 disabled:opacity-40 transition-colors whitespace-nowrap"
          >
            <Download className="w-3.5 h-3.5" aria-hidden="true" />
            <span>Export .log</span>
          </button>
          <button
            type="button"
            onClick={onClearLogs}
            disabled={logs.length === 0}
            className="inline-flex items-center gap-1.5 px-3 py-1.5 text-xs font-medium text-slate-700 bg-white border border-slate-200 rounded-lg hover:bg-slate-50 disabled:opacity-40 transition-colors whitespace-nowrap"
          >
            <Trash2 className="w-3.5 h-3.5" aria-hidden="true" />
            <span>Clear Buffer</span>
          </button>
        </div>
      </div>

      {/* Filter Bar */}
      <div className="flex flex-col md:flex-row md:items-center justify-between gap-3">
        <div className="flex items-center gap-1 p-1 bg-slate-100 rounded-lg overflow-x-auto">
          {TAG_FILTERS.map((tag) => (
            <button
              key={tag}
              type="button"
              onClick={() => setTagFilter(tag)}
              className={`px-3 py-1.5 text-xs font-medium rounded-md transition-colors whitespace-nowrap shrink-0 ${
                tagFilter === tag
                  ? 'bg-white text-slate-900 shadow-xs'
                  : 'text-slate-600 hover:text-slate-900'
              }`}
            >
              {tag}
            </button>
          ))}
        </div>

        <div className="relative w-full md:w-72">
          <Search
            className="w-3.5 h-3.5 text-slate-400 absolute left-3 top-1/2 -translate-y-1/2 pointer-events-none"
            aria-hidden="true"
          />
          <input
            type="search"
            value={query}
            onChange={(e) => setQuery(e.target.value)}
            placeholder="Search log events (e.g. THERMAL_GUARD)..."
            aria-label="Search logcat entries"
            className="w-full pl-8 pr-3 py-1.5 text-xs bg-white border border-slate-200 rounded-lg text-slate-900 placeholder:text-slate-400 focus:outline-none focus:ring-2 focus:ring-blue-600"
          />
        </div>
      </div>

      {/* Logcat Buffer Container */}
      <section className="bg-slate-950 text-slate-100 border border-slate-800 rounded-xl overflow-hidden">
        {filteredLogs.length === 0 ? (
          <div className="p-12 text-center space-y-3">
            <div className="text-sm font-semibold text-slate-200">
              Logcat ring buffer is empty
            </div>
            <p className="text-xs text-slate-400 max-w-md mx-auto">
              Execute a runtime observation tick or adjust workload parameters to emit native Android daemon log entries.
            </p>
            <button
              type="button"
              onClick={onStepTick}
              className="inline-flex items-center gap-1.5 px-4 py-2 text-xs font-semibold text-white bg-blue-600 rounded-lg hover:bg-blue-700 transition-colors"
            >
              <StepForward className="w-3.5 h-3.5" aria-hidden="true" />
              <span>Emit Next Observation Tick</span>
            </button>
          </div>
        ) : (
          <div className="max-h-[540px] overflow-y-auto divide-y divide-slate-900 font-mono text-xs leading-relaxed">
            {filteredLogs.map((entry) => (
              <div
                key={entry.id}
                className="px-4 py-2 hover:bg-slate-900/80 flex flex-col sm:flex-row sm:items-baseline gap-1 sm:gap-3"
              >
                <span className="text-slate-500 tabular-nums shrink-0">
                  {entry.timeFormatted} [#{entry.sampleIndex}]
                </span>
                <span
                  className={`font-semibold shrink-0 ${
                    entry.level === 'W'
                      ? 'text-amber-400'
                      : entry.level === 'E'
                        ? 'text-red-400'
                        : 'text-blue-400'
                  }`}
                >
                  {entry.level}/{entry.tag}:
                </span>
                <span className="text-slate-200 break-all">{entry.message}</span>
              </div>
            ))}
          </div>
        )}
      </section>
    </div>
  );
};
