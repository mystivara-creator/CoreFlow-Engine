import React, { useState } from 'react';
import { Check, Search, ShieldAlert, Slash, Sliders } from 'lucide-react';
import { EXPANDED_DOMAIN_NAMES, ParsedEngineStatus } from '../bridge/daemonBridge';
import { SysfsNodeEntry, SysfsNodeStatus, SysfsSubsystem } from '../engine/types';

interface SysfsInspectorViewProps {
  nodes: SysfsNodeEntry[];
  parsedStatus: ParsedEngineStatus | null;
}

const SUBSYSTEMS: ('All' | SysfsSubsystem)[] = [
  'All',
  'CPU & WALT',
  'Block I/O',
  'VM / Sysctl',
  'GPU & DSP',
  'Charging',
];

function renderStatusCell(status: SysfsNodeStatus) {
  switch (status) {
    case 'MUTATED':
      return (
        <span className="inline-flex items-center gap-1.5 font-mono font-semibold text-blue-700">
          <Sliders className="w-3.5 h-3.5 shrink-0" aria-hidden="true" />
          MUTATED
        </span>
      );
    case 'CLAMPED':
      return (
        <span className="inline-flex items-center gap-1.5 font-mono font-semibold text-red-700">
          <ShieldAlert className="w-3.5 h-3.5 shrink-0" aria-hidden="true" />
          CLAMPED
        </span>
      );
    case 'SKIPPED':
      return (
        <span className="inline-flex items-center gap-1.5 font-mono text-slate-500">
          <Slash className="w-3.5 h-3.5 shrink-0" aria-hidden="true" />
          SKIPPED
        </span>
      );
    case 'BASELINE':
    default:
      return (
        <span className="inline-flex items-center gap-1.5 font-mono text-emerald-700">
          <Check className="w-3.5 h-3.5 shrink-0" aria-hidden="true" />
          BASELINE
        </span>
      );
  }
}

