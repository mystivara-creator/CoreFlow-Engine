import React from 'react';
import { Pause, Play } from 'lucide-react';

export type ActiveTab = 'console' | 'sysfs' | 'baseline' | 'logcat';

interface TopNavProps {
  activeTab: ActiveTab;
  onSelectTab: (tab: ActiveTab) => void;
  isRunning: boolean;
  onToggleRunning: () => void;
}

const NAV_ITEMS: { id: ActiveTab; label: string }[] = [
  { id: 'console', label: 'Runtime Console' },
  { id: 'sysfs', label: 'Sysfs Inspector' },
  { id: 'baseline', label: 'Baseline & Thermal' },
  { id: 'logcat', label: 'Logcat Stream' },
];

export const TopNav: React.FC<TopNavProps> = ({
  activeTab,
  onSelectTab,
  isRunning,
  onToggleRunning,
}) => {
  return (
    <header className="sticky top-0 z-20 flex items-center justify-between gap-8 px-6 py-3.5 bg-white border-b border-slate-200">
      <a
        href="#top"
        onClick={(e) => {
          e.preventDefault();
          onSelectTab('console');
        }}
        className="text-lg font-bold tracking-tight text-slate-900 whitespace-nowrap shrink-0 focus-visible:outline-2 focus-visible:outline-offset-2 focus-visible:outline-blue-600"
      >
        CoreFlow Engine
      </a>

      <nav aria-label="Primary sections" className="flex items-center gap-6 overflow-x-auto">
        {NAV_ITEMS.map((item) => {
          const isActive = activeTab === item.id;
          return (
            <button
              key={item.id}
              type="button"
              onClick={() => onSelectTab(item.id)}
              className={`py-1 text-sm font-medium transition-colors whitespace-nowrap shrink-0 border-b-2 focus-visible:outline-2 focus-visible:outline-offset-2 focus-visible:outline-blue-600 ${
                isActive
                  ? 'border-blue-600 text-slate-900'
                  : 'border-transparent text-slate-600 hover:text-slate-900 hover:border-slate-300'
              }`}
            >
              {item.label}
            </button>
          );
        })}
      </nav>

      <div className="flex items-center shrink-0">
        <button
          type="button"
          onClick={onToggleRunning}
          className={`inline-flex items-center gap-2 px-4 py-2 text-xs font-semibold rounded-lg transition-colors whitespace-nowrap shrink-0 focus-visible:outline-2 focus-visible:outline-offset-2 focus-visible:outline-blue-600 ${
            isRunning
              ? 'bg-slate-900 text-white hover:bg-slate-800'
              : 'bg-blue-600 text-white hover:bg-blue-700'
          }`}
        >
          {isRunning ? (
            <>
              <Pause className="w-3.5 h-3.5" aria-hidden="true" />
              <span>Pause Loop</span>
            </>
          ) : (
            <>
              <Play className="w-3.5 h-3.5" aria-hidden="true" />
              <span>Resume Loop</span>
            </>
          )}
        </button>
      </div>
    </header>
  );
};
