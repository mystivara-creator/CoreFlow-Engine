/**
 * CFAE System Framework v3.1.0 JavaScript RPC Bridge for KernelSU & APatch
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
    '[ -f "$BIN" ] || BIN="/system/bin/coreflowd"; ' +
    'chmod 755 "$BIN" 2>/dev/null';

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

  var CoreFlow = {
    /**
     * Check if KernelSU JavaScript root bridge is present
     * @returns {boolean}
     */
    hasBridge: function () {
      return typeof window !== 'undefined' && typeof window.ksu?.exec === 'function';
    },

    /**
     * Execute shell command via KernelSU bridge
     * @param {string} cmd
     * @returns {Promise<{errno: number, stdout: string, stderr: string}>}
     */
    exec: function (cmd) {
      if (!this.hasBridge()) {
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
    },

    /**
     * Query CFAE daemon status
     * @returns {Promise<Object>}
     */
    getStatus: async function () {
      var res = await this.exec(RESOLVE_BIN + '; "$BIN" --status');
      if (res.errno !== 0) throw new Error(res.stderr || 'Command failed');
      return parseCleanJson(res.stdout);
    },

    /**
     * Query hardware capabilities
     * @returns {Promise<Object>}
     */
    getCapabilities: async function () {
      var res = await this.exec(RESOLVE_BIN + '; "$BIN" --capabilities');
      if (res.errno !== 0) throw new Error(res.stderr || 'Command failed');
      return parseCleanJson(res.stdout);
    },

    /**
     * Set adaptive bias (battery, balanced, responsiveness)
     * @param {'battery'|'balanced'|'responsiveness'} bias
     */
    setBias: async function (bias) {
      var conf = '/data/adb/coreflow/coreflow.conf';
      var cmd = 'sed -i "s/^ui_adaptive_bias[[:space:]]*=.*/ui_adaptive_bias = ' + bias + '/" ' + conf + '; ' +
                RESOLVE_BIN + '; killall -HUP coreflowd 2>/dev/null || true';
      var res = await this.exec(cmd);
      return res.errno === 0;
    },

    /**
     * Start the daemon in detached background session mode
     */
    startDaemon: async function () {
      var log = '/data/adb/coreflow/logs/coreflowd.log';
      var cmd = 'rm -f /data/adb/coreflow/SAFE_MODE /data/adb/coreflow/DISABLE 2>/dev/null; ' +
                'mkdir -p /data/adb/coreflow/logs 2>/dev/null; ' +
                RESOLVE_BIN + '; "$BIN" --daemon >> "' + log + '" 2>&1 </dev/null';
      var res = await this.exec(cmd);
      return res.errno === 0;
    },

    /**
     * Rollback all mutations to factory baseline
     */
    rollbackBaseline: async function () {
      var res = await this.exec(RESOLVE_BIN + '; "$BIN" --rollback-baseline');
      return res.errno === 0;
    }
  };

  global.CoreFlow = CoreFlow;
})(typeof window !== 'undefined' ? window : this);
