// Apply display preferences before the application paints; no network requests.
(function () {
  try {
    var value = JSON.parse(localStorage.getItem('coreflow.appearance.v1') || '{}');
    var theme = ['system', 'light', 'dark'].indexOf(value.theme) >= 0 ? value.theme : 'system';
    document.documentElement.dataset.theme = theme === 'dark' || (theme === 'system' && window.matchMedia && window.matchMedia('(prefers-color-scheme: dark)').matches) ? 'dark' : 'light';
    document.documentElement.dataset.motion = value.reduceMotion === true ? 'reduce' : 'system';
  } catch (_) {
    document.documentElement.dataset.theme = window.matchMedia && window.matchMedia('(prefers-color-scheme: dark)').matches ? 'dark' : 'light';
  }
})();
