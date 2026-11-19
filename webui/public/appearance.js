// Apply Material Design 3 display and dynamic seed preferences before first paint; zero network requests.
(function () {
  try {
    var value = JSON.parse(localStorage.getItem('coreflow.appearance.v1') || '{}');
    var theme = ['system', 'light', 'dark'].indexOf(value.theme) >= 0 ? value.theme : 'system';
    var accent = ['ocean', 'emerald', 'amber'].indexOf(value.accent) >= 0 ? value.accent : 'ocean';
    var isDark =
      theme === 'dark' ||
      (theme === 'system' &&
        window.matchMedia &&
        window.matchMedia('(prefers-color-scheme: dark)').matches);
    document.documentElement.dataset.theme = isDark ? 'dark' : 'light';
    document.documentElement.dataset.motion = value.reduceMotion === true ? 'reduce' : 'system';
    document.documentElement.dataset.accent = accent;
  } catch (_) {
    document.documentElement.dataset.theme =
      window.matchMedia && window.matchMedia('(prefers-color-scheme: dark)').matches
        ? 'dark'
        : 'light';
    document.documentElement.dataset.accent = 'ocean';
  }
})();
