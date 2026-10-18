/* CoreFlow control logic: pure functions only (no DOM, no shell execution).
 *
 * The WebUI never accepts free-form shell input. Every command is built here from
 * fixed paths and validated values, then sent through the KernelSU exec bridge.
 * The daemon stays the only authority over kernel writes: the UI only edits
 * config.ini, touches the documented marker files, sends SIGTERM/SIGUSR1 to
 * coreflowd, and asks the module supervisor (service.sh) to start it.
 *
 * Tested with node by tools/test_webui_control.js.
 */
(function (root) {
  "use strict";

  var BASE = "/data/adb/coreflow";
  var MODULE = "/data/adb/modules/coreflow_autonomous";

  var PATHS = {
    base: BASE,
    config: BASE + "/config.ini",
    pid: BASE + "/coreflowd.pid",
    status: BASE + "/status.json",
    trace: BASE + "/decision_trace.jsonl",
    log: BASE + "/logs/coreflowd.log",
    safe: BASE + "/SAFE_MODE",
    disable: BASE + "/DISABLE",
    pending: BASE + "/webui_config_pending",
    restartRequest: BASE + "/webui_restart_request",
    bootAttempts: BASE + "/boot_attempts",
    service: MODULE + "/service.sh"
  };

  // Values the daemon uses when a key is absent (src/runtime/config.hpp).
  var DEFAULTS = {
    mutation_mode: "observe",
    runtime_refresh: "true",
    interactivity_mode: "observe",
    monitor_interval: "5",
    min_confidence: "0.6",
    launch_window_ms: "2500",
    jank_miss_rate: "0.05",
    boost_readahead_kb: "512",
    boost_uclamp_launch: "20",
    boost_uclamp_jank: "10",
    boost_gpu_steps: "1",
    boost_window_ms: "3000"
  };

  // Numeric keys the Policy page owns. Bounds mirror EngineConfig's clamps; the
  // UI rejects out-of-range input instead of silently clamping it.
  var NUMERIC = [
    { key: "monitor_interval", label: "Monitor interval", unit: "s", kind: "int", min: 1, max: 60, group: "runtime",
      help: "How often the engine samples the device." },
    { key: "min_confidence", label: "Minimum confidence", unit: "", kind: "real", min: 0.5, max: 1, group: "runtime",
      help: "Telemetry confidence required before the engine acts." },
    { key: "launch_window_ms", label: "Launch window", unit: "ms", kind: "int", min: 1, group: "interactivity",
      help: "Window after a detected app launch." },
    { key: "jank_miss_rate", label: "Jank miss rate", unit: "", kind: "real", min: 0.01, max: 0.9, group: "interactivity",
      help: "Frame-miss ratio that triggers a boost." },
    { key: "boost_readahead_kb", label: "Boost readahead", unit: "KB", kind: "int", min: 0, group: "interactivity",
      help: "Read-ahead applied during a boost." },
    { key: "boost_uclamp_launch", label: "Boost uclamp (launch)", unit: "%", kind: "int", min: 0, max: 100, group: "interactivity",
      help: "Utilization clamp during launch." },
    { key: "boost_uclamp_jank", label: "Boost uclamp (jank)", unit: "%", kind: "int", min: 0, max: 100, group: "interactivity",
      help: "Utilization clamp during jank." },
    { key: "boost_gpu_steps", label: "Boost GPU steps", unit: "", kind: "int", min: 0, group: "interactivity",
      help: "GPU frequency steps added during a boost." },
    { key: "boost_window_ms", label: "Boost window", unit: "ms", kind: "int", min: 500, max: 15000, group: "interactivity",
      help: "How long a boost is held." }
  ];

  // Keys this UI writes. Anything else in config.ini (including legacy grant_* and
  // allow_cpu_governor lines the daemon still parses) is preserved untouched.
  var OWNED = ["mutation_mode", "mutation_armed", "runtime_refresh", "interactivity_mode"]
    .concat(NUMERIC.map(function (f) { return f.key; }));

  function q(s) {
    return "'" + String(s).replace(/'/g, "'\\''") + "'";
  }

  function rangeText(f) {
    if (f.max != null) return "between " + f.min + " and " + f.max;
    return "at least " + f.min;
  }

  function parseBool(value, fallback) {
    var v = String(value == null ? "" : value).trim().toLowerCase();
    if (v === "1" || v === "true" || v === "yes" || v === "on") return true;
    if (v === "0" || v === "false" || v === "no" || v === "off") return false;
    return fallback;
  }

  // Last occurrence wins, matching the daemon's loader.
  function parseIni(text) {
    var out = {};
    String(text || "").replace(/\r/g, "").split("\n").forEach(function (line) {
      var t = line.trim();
      if (!t || t[0] === "#" || t[0] === ";") return;
      var eq = t.indexOf("=");
      if (eq <= 0) return;
      out[t.slice(0, eq).trim()] = t.slice(eq + 1).trim();
    });
    return out;
  }

  // Form state from a parsed config, filling defaults for absent keys.
  function formFromConfig(cfg) {
    var c = cfg || {};
    var form = {
      mutation_mode: c.mutation_mode === "adaptive" ? "adaptive" : "observe",
      runtime_refresh: parseBool(c.runtime_refresh, true),
      interactivity_mode: c.interactivity_mode === "adaptive" ? "adaptive" : "observe"
    };
    NUMERIC.forEach(function (f) {
      form[f.key] = c[f.key] != null && c[f.key] !== "" ? c[f.key] : DEFAULTS[f.key];
    });
    return form;
  }

  // Validates the raw form. Returns canonical string values for config.ini.
  function validate(form) {
    var errors = {};
    var values = {};
    NUMERIC.forEach(function (f) {
      var raw = String(form[f.key] == null ? "" : form[f.key]).trim();
      var wellFormed = f.kind === "int" ? /^\d{1,9}$/.test(raw) : /^(\d+(\.\d+)?|\.\d+)$/.test(raw);
      if (!wellFormed) {
        errors[f.key] = f.kind === "int" ? "Enter a whole number." : "Enter a decimal number.";
        return;
      }
      var n = Number(raw);
      if (!isFinite(n) || n < f.min || (f.max != null && n > f.max)) {
        errors[f.key] = "Must be " + rangeText(f) + ".";
        return;
      }
      values[f.key] = f.kind === "int" ? String(Math.floor(n)) : String(n);
    });
    values.mutation_mode = form.mutation_mode === "adaptive" ? "adaptive" : "observe";
    // mutation_armed is derived: the daemon requires it true only in Adaptive mode.
    values.mutation_armed = values.mutation_mode === "adaptive" ? "true" : "false";
    values.interactivity_mode = form.interactivity_mode === "adaptive" ? "adaptive" : "observe";
    values.runtime_refresh = form.runtime_refresh ? "true" : "false";
    return { ok: Object.keys(errors).length === 0, errors: errors, values: values };
  }

  // Safety gates for a save. ctx: { currentMode, safe, disable }.
  // Returns { ok, reason, confirm, clearDisable }. confirm names the dialog to show.
  function gate(values, ctx) {
    var c = ctx || {};
    var adaptive = values.mutation_mode === "adaptive";
    var changed = values.mutation_mode !== (c.currentMode === "adaptive" ? "adaptive" : "observe");
    if (adaptive && c.safe) {
      return { ok: false, reason: "SAFE_MODE is present. Adaptive stays blocked until the normal recovery procedure clears it." };
    }
    var confirm = null;
    if (adaptive && c.disable) confirm = "resume";
    else if (changed) confirm = adaptive ? "arm" : "observe";
    return { ok: true, confirm: confirm, clearDisable: adaptive && !!c.disable };
  }

  // Replaces owned keys in place (all duplicates collapse to one line) and appends
  // any missing ones. Unowned lines and comments are preserved byte-for-byte.
  function applyValues(raw, values) {
    var seen = {};
    var lines = String(raw || "").replace(/\r/g, "").split("\n");
    var out = [];
    lines.forEach(function (line) {
      var t = line.trim();
      var eq = t.indexOf("=");
      if (!t || t[0] === "#" || t[0] === ";" || eq <= 0) { out.push(line); return; }
      var key = t.slice(0, eq).trim();
      if (!Object.prototype.hasOwnProperty.call(values, key)) { out.push(line); return; }
      if (seen[key]) return;
      seen[key] = true;
      out.push(key + "=" + values[key]);
    });
    Object.keys(values).forEach(function (key) {
      if (!seen[key]) out.push(key + "=" + values[key]);
    });
    while (out.length && out[out.length - 1].trim() === "") out.pop();
    return out.join("\n") + "\n";
  }

  // ---- command builders (fixed text; only quoted, validated values are interpolated) ----

  function flagLine(name, path) {
    return "printf '" + name + "=%s\\n' \"$([ -e " + q(path) + " ] && echo 1 || echo 0)\"";
  }

  // One bridge round-trip returns the process probe and the status.json snapshot.
  function snapshotCmd() {
    return [
      "printf '@@PROBE\\n'",
      "printf 'PIDS=%s\\n' \"$(pidof coreflowd 2>/dev/null)\"",
      "SUP=$(cat " + q(PATHS.pid) + " 2>/dev/null)",
      "printf 'SUP=%s\\n' \"$SUP\"",
      "printf 'SUP_ALIVE=%s\\n' \"$([ -n \"$SUP\" ] && kill -0 \"$SUP\" 2>/dev/null && echo 1 || echo 0)\"",
      flagLine("SAFE", PATHS.safe),
      flagLine("DISABLE", PATHS.disable),
      flagLine("PENDING", PATHS.pending),
      "printf 'BOOT=%s\\n' \"$(cat " + q(PATHS.bootAttempts) + " 2>/dev/null)\"",
      "printf '@@STATUS\\n'",
      "printf 'NOW=%s\\n' \"$(date +%s)\"",
      "cat " + q(PATHS.status) + " 2>/dev/null",
      "printf '\\n@@END\\n'"
    ].join("; ");
  }

  function splitSnapshot(text) {
    var s = String(text || "");
    var pi = s.indexOf("@@PROBE\n");
    var si = s.indexOf("@@STATUS\n");
    if (pi < 0 || si < 0) return { probe: "", status: "" };
    var ei = s.indexOf("@@END", si);
    return {
      probe: s.slice(pi + "@@PROBE\n".length, si),
      status: s.slice(si + "@@STATUS\n".length, ei > si ? ei : s.length)
    };
  }

  function parseProbe(text) {
    var out = {
      pids: [], supervisor: "", supervisorAlive: false,
      safe: false, disable: false, pending: false, bootAttempts: ""
    };
    String(text || "").split(/\r?\n/).forEach(function (line) {
      var i = line.indexOf("=");
      if (i <= 0) return;
      var k = line.slice(0, i);
      var v = line.slice(i + 1).trim();
      if (k === "PIDS") out.pids = v.split(/\s+/).filter(function (p) { return /^\d+$/.test(p); });
      else if (k === "SUP") out.supervisor = /^\d+$/.test(v) ? v : "";
      else if (k === "SUP_ALIVE") out.supervisorAlive = v === "1";
      else if (k === "SAFE") out.safe = v === "1";
      else if (k === "DISABLE") out.disable = v === "1";
      else if (k === "PENDING") out.pending = v === "1";
      else if (k === "BOOT") out.bootAttempts = v.slice(0, 6);
    });
    out.running = out.pids.length > 0;
    return out;
  }

  function readConfigCmd() {
    return "cat " + q(PATHS.config) + " 2>/dev/null";
  }

  function readTraceCmd() {
    return "tail -n 400 " + q(PATHS.trace) + " 2>/dev/null";
  }

  function readLogCmd() {
    return "tail -n 500 " + q(PATHS.log) + " 2>/dev/null";
  }

  function stopBody() {
    return [
      "SUP=$(cat " + q(PATHS.pid) + " 2>/dev/null)",
      "[ -n \"$SUP\" ] && kill -TERM \"$SUP\" 2>/dev/null",
      "for p in $(pidof coreflowd 2>/dev/null); do kill -TERM \"$p\" 2>/dev/null; done",
      "i=0; while [ -n \"$(pidof coreflowd 2>/dev/null)\" ] && [ \"$i\" -lt 20 ]; do sleep 1; i=$((i+1)); done",
      "if [ -n \"$(pidof coreflowd 2>/dev/null)\" ]; then echo STOP_TIMEOUT; exit 1; fi",
      "echo STOP_OK"
    ].join("; ");
  }

  // Newline-joined on purpose: "&" ends a command, so it must not be followed by ";".
  function startBody() {
    return [
      "if [ -n \"$(pidof coreflowd 2>/dev/null)\" ]; then echo ALREADY_RUNNING; exit 0; fi",
      "[ -x " + q(PATHS.service) + " ] || { echo MODULE_MISSING; exit 21; }",
      "nohup sh " + q(PATHS.service) + " </dev/null >/dev/null 2>&1 &",
      "echo START_REQUESTED"
    ].join("\n");
  }

  function startCmd() {
    return startBody();
  }

  function stopCmd() {
    return stopBody();
  }

  // With a live supervisor, the restart request marker tells service.sh to relaunch
  // coreflowd without counting it as a crash. Without one, stop then start.
  function restartCmd() {
    return [
      "SUP=$(cat " + q(PATHS.pid) + " 2>/dev/null)",
      "OLD=$(pidof coreflowd 2>/dev/null)",
      "if [ -n \"$OLD\" ] && [ -n \"$SUP\" ] && kill -0 \"$SUP\" 2>/dev/null; then",
      "  touch " + q(PATHS.restartRequest),
      "  for p in $OLD; do kill -TERM \"$p\" 2>/dev/null; done",
      "  i=0; NEW=''; while [ \"$i\" -lt 30 ]; do NEW=$(pidof coreflowd 2>/dev/null); [ -n \"$NEW\" ] && [ \"$NEW\" != \"$OLD\" ] && break; sleep 1; i=$((i+1)); done",
      "  if [ -n \"$NEW\" ] && [ \"$NEW\" != \"$OLD\" ]; then echo RESTART_OK; else rm -f " + q(PATHS.restartRequest) + "; echo RESTART_TIMEOUT; exit 1; fi",
      "else",
      "  " + stopBody() + "\n" + startBody(),
      "fi"
    ].join("\n");
  }

  // SIGUSR1 asks the daemon to re-run capability discovery. It changes no tunable.
  function refreshCmd() {
    return [
      "p=$(pidof coreflowd 2>/dev/null)",
      "[ -n \"$p\" ] || { echo NOT_RUNNING; exit 2; }",
      "for one in $p; do kill -USR1 \"$one\" 2>/dev/null; done",
      "echo REFRESH_SIGNAL_SENT"
    ].join("\n");
  }

  // Writes config.ini atomically and sets the pending-restart marker. Observe creates
  // the DISABLE kill switch; Adaptive removes it only after explicit resume.
  function writeConfigCmd(newRaw, values, opts) {
    var adaptive = values.mutation_mode === "adaptive";
    var lines = [
      "set -eu",
      "CFG=" + q(PATHS.config),
      "DIR=" + q(PATHS.base),
      "TMP=\"$CFG.webui.tmp.$$\"",
      "umask 077",
      "[ -d \"$DIR\" ] || exit 21",
      "printf %s " + q(newRaw) + " > \"$TMP\"",
      "chmod 0600 \"$TMP\"",
      "[ -s \"$TMP\" ]",
      "grep -qx " + q("mutation_mode=" + values.mutation_mode) + " \"$TMP\"",
      "grep -qx " + q("mutation_armed=" + values.mutation_armed) + " \"$TMP\""
    ];
    if (!adaptive) lines.push("touch \"$DIR/DISABLE\"");
    lines.push("mv \"$TMP\" \"$CFG\"", "chmod 0600 \"$CFG\"");
    if (adaptive && opts && opts.clearDisable) lines.push("rm -f \"$DIR/DISABLE\"");
    lines.push("touch \"$DIR/webui_config_pending\"", "chmod 0600 \"$DIR/webui_config_pending\"", "echo CONFIG_COMMIT_OK");
    return lines.join("; ");
  }

  // ---- decision journal ----

  function str(v, max) {
    return typeof v === "string" ? v.slice(0, max || 160) : "";
  }

  // error: something failed. block: the policy refused an action. safety: a hold is active.
  // change: an actuator applied, verified or restored something. event: everything else.
  function classify(o) {
    var r = (String(o.result || "") + " " + String(o.event || "")).toUpperCase();
    if (/FAIL|ERROR|FAULT/.test(r)) return "error";
    if (/DENY|DENIED|REJECT|BLOCK/.test(r)) return "block";
    if (/MUTAT|APPL|VERIF|ROLLBACK|RESTORE/.test(r)) return "change";
    var hold = String(o.safety_hold || "");
    if (hold && !/^(NONE|NO_HOLD)?$/i.test(hold)) return "safety";
    return "event";
  }

  // Newest first. Non-JSON or non-object lines are skipped, never rendered as HTML.
  function parseTrace(text, limit) {
    var rows = [];
    String(text || "").split(/\r?\n/).filter(Boolean).slice(-(limit || 300)).forEach(function (line) {
      var o;
      try { o = JSON.parse(line); } catch (e) { return; }
      if (!o || typeof o !== "object" || Array.isArray(o)) return;
      rows.push({
        time: str(o.timestamp, 40),
        event: str(o.event, 80),
        scope: str(o.scope, 40),
        result: str(o.result, 60),
        reason: str(o.reason, 160),
        state: str(o.state, 40),
        decision: str(o.decision, 60),
        kind: classify(o)
      });
    });
    return rows.reverse();
  }

  function filterTrace(rows, filter) {
    if (filter === "changes") return rows.filter(function (r) { return r.kind === "change"; });
    if (filter === "errors") return rows.filter(function (r) {
      return r.kind === "error" || r.kind === "block" || r.kind === "safety";
    });
    return rows;
  }

  var api = {
    PATHS: PATHS, DEFAULTS: DEFAULTS, NUMERIC: NUMERIC, OWNED: OWNED,
    parseIni: parseIni, formFromConfig: formFromConfig, validate: validate, gate: gate, applyValues: applyValues,
    snapshotCmd: snapshotCmd, splitSnapshot: splitSnapshot, parseProbe: parseProbe,
    readConfigCmd: readConfigCmd, readTraceCmd: readTraceCmd, readLogCmd: readLogCmd,
    startCmd: startCmd, stopCmd: stopCmd, restartCmd: restartCmd, refreshCmd: refreshCmd,
    writeConfigCmd: writeConfigCmd, parseTrace: parseTrace, filterTrace: filterTrace, classify: classify
  };
  if (typeof module !== "undefined" && module.exports) module.exports = api;
  else root.CoreFlowControl = api;
})(typeof window !== "undefined" ? window : this);
