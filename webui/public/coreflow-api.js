/**
 * CFAE System Framework v6.0.0 JavaScript RPC Bridge for KernelSU, APatch & Magisk (KsuWebUI/MMRL)
 * Read-only diagnostic surface for native daemon status, capabilities, journal, and diagnostic bundle.
 * @license Apache-2.0
 */
(function (global) {
  'use strict';

  var RESOLVE_BIN =
    'BIN="/data/adb/modules_update/cfae_framework/system/bin/coreflowd"; ' +
    '[ -x "$BIN" ] || BIN="/data/adb/modules/cfae_framework/system/bin/coreflowd"; ' +
    '[ -x "$BIN" ] || BIN="/data/adb/modules/coreflow_engine/system/bin/coreflowd"; ' +
    '[ -x "$BIN" ] || BIN="/data/adb/modules/cfae_framework/coreflowd"; ' +
    '[ -x "$BIN" ] || BIN="/data/adb/modules/coreflow_engine/coreflowd"; ' +
    '[ -x "$BIN" ] || BIN="/system_ext/bin/coreflowd"; ' +
    '[ -x "$BIN" ] || BIN="/system/bin/coreflowd"';

  function parseCleanJson(raw) {
    var trimmed = String(raw || '').trim();
    var lines = trimmed.split('\n').map(function (l) { return l.trim(); }).filter(Boolean);
    for (var i = lines.length - 1; i >= 0; i--) {
      if (lines[i].charAt(0) === '{' && lines[i].charAt(lines[i].length - 1) === '}') {
        try { return JSON.parse(lines[i]); } catch (e) {}
      }
    }
    var first = trimmed.indexOf('{');
    var last = trimmed.lastIndexOf('}');
    if (first >= 0 && last > first) {
      return JSON.parse(trimmed.slice(first, last + 1).replace(/\r?\n/g, ''));
    }
    return JSON.parse(lines[lines.length - 1] || '{}');
  }

  function hasBridge() {
    return typeof window !== 'undefined' && typeof window.ksu?.exec === 'function';
  }

  var EXEC_TIMEOUT_MS = 10000;
  var callbackSeq = 0;

  // Private implementation detail: never expose arbitrary root shell execution
  // to page scripts. The public API below is intentionally read-only.
  function execCommand(cmd) {
    if (!hasBridge()) {
      return Promise.reject(new Error('KernelSU bridge not available'));
    }
    return new Promise(function (resolve) {
      var settled = false;
      var timer = null;
      callbackSeq += 1;
      var callbackName = '__cf_js_' + Date.now() + '_' + callbackSeq;

      function finish(result) {
        if (settled) return;
        settled = true;
        if (timer !== null) clearTimeout(timer);
        try { delete window[callbackName]; } catch (e) { window[callbackName] = undefined; }
        resolve(result);
      }

      // A bridge that never calls back must not leave the UI waiting forever.
      timer = setTimeout(function () {
        finish({ errno: -1, stdout: '', stderr: 'Native command timed out after ' + EXEC_TIMEOUT_MS + 'ms' });
      }, EXEC_TIMEOUT_MS);

      window[callbackName] = function (errno, stdout, stderr) {
        finish({
          errno: typeof errno === 'number' && isFinite(errno) ? errno : 0,
          stdout: String(stdout || ''),
          stderr: String(stderr || '')
        });
      };

      try {
        window.ksu.exec(cmd, '{}', callbackName);
      } catch (e) {
        try {
          var direct = window.ksu.exec(cmd);
          finish({ errno: 0, stdout: String(direct || ''), stderr: '' });
        } catch (err) {
          finish({ errno: -1, stdout: '', stderr: String(err) });
        }
      }
    });
  }

  var CoreFlow = {
    version: '6.0.0',
    hasBridge: hasBridge,
    getStatus: async function () {
      var res = await execCommand(RESOLVE_BIN + '; "$BIN" --status');
      if (res.errno !== 0) throw new Error(res.stderr || 'Command failed');
      return parseCleanJson(res.stdout);
    },
    getCapabilities: async function () {
      var res = await execCommand(RESOLVE_BIN + '; "$BIN" --capabilities');
      if (res.errno !== 0) throw new Error(res.stderr || 'Command failed');
      return parseCleanJson(res.stdout);
    },
    getAtlas: async function () {
      var res = await execCommand(RESOLVE_BIN + '; "$BIN" --atlas');
      if (res.errno !== 0) throw new Error(res.stderr || 'Command failed');
      return parseCleanJson(res.stdout);
    },
    getEpisodes: async function () {
      var res = await execCommand(RESOLVE_BIN + '; "$BIN" --episodes');
      if (res.errno !== 0) throw new Error(res.stderr || 'Command failed');
      var payload = parseCleanJson(res.stdout);
      return Array.isArray(payload && payload.episodes) ? payload.episodes : [];
    },
    getJournal: async function () {
      var res = await execCommand(RESOLVE_BIN + '; "$BIN" --journal');
      if (res.errno !== 0) throw new Error(res.stderr || 'Command failed');
      return parseCleanJson(res.stdout);
    },
    getDiag: async function () {
      var res = await execCommand(RESOLVE_BIN + '; "$BIN" --diag');
      if (res.errno !== 0) throw new Error(res.stderr || 'Command failed');
      return parseCleanJson(res.stdout);
    }
  };

  global.CoreFlow = CoreFlow;
})(typeof window !== 'undefined' ? window : this);
