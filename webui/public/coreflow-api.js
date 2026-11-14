/**
 * CFAE System Framework v4.0.0 JavaScript RPC Bridge for KernelSU & APatch
 * @license MIT
 */
(function (global) {
  'use strict';

  var RESOLVE_BIN =
    'BIN="/data/adb/modules_update/cfae_framework/system/bin/coreflowd"; ' +
    '[ -f "$BIN" ] || BIN="/data/adb/modules/cfae_framework/system/bin/coreflowd"; ' +
    '[ -f "$BIN" ] || BIN="/data/adb/modules/coreflow_engine/system/bin/coreflowd"; ' +
    '[ -f "$BIN" ] || BIN="/data/adb/modules/cfae_framework/coreflowd"; ' +
    '[ -f "$BIN" ] || BIN="/data/adb/modules/coreflow_engine/coreflowd"; ' +
    '[ -f "$BIN" ] || BIN="/system_ext/bin/coreflowd"; ' +
    '[ -f "$BIN" ] || BIN="/system/bin/coreflowd"';

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

  // Private implementation detail: never expose arbitrary root shell execution
  // to page scripts. The public API below is intentionally read-only.
  function execCommand(cmd) {
    if (!hasBridge()) {
      return Promise.reject(new Error('KernelSU bridge not available'));
    }
    return new Promise(function (resolve) {
      var callbackName = '__cf_js_' + Date.now() + '_' + Math.floor(Math.random() * 10000);
      window[callbackName] = function (errno, stdout, stderr) {
        delete window[callbackName];
        resolve({
          errno: typeof errno === 'number' ? errno : 0,
          stdout: String(stdout || ''),
          stderr: String(stderr || '')
        });
      };
      try {
        window.ksu.exec(cmd, '{}', callbackName);
      } catch (e) {
        try {
          var direct = window.ksu.exec(cmd);
          resolve({ errno: 0, stdout: String(direct || ''), stderr: '' });
        } catch (err) {
          resolve({ errno: -1, stdout: '', stderr: String(err) });
        }
      }
    });
  }

  var CoreFlow = {
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
    }
  };

  global.CoreFlow = CoreFlow;
})(typeof window !== 'undefined' ? window : this);
