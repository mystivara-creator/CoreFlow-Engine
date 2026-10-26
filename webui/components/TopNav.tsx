import React, { useEffect, useRef, useState } from 'react';
import { Check, Menu, Pause, Play, RefreshCw, X } from 'lucide-react';

export type ActiveTab = 'console' | 'sysfs' | 'policy' | 'baseline' | 'logcat';

interface TopNavProps {
  activeTab: ActiveTab;
  onSelectTab: (tab: ActiveTab) => void;
  isNativeBridge: boolean;
  isRunning: boolean;
  onToggleRunning: () => void;
  onRefreshNative: () => void;
}

const NAV_ITEMS: { id: ActiveTab; label: string; summary: string }[] = [
  { id: 'console', label: 'Runtime Console', summary: 'Live closed-loop telemetry & status' },
  { id: 'sysfs', label: 'Sysfs Inspector', summary: 'Discovered kernel & GPU capabilities' },
  { id: 'policy', label: 'Policy & Config', summary: 'Adaptive gate & frame pacing bounds' },
  { id: 'baseline', label: 'Baseline & Thermal', summary: 'ONNX 18f predictor & outcome scores' },
  { id: 'logcat', label: 'Journal & Logcat', summary: 'Mutation journal & conscience trace' },
];

export const TopNav: React.FC<TopNavProps> = ({
  activeTab,
  onSelectTab,
  isNativeBridge,
  isRunning,
  onToggleRunning,
  onRefreshNative,
}) => {
  const [menuOpen, setMenuOpen] = useState(false);
  const menuContainerRef = useRef<HTMLDivElement | null>(null);

  const activeItem = NAV_ITEMS.find((item) => item.id === activeTab) ?? NAV_ITEMS[0];

  useEffect(() => {
    if (!menuOpen) return;

    const handleKeyDown = (event: KeyboardEvent) => {
      if (event.key === 'Escape') {
        setMenuOpen(false);
      }
    };

    const handlePointerDown = (event: MouseEvent | TouchEvent) => {
      const target = event.target as Node | null;
      if (menuContainerRef.current && target && !menuContainerRef.current.contains(target)) {
        setMenuOpen(false);
      }
    };

    document.addEventListener('keydown', handleKeyDown);
    document.addEventListener('mousedown', handlePointerDown);
    document.addEventListener('touchstart', handlePointerDown, { passive: true });
    return () => {
      document.removeEventListener('keydown', handleKeyDown);
      document.removeEventListener('mousedown', handlePointerDown);
      document.removeEventListener('touchstart', handlePointerDown);
    };
  }, [menuOpen]);

  const handleSelectMobileTab = (tab: ActiveTab) => {
    onSelectTab(tab);
    setMenuOpen(false);
  };

  return (
    <header
      ref={menuContainerRef}
      className="sticky top-0 z-30 bg-white border-b border-slate-200"
    >
      <div className="flex items-center justify-between gap-3 px-4 sm:px-6 py-2.5 lg:py-3.5">
        <div className="flex items-center gap-2.5 min-w-0">
          {/* Portrait / compact hamburger menu button (< lg) */}
          <button
            type="button"
            aria-label={menuOpen ? 'Close navigation menu' : 'Open navigation menu'}
            aria-expanded={menuOpen}
            aria-controls="mobile-primary-nav"
            onClick={() => setMenuOpen((prev) => !prev)}
            className="lg:hidden inline-flex items-center justify-center min-h-[44px] min-w-[44px] -ml-1 rounded-lg text-slate-700 hover:text-slate-900 hover:bg-slate-100 transition-colors shrink-0 focus-visible:outline-2 focus-visible:outline-offset-2 focus-visible:outline-blue-600"
          >
            {menuOpen ? (
              <X className="w-5 h-5" aria-hidden="true" />
            ) : (
              <Menu className="w-5 h-5" aria-hidden="true" />
            )}
          </button>

          <div className="min-w-0">
            <a
              href="#top"
              onClick={(e) => {
                e.preventDefault();
                handleSelectMobileTab('console');
              }}
              className="block text-base sm:text-lg font-bold tracking-tight text-slate-900 truncate focus-visible:outline-2 focus-visible:outline-offset-2 focus-visible:outline-blue-600"
            >
              CoreFlow Engine
            </a>
            <div className="lg:hidden text-xs text-slate-500 truncate">
              {activeItem.label}
            </div>
          </div>
        </div>

        {/* Desktop / landscape inline navigation (lg+) */}
        <nav aria-label="Primary sections" className="hidden lg:flex items-center gap-6">
          {NAV_ITEMS.map((item) => {
            const isActive = activeTab === item.id;
            return (
              <button
                key={item.id}
                type="button"
                onClick={() => onSelectTab(item.id)}
                className={`py-1.5 text-sm font-medium transition-colors whitespace-nowrap shrink-0 border-b-2 focus-visible:outline-2 focus-visible:outline-offset-2 focus-visible:outline-blue-600 ${
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
          {isNativeBridge ? (
            <button
              type="button"
              onClick={onRefreshNative}
              className="inline-flex items-center justify-center gap-2 min-h-[44px] lg:min-h-0 px-3.5 sm:px-4 py-2 text-xs font-semibold rounded-lg bg-slate-900 text-white hover:bg-slate-800 transition-colors whitespace-nowrap shrink-0 focus-visible:outline-2 focus-visible:outline-offset-2 focus-visible:outline-blue-600"
            >
              <RefreshCw className="w-3.5 h-3.5" aria-hidden="true" />
              <span>Sync</span>
            </button>
          ) : (
            <button
              type="button"
              onClick={onToggleRunning}
              className={`inline-flex items-center justify-center gap-2 min-h-[44px] lg:min-h-0 px-3.5 sm:px-4 py-2 text-xs font-semibold rounded-lg transition-colors whitespace-nowrap shrink-0 focus-visible:outline-2 focus-visible:outline-offset-2 focus-visible:outline-blue-600 ${
                isRunning
                  ? 'bg-slate-900 text-white hover:bg-slate-800'
                  : 'bg-blue-600 text-white hover:bg-blue-700'
              }`}
            >
              {isRunning ? (
                <>
                  <Pause className="w-3.5 h-3.5" aria-hidden="true" />
                  <span>Pause</span>
                </>
              ) : (
                <>
                  <Play className="w-3.5 h-3.5" aria-hidden="true" />
                  <span>Resume</span>
                </>
              )}
            </button>
          )}
        </div>
      </div>

      {/* Portrait dropdown menu drawer (< lg) */}
      {menuOpen && (
        <>
          <div
            aria-hidden="true"
            onClick={() => setMenuOpen(false)}
            className="lg:hidden fixed inset-0 top-[65px] bg-slate-900/20 z-20"
          />
          <nav
            id="mobile-primary-nav"
            aria-label="Mobile sections"
            className="lg:hidden relative z-30 bg-white border-t border-slate-200 px-3 py-2 shadow-lg divide-y divide-slate-100"
          >
            {NAV_ITEMS.map((item) => {
              const isActive = activeTab === item.id;
              return (
                <button
                  key={item.id}
                  type="button"
                  onClick={() => handleSelectMobileTab(item.id)}
                  className={`w-full min-h-[48px] px-3 py-2.5 flex items-center justify-between gap-3 text-left rounded-lg transition-colors focus-visible:outline-2 focus-visible:outline-offset-2 focus-visible:outline-blue-600 ${
                    isActive
                      ? 'bg-blue-50/70 text-blue-900'
                      : 'text-slate-700 hover:bg-slate-50 hover:text-slate-900'
                  }`}
                >
                  <div className="min-w-0">
                    <div className="text-sm font-semibold truncate">{item.label}</div>
                    <div className="text-xs text-slate-500 truncate">{item.summary}</div>
                  </div>
                  {isActive && (
                    <Check className="w-4 h-4 text-blue-600 shrink-0" aria-hidden="true" />
                  )}
                </button>
              );
            })}
          </nav>
        </>
      )}
    </header>
  );
};
