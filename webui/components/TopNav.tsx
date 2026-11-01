import React, { useEffect, useRef, useState } from 'react';
import { Check, Cpu, Menu, Pause, Play, RefreshCw, X } from 'lucide-react';

import { AppearanceControls } from './AppearanceControls';

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
  { id: 'sysfs', label: 'Sysfs Inspector', summary: 'Discovered kernel & ZRAM capabilities' },
  { id: 'policy', label: 'Policy & Config', summary: 'Adaptive gate & frame pacing bounds' },
  { id: 'baseline', label: 'Baseline & Thermal', summary: 'ONNX 18f predictor & outcome scores' },
  { id: 'logcat', label: 'Journal & Logcat', summary: 'Mutation journal & conscience trace' },
];

const SLIDE_OFFSET_BY_INDEX: Record<number, string> = {
  0: 'translate-x-0',
  1: 'translate-x-full',
  2: 'translate-x-[200%]',
  3: 'translate-x-[300%]',
  4: 'translate-x-[400%]',
};

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

  const activeIndex = Math.max(
    0,
    NAV_ITEMS.findIndex((item) => item.id === activeTab)
  );
  const activeItem = NAV_ITEMS[activeIndex] ?? NAV_ITEMS[0];
  const slideClass = SLIDE_OFFSET_BY_INDEX[activeIndex] ?? 'translate-x-0';

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
      className="sticky top-0 z-30 bg-white/95 backdrop-blur-md border-b border-slate-200"
    >
      <div className="flex items-center justify-between gap-3 px-4 sm:px-6 py-2.5">
        <div className="flex items-center gap-2.5 min-w-0">
          <button
            type="button"
            aria-label={menuOpen ? 'Close navigation menu' : 'Open navigation menu'}
            aria-expanded={menuOpen}
            aria-controls="mobile-primary-nav"
            onClick={() => setMenuOpen((prev) => !prev)}
            className="lg:hidden inline-flex items-center justify-center min-h-[42px] min-w-[42px] -ml-1 rounded-lg text-slate-700 hover:text-slate-900 hover:bg-slate-100 shrink-0"
          >
            {menuOpen ? (
              <X className="w-5 h-5" aria-hidden="true" />
            ) : (
              <Menu className="w-5 h-5" aria-hidden="true" />
            )}
          </button>

          <a
            href="#top"
            onClick={(e) => {
              e.preventDefault();
              handleSelectMobileTab('console');
            }}
            className="flex items-center gap-2.5 min-w-0 rounded-lg py-0.5 pr-1"
          >
            <span className="hidden sm:inline-flex items-center justify-center w-8 h-8 rounded-lg bg-slate-900 text-white shrink-0">
              <Cpu className="w-4 h-4" aria-hidden="true" />
            </span>
            <span className="min-w-0">
              <span className="block text-sm sm:text-base font-bold tracking-tight text-slate-900 truncate">
                CoreFlow Engine
              </span>
              <span className="lg:hidden block text-[11px] text-slate-500 truncate">
                {activeItem.label}
              </span>
              <span className="hidden lg:block text-[11px] font-mono text-slate-500 truncate">
                {isNativeBridge ? 'KernelSU Native Runtime' : 'v3.1.1 · Closed-Loop System Engine'}
              </span>
            </span>
          </a>
        </div>

        {/* Desktop / landscape sliding segmented navigation (lg+) */}
        <nav
          aria-label="Primary sections"
          className="hidden lg:grid grid-cols-5 relative p-1 bg-slate-100 border border-slate-200/80 rounded-xl"
        >
          <span
            aria-hidden="true"
            className={`pointer-events-none absolute top-1 bottom-1 left-1 w-[calc((100%-0.5rem)/5)] rounded-lg bg-white shadow-xs border border-slate-200/80 transition-transform duration-300 ease-[cubic-bezier(0.16,1,0.3,1)] ${slideClass}`}
          />
          {NAV_ITEMS.map((item) => {
            const isActive = activeTab === item.id;
            return (
              <button
                key={item.id}
                type="button"
                aria-current={isActive ? 'page' : undefined}
                onClick={() => onSelectTab(item.id)}
                className={`relative z-10 px-3 xl:px-4 py-1.5 text-xs font-semibold rounded-lg transition-colors whitespace-nowrap text-center ${
                  isActive
                    ? 'text-slate-900'
                    : 'text-slate-600 hover:text-slate-900'
                }`}
              >
                {item.label}
              </button>
            );
          })}
        </nav>

        <div className="flex items-center shrink-0">
          <AppearanceControls compact />
          {isNativeBridge ? (
            <button
              type="button"
              onClick={onRefreshNative}
              className="inline-flex items-center justify-center gap-2 h-9 px-3.5 text-xs font-semibold rounded-lg bg-slate-900 text-white hover:bg-slate-800 whitespace-nowrap shrink-0"
            >
              <RefreshCw className="w-3.5 h-3.5" aria-hidden="true" />
              <span>Sync</span>
            </button>
          ) : (
            <button
              type="button"
              onClick={onToggleRunning}
              className={`inline-flex items-center justify-center gap-2 h-9 px-3.5 text-xs font-semibold rounded-lg whitespace-nowrap shrink-0 ${
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
            className="lg:hidden fixed inset-0 top-[61px] bg-slate-900/25 backdrop-blur-[1px] z-20"
          />
          <nav
            id="mobile-primary-nav"
            aria-label="Mobile sections"
            className="cf-view-stage lg:hidden relative z-30 bg-white border-t border-slate-200 px-3 py-2 shadow-lg divide-y divide-slate-100"
          >
            {NAV_ITEMS.map((item) => {
              const isActive = activeTab === item.id;
              return (
                <button
                  key={item.id}
                  type="button"
                  onClick={() => handleSelectMobileTab(item.id)}
                  className={`w-full min-h-[48px] px-3 py-2.5 flex items-center justify-between gap-3 text-left rounded-lg transition-colors ${
                    isActive
                      ? 'bg-blue-50/80 text-blue-900'
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
