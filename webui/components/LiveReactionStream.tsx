import React, { useState, useMemo } from 'react';
import { Terminal, Copy, Check, Trash2, Search, Filter } from 'lucide-react';
import { ReactionLogEvent } from '../engine/autonomousEngine';

interface LiveReactionStreamProps {
  logs: ReactionLogEvent[];
  onClearLogs: () => void;
  engineActive: boolean;
  nativeMode?: boolean;
}

export const LiveReactionStream: React.FC<LiveReactionStreamProps> = ({
  logs,
  onClearLogs,
  engineActive,
  nativeMode = false,
}) => {
  const [filterType, setFilterType] = useState<string>('ALL');
  const [searchQuery, setSearchQuery] = useState<string>('');
  const [copied, setCopied] = useState<boolean>(false);

  const filteredLogs = useMemo(() => {
    return logs.filter((log) => {
      const matchesType = filterType === 'ALL' || log.type === filterType;
      const matchesSearch =
        !searchQuery ||
        log.message.toLowerCase().includes(searchQuery.toLowerCase()) ||
        log.tag.toLowerCase().includes(searchQuery.toLowerCase());
      return matchesType && matchesSearch;
    });
  }, [logs, filterType, searchQuery]);

  const handleCopyLogs = async () => {
    const text = filteredLogs
      .map((l) => `[${l.timestamp}] [${l.type}] ${l.tag}: ${l.message}`)
      .join('\n');
    try {
      await navigator.clipboard.writeText(text);
      setCopied(true);
      setTimeout(() => setCopied(false), 2000);
    } catch {
      // ignore
    }
  };

  const getTagBadge = (type: ReactionLogEvent['type']) => {
    switch (type) {
      case 'BOOST':
        return 'text-sky-500 bg-sky-500/10 border-sky-500/20';
      case 'THERMAL':
        return 'text-rose-500 bg-rose-500/10 border-rose-500/20';
      case 'BASELINE':
        return 'text-teal-400 bg-teal-500/10 border-teal-500/20';
      case 'INFO':
      default:
        return 'text-slate-400 bg-slate-500/10 border-slate-500/20';
    }
  };

  return (
    <div className="rounded-3xl bg-slate-100/80 dark:bg-[#111827]/90 border border-slate-200/90 dark:border-slate-800 p-5 sm:p-7 shadow-lg shadow-slate-900/5 transition-all">
      {/* Header and Controls */}
      <div className="flex flex-col sm:flex-row sm:items-center justify-between gap-3 mb-4 pb-4 border-b border-slate-200/60 dark:border-slate-800/60">
        <div className="flex items-center gap-2.5">
          <div className="w-8 h-8 rounded-xl bg-slate-200 dark:bg-slate-800 flex items-center justify-center text-slate-700 dark:text-slate-300">
            <Terminal className="w-4 h-4" />
          </div>
          <div>
            <div className="flex items-center gap-2">
              <h3 className="font-bold text-base text-slate-900 dark:text-white">
                Live Engine Activity Stream
              </h3>
              {engineActive && (
                <span className="flex h-2 w-2 relative">
                  <span className="animate-ping absolute inline-flex h-full w-full rounded-full bg-emerald-400 opacity-75" />
                  <span className="relative inline-flex rounded-full h-2 w-2 bg-emerald-500" />
                </span>
              )}
            </div>
            <p className="text-xs text-slate-500 dark:text-slate-400">
              {nativeMode ? 'Captured native daemon stdout/stderr; simulation is disabled' : 'Preview-only simulated engine events'}
            </p>
          </div>
        </div>

        {/* Action Buttons: Copy and Clear */}
        <div className="flex items-center gap-2">
          <button
            onClick={handleCopyLogs}
            className="flex items-center gap-1.5 px-3 py-1.5 rounded-full bg-white dark:bg-slate-800 border border-slate-200 dark:border-slate-700 text-xs font-semibold text-slate-700 dark:text-slate-200 hover:bg-slate-100 dark:hover:bg-slate-700 transition-colors"
          >
            {copied ? <Check className="w-3.5 h-3.5 text-emerald-500" /> : <Copy className="w-3.5 h-3.5" />}
            <span>{copied ? 'Copied' : 'Copy'}</span>
          </button>
          <button
            onClick={onClearLogs}
            className="p-1.5 rounded-full bg-white dark:bg-slate-800 border border-slate-200 dark:border-slate-700 text-slate-400 hover:text-rose-500 hover:bg-slate-100 dark:hover:bg-slate-700 transition-colors"
            title="Clear log buffer"
          >
            <Trash2 className="w-3.5 h-3.5" />
          </button>
        </div>
      </div>

      {/* Filter and Search Bar */}
      <div className="flex flex-col sm:flex-row items-stretch sm:items-center justify-between gap-3 mb-3">
        {/* Category Filter Chips */}
        <div className="flex flex-wrap items-center gap-1.5 text-xs">
          {[
            { id: 'ALL', label: 'All' },
            { id: 'BOOST', label: 'Sustained Load' },
            { id: 'THERMAL', label: 'Thermal' },
            { id: 'BASELINE', label: 'Recovery' },
          ].map((chip) => (
            <button
              key={chip.id}
              onClick={() => setFilterType(chip.id)}
              className={`px-3 py-1 rounded-full font-medium transition-all ${
                filterType === chip.id
                  ? 'bg-sky-500 text-slate-950 font-semibold shadow-sm'
                  : 'bg-white/80 dark:bg-slate-900/80 text-slate-600 dark:text-slate-400 hover:text-slate-900 dark:hover:text-white border border-slate-200 dark:border-slate-800'
              }`}
            >
              {chip.label}
            </button>
          ))}
        </div>

        {/* Live Search input */}
        <div className="relative">
          <Search className="w-3.5 h-3.5 text-slate-400 absolute left-3 top-1/2 -translate-y-1/2 pointer-events-none" />
          <input
            type="text"
            placeholder="Search events..."
            value={searchQuery}
            onChange={(e) => setSearchQuery(e.target.value)}
            className="w-full sm:w-48 pl-8 pr-3 py-1 text-xs rounded-full bg-white dark:bg-slate-900 border border-slate-200 dark:border-slate-800 text-slate-800 dark:text-slate-200 focus:outline-none focus:border-sky-500"
          />
        </div>
      </div>

      {/* Log Feed Display */}
      <div className="rounded-2xl bg-[#090d16] border border-slate-800 p-3 sm:p-4 max-h-72 overflow-y-auto font-mono text-xs text-slate-300 space-y-1.5">
        {filteredLogs.length === 0 ? (
          <div className="text-center py-6 text-slate-500 font-sans text-xs">
            No events match current filter.
          </div>
        ) : (
          filteredLogs.map((log) => (
            <div
              key={log.id}
              className="flex items-start gap-2 py-0.5 hover:bg-slate-800/40 px-1 rounded transition-colors"
            >
              <span className="text-slate-500 shrink-0 tabular-nums">
                {log.timestamp}
              </span>
              <span
                className={`text-[10px] font-semibold px-1.5 py-0.2 rounded border uppercase tracking-wider shrink-0 ${getTagBadge(
                  log.type
                )}`}
              >
                {log.type}
              </span>
              <span className="text-sky-400 font-semibold shrink-0">
                {log.tag}:
              </span>
              <span className="text-slate-300 break-all">{log.message}</span>
            </div>
          ))
        )}
      </div>
    </div>
  );
};
