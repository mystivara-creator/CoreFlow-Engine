import React, { useEffect, useMemo, useRef, useState } from 'react';
import { Check, Copy, Search, Terminal } from 'lucide-react';
import { ReactionLogEvent } from '../engine/autonomousEngine';

interface LiveReactionStreamProps {
  logs: ReactionLogEvent[];
  nativeMode?: boolean;
}

const FILTER_CHIPS: { id: string; label: string }[] = [
  { id: 'ALL', label: 'All Events' },
  { id: 'BOOST', label: 'Sustained Load' },
  { id: 'THERMAL', label: 'Thermal Gate' },
  { id: 'BASELINE', label: 'Journal & Recovery' },
  { id: 'INFO', label: 'Admission & Info' },
];

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
      // clipboard unavailable in restricted WebView context
    }
  };

  const badgeClassForType = (type: ReactionLogEvent['type']): string => {
    switch (type) {
      case 'THERMAL':
        return 'md3-badge md3-badge-warning';
      case 'BASELINE':
        return 'md3-badge md3-badge-success';
      case 'BOOST':
        return 'md3-badge md3-badge-primary';
      default:
        return 'md3-badge md3-badge-neutral';
    }
  };

  return (
    <section className="md3-card-elevated p-6 sm:p-7 space-y-5" aria-label="Observability and audit trail">
      {/* Header Row */}
      <div
        className="flex flex-col sm:flex-row sm:items-center justify-between gap-4 pb-5"
        style={{ borderBottom: '1px solid var(--md-sys-color-outline-variant)' }}
      >
        <div className="flex items-start gap-3.5">
          <div
            className="w-11 h-11 rounded-2xl flex items-center justify-center shrink-0"
            style={{
              backgroundColor: 'var(--md-sys-color-primary-container)',
              color: 'var(--md-sys-color-on-primary-container)',
            }}
          >
            <Terminal className="w-5 h-5" />
          </div>
          <div className="space-y-1">
            <div className="flex flex-wrap items-center gap-2">
              <h3 className="font-bold text-base sm:text-lg">
                Observability &amp; Audit Trail
              </h3>
              <span className="md3-badge md3-badge-neutral font-mono">
                {filteredLogs.length} events
              </span>
            </div>
            <p className="text-xs sm:text-sm" style={{ color: 'var(--md-sys-color-on-surface-variant)' }}>
              {nativeMode
                ? 'Live daemon transaction audit stream from runtime log ring.'
                : 'Preview transaction trace showing admission, readback verification, and outcome learning.'}
            </p>
          </div>
        </div>

        <div className="flex items-center gap-2 shrink-0">
          <button
            type="button"
            onClick={() => void handleCopyLogs()}
            className="md3-btn-outlined"
          >
            {copied ? (
              <Check className="w-3.5 h-3.5" style={{ color: 'var(--md-sys-color-primary)' }} />
            ) : (
              <Copy className="w-3.5 h-3.5" />
            )}
            <span>{copied ? 'Copied' : 'Copy Audit Log'}</span>
          </button>
        </div>
      </div>

      {/* Material 3 Filter Chips & Search Input */}
      <div className="flex flex-col sm:flex-row items-stretch sm:items-center justify-between gap-3">
        <div className="flex flex-wrap items-center gap-1.5">
          {FILTER_CHIPS.map((chip) => {
            const active = filterType === chip.id;
            return (
              <button
                key={chip.id}
                type="button"
                aria-pressed={active}
                onClick={() => setFilterType(chip.id)}
                className={'md3-chip ' + (active ? 'is-selected' : '')}
              >
                {chip.label}
              </button>
            );
          })}
        </div>

        <div className="relative">
          <Search
            className="w-3.5 h-3.5 absolute left-3.5 top-1/2 -translate-y-1/2 pointer-events-none"
            style={{ color: 'var(--md-sys-color-on-surface-variant)' }}
          />
          <input
            type="text"
            aria-label="Filter audit trail"
            placeholder="Filter audit trail..."
            value={searchQuery}
            onChange={(e) => setSearchQuery(e.target.value)}
            className="w-full sm:w-60 pl-9 pr-4 py-2 text-xs rounded-full focus:outline-none"
            style={{
              backgroundColor: 'var(--md-sys-color-surface-container)',
              border: '1px solid var(--md-sys-color-outline-variant)',
              color: 'var(--md-sys-color-on-surface)',
            }}
          />
        </div>
      </div>

      {/* Block-Aligned Log Stream */}
      <div
        role="log"
        aria-label="Audit trail"
        className="md3-card-filled p-3.5 max-h-80 overflow-y-auto font-mono text-xs divide-y"
        style={{ borderColor: 'var(--md-sys-color-outline-variant)' }}
      >
        {filteredLogs.length === 0 ? (
          <div
            className="text-center py-8 font-sans text-xs"
            style={{ color: 'var(--md-sys-color-on-surface-variant)' }}
          >
            No audit events match the current filter.
          </div>
        ) : (
          filteredLogs.map((log) => (
            <div
              key={log.id}
              className="grid grid-cols-1 sm:grid-cols-[78px_98px_124px_1fr] items-baseline gap-2 py-2.5 px-2.5 rounded-xl transition-colors"
              style={{ borderColor: 'var(--md-sys-color-outline-variant)' }}
            >
              <span
                className="tabular-nums text-[11px]"
                style={{ color: 'var(--md-sys-color-on-surface-variant)' }}
              >
                {log.timestamp}
              </span>
              <span>
                <span className={badgeClassForType(log.type)}>{log.type}</span>
              </span>
              <span className="font-semibold truncate">{log.tag}</span>
              <span
                className="break-words leading-relaxed"
                style={{ color: 'var(--md-sys-color-on-surface-variant)' }}
              >
                {log.message}
              </span>
            </div>
          ))
        )}
      </div>
    </section>
  );
};