export const SysfsInspectorView: React.FC<SysfsInspectorViewProps> = ({
  nodes,
  parsedStatus,
}) => {
  const [subsystem, setSubsystem] = useState<'All' | SysfsSubsystem>('All');
  const [searchQuery, setSearchQuery] = useState('');
  const [onlyModified, setOnlyModified] = useState(false);

  const d = parsedStatus?.data;
  const liveApplied = [
    ...(d?.hands.cpuApplied.map((x) => ({ path: x.key, value: x.value, kind: 'CpuFreqActuator' })) ?? []),
    ...(d?.hands.resourceApplied.map((x) => ({ path: x.key, value: x.value, kind: 'ResourceActuator' })) ?? []),
  ];

  const filtered = nodes.filter((node) => {
    if (subsystem !== 'All' && node.subsystem !== subsystem) return false;
    if (onlyModified && node.status !== 'MUTATED' && node.status !== 'CLAMPED') {
      return false;
    }
    if (searchQuery.trim() !== '') {
      const q = searchQuery.toLowerCase();
      return (
        node.path.toLowerCase().includes(q) ||
        node.rationale.toLowerCase().includes(q) ||
        node.currentValue.toLowerCase().includes(q)
      );
    }
    return true;
  });

  return (
    <div className="space-y-6">
      <div className="pb-4 border-b border-slate-200 flex flex-col sm:flex-row sm:items-end justify-between gap-3">
        <div>
          <div className="text-xs text-slate-500">
            TunableDiscovery &rarr; MutationPermit &rarr; Pre-Write Journal &rarr; Read-Back Verify
          </div>
          <h1 className="mt-1 text-2xl font-bold tracking-tight text-slate-900">
            Kernel Resources &amp; Sysfs Inspector
          </h1>
        </div>
        <div className="text-xs font-mono text-slate-600 tabular-nums">
          {d
            ? `${d.device.discoveredResources} discovered · ${d.device.mutationReadyResources} policy-ready`
            : `Showing ${filtered.length} of ${nodes.length} nodes`}
        </div>
      </div>

      {/* Live C++ Hands Applied Mutations (if any held by coreflowd) */}
      {liveApplied.length > 0 && (
        <section className="bg-white border border-slate-200 rounded-xl overflow-hidden">
          <div className="px-5 py-3.5 border-b border-slate-200 flex items-center justify-between">
            <h2 className="text-sm font-semibold text-slate-900">
              Active Mutations Held by C++ ActuatorManager
            </h2>
            <span className="text-xs font-mono text-slate-500 tabular-nums">
              {liveApplied.length} verified writes
            </span>
          </div>
          <div className="divide-y divide-slate-200 text-xs font-mono">
            {liveApplied.map((item, idx) => (
              <div key={`${item.path}-${idx}`} className="px-5 py-2.5 flex items-center justify-between gap-4">
                <span className="text-slate-900 truncate">{item.path}</span>
                <span className="text-slate-500 shrink-0">
                  {item.kind} &rarr; <strong className="text-blue-700">{item.value}</strong>
                </span>
              </div>
            ))}
          </div>
        </section>
      )}

      {/* Interactive Filter Toolbar */}
      <div className="flex flex-col md:flex-row md:items-center justify-between gap-3">
        <div className="flex items-center gap-1 p-1 bg-slate-100 rounded-lg overflow-x-auto">
          {SUBSYSTEMS.map((tab) => (
            <button
              key={tab}
              type="button"
              onClick={() => setSubsystem(tab)}
              className={`px-3 py-1.5 text-xs font-medium rounded-md transition-colors whitespace-nowrap shrink-0 ${
                subsystem === tab
                  ? 'bg-white text-slate-900 shadow-xs'
                  : 'text-slate-600 hover:text-slate-900'
              }`}
            >
              {tab}
            </button>
          ))}
        </div>

        <div className="flex items-center gap-3">
          <label className="inline-flex items-center gap-2 text-xs font-medium text-slate-700 cursor-pointer whitespace-nowrap">
            <input
              type="checkbox"
              checked={onlyModified}
              onChange={(e) => setOnlyModified(e.target.checked)}
              className="w-4 h-4 rounded border-slate-300 text-blue-600 focus:ring-blue-600"
            />
            <span>Active Mutations Only</span>
          </label>

          <div className="relative w-full md:w-64">
            <Search
              className="w-3.5 h-3.5 text-slate-400 absolute left-3 top-1/2 -translate-y-1/2 pointer-events-none"
              aria-hidden="true"
            />
            <input
              type="search"
              value={searchQuery}
              onChange={(e) => setSearchQuery(e.target.value)}
              placeholder="Filter path or rationale..."
              aria-label="Filter kernel sysfs nodes"
              className="w-full pl-8 pr-3 py-1.5 text-xs bg-white border border-slate-200 rounded-lg text-slate-900 placeholder:text-slate-400 focus:outline-none focus:ring-2 focus:ring-blue-600"
            />
          </div>
        </div>
      </div>

      {/* Sysfs Data Grid */}
      <section className="bg-white border border-slate-200 rounded-xl overflow-hidden">
        {filtered.length === 0 ? (
          <div className="p-12 text-center space-y-3">
            <div className="text-sm font-semibold text-slate-900">
              No kernel nodes match the current filter
            </div>
            <p className="text-xs text-slate-500 max-w-md mx-auto">
              Clear your search query or disable the Active Mutations filter to inspect preserved OEM factory baseline parameters.
            </p>
            <button
              type="button"
              onClick={() => {
                setSubsystem('All');
                setSearchQuery('');
                setOnlyModified(false);
              }}
              className="px-4 py-2 text-xs font-semibold text-white bg-blue-600 rounded-lg hover:bg-blue-700 transition-colors"
            >
              Reset Node Filters
            </button>
          </div>
        ) : (
          <div className="overflow-x-auto">
            <table className="w-full text-left border-collapse">
              <thead>
                <tr className="border-b border-slate-200 bg-slate-50 text-xs font-medium text-slate-600">
                  <th className="py-2.5 px-5">Kernel Path / Tunable</th>
                  <th className="py-2.5 px-4">Subsystem</th>
                  <th className="py-2.5 px-4 text-right">OEM Baseline</th>
                  <th className="py-2.5 px-4 text-right">Active Value</th>
                  <th className="py-2.5 px-4">State</th>
                  <th className="py-2.5 px-5">Engine Rationale</th>
                </tr>
              </thead>
              <tbody className="divide-y divide-slate-200 text-xs">
                {filtered.map((node) => (
                  <tr key={node.path} className="hover:bg-slate-50/80 transition-colors">
                    <td className="py-2.5 px-5 font-mono text-slate-900 whitespace-nowrap">
                      {node.path}
                    </td>
                    <td className="py-2.5 px-4 text-slate-600 whitespace-nowrap">
                      {node.subsystem}
                    </td>
                    <td className="py-2.5 px-4 text-right font-mono text-slate-500 tabular-nums whitespace-nowrap">
                      {node.baselineValue}
                    </td>
                    <td className="py-2.5 px-4 text-right font-mono font-semibold text-slate-900 tabular-nums whitespace-nowrap">
                      {node.currentValue}
                    </td>
                    <td className="py-2.5 px-4 whitespace-nowrap">
                      {renderStatusCell(node.status)}
                    </td>
                    <td className="py-2.5 px-5 text-slate-600 min-w-[240px]">
                      {node.rationale}
                    </td>
                  </tr>
                ))}
              </tbody>
            </table>
          </div>
        )}
      </section>

      {/* C++ Expanded Kernel Discovery Domains (ADR-0008) */}
      {d && d.device.expandedDomains.length > 0 && (
        <section className="bg-white border border-slate-200 rounded-xl overflow-hidden">
          <div className="px-5 py-4 border-b border-slate-200">
            <h2 className="text-base font-semibold text-slate-900">
              C++ Dynamic Kernel Resource Discovery Domains (ADR-0008)
            </h2>
          </div>
          <div className="overflow-x-auto">
            <table className="w-full text-left border-collapse">
              <thead>
                <tr className="border-b border-slate-200 bg-slate-50 text-xs font-medium text-slate-600">
                  <th className="py-2.5 px-5">Domain</th>
                  <th className="py-2.5 px-4">Evidence State</th>
                  <th className="py-2.5 px-4 text-right">Matched Nodes</th>
                  <th className="py-2.5 px-4 text-right">Readable</th>
                  <th className="py-2.5 px-5">Mutation Permit</th>
                </tr>
              </thead>
              <tbody className="divide-y divide-slate-200 text-xs">
                {d.device.expandedDomains.map((dom) => (
                  <tr key={dom.id} className="hover:bg-slate-50/80">
                    <td className="py-2.5 px-5 font-medium text-slate-900">
                      {EXPANDED_DOMAIN_NAMES[dom.id] || `Domain #${dom.id}`}
                    </td>
                    <td className="py-2.5 px-4 font-mono text-slate-700">{dom.evidence}</td>
                    <td className="py-2.5 px-4 text-right font-mono tabular-nums text-slate-700">
                      {dom.matched}
                    </td>
                    <td className="py-2.5 px-4 text-right font-mono tabular-nums text-slate-700">
                      {dom.readable}
                    </td>
                    <td className="py-2.5 px-5 font-mono">
                      {dom.mutationAuthorized ? (
                        <span className="text-emerald-700 font-semibold">AUTHORIZED</span>
                      ) : (
                        <span className="text-slate-500">OBSERVE_ONLY</span>
                      )}
                    </td>
                  </tr>
                ))}
              </tbody>
            </table>
          </div>
        </section>
      )}
    </div>
  );
};
