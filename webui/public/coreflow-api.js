/**
 * CoreFlow JavaScript API for KernelSU & Root Environments
 * @license Apache-2.0
 */
(function (global) {
  'use strict';

  var RESOLVE_BIN =
    'BIN="/data/adb/modules/coreflow_engine/system/bin/coreflowd"; ' +
    '[ -f "$BIN" ] || BIN="/data/adb/modules/coreflow_engine/coreflowd"; ' +
    '[ -f "$BIN" ] || BIN="/system_ext/bin/coreflowd"; ' +
    '[ -f "$BIN" ] || BIN="/system/bin/coreflowd"; ' +
    'chmod 755 "$BIN" 2>/dev/null';

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
     * Query CoreFlow daemon status
     * @returns {Promise<Object>}
     */
    getStatus: async function () {
      var res = await this.exec(RESOLVE_BIN + '; "$BIN" --status');
      if (res.errno !== 0) throw new Error(res.stderr || 'Command failed');
      var lines = res.stdout.trim().split('\n');
      return JSON.parse(lines[lines.length - 1]);
    },

    /**
     * Query hardware capabilities
     * @returns {Promise<Object>}
     */
    getCapabilities: async function () {
      var res = await this.exec(RESOLVE_BIN + '; "$BIN" --capabilities');
      if (res.errno !== 0) throw new Error(res.stderr || 'Command failed');
      var lines = res.stdout.trim().split('\n');
      return JSON.parse(lines[lines.length - 1]);
    },

    /**
     * Set adaptive bias (battery, balanced, responsiveness)
     * @param {'battery'|'balanced'|'responsiveness'} bias
     */
    setBias: async function (bias) {
      var conf = '/data/adb/coreflow/coreflow.conf';
      var cmd = 'sed -i "s/^ui_adaptive_bias=.*/ui_adaptive_bias=' + bias + '/" ' + conf + '; ' +
                RESOLVE_BIN + '; killall -HUP coreflowd 2>/dev/null || true';
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
