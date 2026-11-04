export type Theme = 'system' | 'light' | 'dark';

export interface Appearance {
  theme: Theme;
  reduceMotion: boolean;
}

export const APPEARANCE_KEY = 'coreflow.appearance.v1';

export function readAppearance(): Appearance {
  try {
    const value = JSON.parse(window.localStorage?.getItem(APPEARANCE_KEY) || '{}');
    return {
      theme: ['system', 'light', 'dark'].includes(value.theme) ? value.theme : 'system',
      reduceMotion: value.reduceMotion === true,
    };
  } catch {
    return { theme: 'system', reduceMotion: false };
  }
}

export function applyAppearance(value: Appearance): void {
  const dark =
    value.theme === 'dark' ||
    (value.theme === 'system' && window.matchMedia?.('(prefers-color-scheme: dark)').matches);
  document.documentElement.dataset.theme = dark ? 'dark' : 'light';
  document.documentElement.dataset.motion = value.reduceMotion ? 'reduce' : 'system';
}

export function saveAppearance(value: Appearance): boolean {
  applyAppearance(value);
  let stored = true;
  try {
    window.localStorage.setItem(APPEARANCE_KEY, JSON.stringify(value));
  } catch {
    stored = false;
  }
  window.dispatchEvent(new CustomEvent('coreflow-appearance', { detail: value }));
  return stored;
}
