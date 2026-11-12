import React, { useEffect, useMemo, useRef, useState } from 'react';
import { Terminal, Copy, Check, Search } from 'lucide-react';
import { ReactionLogEvent } from '../engine/autonomousEngine';

interface LiveReactionStreamProps {
  logs: ReactionLogEvent[];
  nativeMode?: boolean;
}

export const LiveReactionStream: React.FC<LiveReactionStreamProps> = ({
  logs,
  nativeMode = false,
}) => {
  const [filterType, setFilterType] = useState<string>('ALL');
  const [searchQuery, setSearchQuery] = useState<string>('');
  const [copied, setCopied] = useState<boolean>(false);
  const copyTimerRef = useRef<number | null>(null);

  useEffect(() => {
    return () => {
      if (copyTimerRef.current !== null) {
        window.clearTimeout(copyTimerRef.current);
      }
    };
  }, []);

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
      if (copyTimerRef.current !== null) {
        window.clearTimeout(copyTimerRef.current);
      }
      copyTimerRef.current = window.setTimeout(() => {
        setCopied(false);
        copyTimerRef.current = null;
      }, 2000);
    } catch {
      // ignore
    }
  };

  return (
    <div className="m3-card p-6 sm:p-8">
      {/* Header and Controls */}
      <div className="flex flex-col sm:flex-row sm:items-center justify-between gap-4 pb-5 mb-5 border-b border-slate-900/[0.06] dark:border-white/[0.06]">
        <div className="flex items-start gap-3.5">
          <div className="w-10 h-10 rounded-2xl bg-[#d3e3fd] dark:bg-[#0842a0] text-[#041e49] dark:text-[#d3e3fd] flex items-center justify-center shrink-0">
            <Terminal className="w-4 h-4" />
          </div>
          <div className="space-y-1">
            <h3 className="font-semibold text-base sm:text-lg text-slate-900 dark:text-slate-100">
              Observability & Audit Trail
            </h3>
            <p className="text-xs sm:text-sm text-slate-600 dark:text-slate-400">
              {nativeMode
                ? 'Live daemon transaction audit stream from runtime stdout/stderr.'
                : 'Preview transaction trace showing admission, readback verification, and outcome learning.'}
            </p>
          </div>
        </div>

        {/* Action Buttons */}
        <div className="flex items-center gap-2 shrink-0">
          <button
            onClick={handleCopyLogs}
            className="inline-flex items-center gap-2 px-4 py-2 rounded-full bg-slate-900/[0.05] hover:bg-slate-900/[0.09] dark:bg-white/[0.06] dark:hover:bg-white/[0.11] text-xs font-medium text-slate-700 dark:text-slate-200"
          >
            {copied ? (
              <Check className="w-3.5 h-3.5 text-[#0b57d0] dark:text-[#a8c7fa]" />
            ) : (
              <Copy className="w-3.5 h-3.5" />
            )}
            <span>{copied ? 'Copied' : 'Copy Audit Log'}</span>
          </button>
        </div>
      </div>

      {/* Filter and Search Bar */}
      <div className="flex flex-col sm:flex-row items-stretch sm:items-center justify-between gap-3 mb-4">
        <div className="flex flex-wrap items-center gap-1.5 text-xs">
          {[
            { id: 'ALL', label: 'All Events' },
            { id: 'BOOST', label: 'Sustained Load' },
            { id: 'THERMAL', label: 'Thermal Gate' },
            { id: 'BASELINE', label: 'Journal & Recovery' },
          ].map((chip) => {
            const active = filterType === chip.id;
            return (
              <button
                key={chip.id}
                onClick={() => setFilterType(chip.id)}
                className={
                  'px-3.5 py-1.5 rounded-full font-medium transition-all ' +
                  (active
                    ? 'bg-[#d3e3fd] text-[#041e49] dark:bg-[#0842a0] dark:text-[#d3e3fd] font-semibold'
                    : 'bg-slate-900/[0.04] hover:bg-slate-900/[0.08] dark:bg-white/[0.05] dark:hover:bg-white/[0.09] text-slate-600 dark:text-slate-400')
                }
              >
                {chip.label}
              </button>
            );
          })}
        </div>

        <div className="relative">
          <Search className="w-3.5 h-3.5 text-slate-400 absolute left-3.5 top-1/2 -translate-y-1/2 pointer-events-none" />
          <input
            type="text"
            placeholder="Filter audit trail..."
            value={searchQuery}
            onChange={(e) => setSearchQuery(e.target.value)}
            className="w-full sm:w-56 pl-9 pr-4 py-1.5 text-xs rounded-full bg-slate-900/[0.04] dark:bg-white/[0.05] border border-slate-900/[0.06] dark:border-white/[0.06] text-slate-800 dark:text-slate-200 focus:outline-none focus:border-[#0b57d0] dark:focus:border-[#a8c7fa]"
          />
        </div>
      </div>

      {/* Block-Aligned Log Stream */}
      <div className="m3-tonal-block p-4 max-h-72 overflow-y-auto font-mono text-xs divide-y divide-slate-900/[0.05] dark:divide-white/[0.05]">
        {filteredLogs.length === 0 ? (
          <div className="text-center py-8 text-slate-500 dark:text-slate-400 font-sans text-xs">
            No audit events match the current filter.
          </div>
        ) : (
          filteredLogs.map((log) => (
            <div
              key={log.id}
              className="grid grid-cols-1 sm:grid-cols-[76px_88px_110px_1fr] items-baseline gap-2 py-2 px-2 hover:bg-slate-900/[0.03] dark:hover:bg-white/[0.03] rounded-xl transition-colors"
            >
              <span className="text-slate-400 dark:text-slate-500 tabular-nums">
                {log.timestamp}
              </span>
              <span className="text-[11px] font-semibold text-[#0b57d0] dark:text-[#a8c7fa]">
                {log.type}
              </span>
              <span className="text-slate-700 dark:text-slate-300 font-medium truncate">
                {log.tag}
              </span>
              <span className="text-slate-600 dark:text-slate-300 break-words leading-relaxed">
                {log.message}
              </span>
            </div>
          ))
        )}
      </div>
    </div>
  );
};
