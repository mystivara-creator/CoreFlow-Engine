export type Theme = 'system' | 'light' | 'dark';
export type AccentSeed = 'ocean' | 'emerald' | 'amber';

export interface Appearance {
  theme: Theme;
  reduceMotion: boolean;
  accent?: AccentSeed;
}

export const APPEARANCE_KEY = 'coreflow.appearance.v1';

const VALID_THEMES: readonly Theme[] = ['system', 'light', 'dark'];
const VALID_ACCENTS: readonly AccentSeed[] = ['ocean', 'emerald', 'amber'];

export function readAppearance(): Appearance {
  try {
    const raw = typeof window !== 'undefined' ? window.localStorage?.getItem(APPEARANCE_KEY) : null;
    const value = JSON.parse(raw || '{}');
    const theme: Theme = VALID_THEMES.includes(value.theme) ? value.theme : 'system';
    const accent: AccentSeed = VALID_ACCENTS.includes(value.accent) ? value.accent : 'ocean';
    return {
      theme,
      reduceMotion: value.reduceMotion === true,
      accent,
    };
  } catch {
    return { theme: 'system', reduceMotion: false, accent: 'ocean' };
  }
}

export function resolveIsDark(theme: Theme): boolean {
  if (theme === 'dark') return true;
  if (theme === 'light') return false;
  return (
    typeof window !== 'undefined' &&
    Boolean(window.matchMedia?.('(prefers-color-scheme: dark)').matches)
  );
}

export function applyAppearance(value: Appearance): void {
  if (typeof document === 'undefined') return;
  const dark = resolveIsDark(value.theme);
  const accent: AccentSeed =
    value.accent && VALID_ACCENTS.includes(value.accent) ? value.accent : 'ocean';
  document.documentElement.dataset.theme = dark ? 'dark' : 'light';
  document.documentElement.dataset.motion = value.reduceMotion ? 'reduce' : 'system';
  document.documentElement.dataset.accent = accent;
}

export function saveAppearance(value: Appearance): boolean {
  applyAppearance(value);
  let stored = true;
  try {
    window.localStorage.setItem(APPEARANCE_KEY, JSON.stringify(value));
  } catch {
    stored = false;
  }
  if (typeof window !== 'undefined') {
    window.dispatchEvent(new CustomEvent('coreflow-appearance', { detail: value }));
  }
  return stored;
}
