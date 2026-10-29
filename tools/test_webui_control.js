#!/usr/bin/env node
"use strict";
// Behaviour test for the WebUI control layer (module/webroot/control_logic.js).
// Usage: node tools/test_webui_control.js
// The shell commands are executed for real, but every /data/adb/coreflow path is rewritten to
// a throwaway directory first, so the test never touches device state or running processes.
import fs from "node:fs";
import os from "node:os";
import path from "node:path";
import vm from "node:vm";
import { spawnSync } from "node:child_process";
import { fileURLToPath } from "node:url";

const __dirname = path.dirname(fileURLToPath(import.meta.url));
const root = path.resolve(__dirname, "..");
function loadUmd(filePath) {
  const code = fs.readFileSync(filePath, "utf8");
  const mod = { exports: {} };
  vm.runInNewContext(code, { module: mod, exports: mod.exports, self: {} }, { filename: filePath });
  return mod.exports;
}
const C = loadUmd(path.join(root, "module/webroot/control_logic.js"));
const S = loadUmd(path.join(root, "module/webroot/engine_status.js"));

let checks = 0;
let failures = 0;
function check(cond, label) {
  checks++;
  if (!cond) {
    failures++;
    console.error("FAIL: " + label);
  }
}

// ---- validation ----
const good = {
  mutation_mode: "observe", runtime_refresh: true, interactivity_mode: "observe",
  monitor_interval: "5", min_confidence: "0.6", launch_window_ms: "2500", jank_miss_rate: "0.05",
  boost_readahead_kb: "512", boost_uclamp_launch: "20", boost_uclamp_jank: "10",
  boost_gpu_steps: "1", boost_window_ms: "3000"
};
let v = C.validate(good);
check(v.ok, "defaults-like form validates");
check(v.values.mutation_armed === "true" && v.values.mutation_mode === "adaptive", "obsolete form modes normalize to automatic operation");
check(v.values.runtime_refresh === "true", "runtime_refresh serialised as true");
check(v.values.min_confidence === "0.6", "min_confidence canonical");

v = C.validate(Object.assign({}, good, { mutation_mode: "adaptive" }));
check(v.ok && v.values.mutation_armed === "true", "adaptive derives armed=true");

const bounds = [
  ["monitor_interval", "0"], ["monitor_interval", "61"], ["min_confidence", "0.4"],
  ["min_confidence", "1.5"], ["jank_miss_rate", "0.95"], ["jank_miss_rate", "0"],
  ["boost_uclamp_launch", "101"], ["boost_window_ms", "499"], ["boost_window_ms", "15001"],
  ["launch_window_ms", "0"], ["boost_gpu_steps", "-1"], ["monitor_interval", "1e3"],
  ["min_confidence", "abc"], ["boost_readahead_kb", ""], ["jank_miss_rate", "NaN"]
];
bounds.forEach(function (pair) {
  const r = C.validate(Object.assign({}, good, { [pair[0]]: pair[1] }));
  check(!r.ok && r.errors[pair[0]], "rejects " + pair[0] + "=" + JSON.stringify(pair[1]));
});

// ---- settings cannot change runtime hold markers ----
check(C.gate({ mutation_mode: "adaptive", mutation_armed: "true" }, { safe: true }).ok,
  "settings can be repaired during a recovery hold without clearing it");
let g = C.gate({ mutation_mode: "adaptive", mutation_armed: "true" }, { currentMode: "observe", disable: true });
check(g.ok && g.confirm === null && g.clearDisable === false, "saving settings preserves an operator stop");
g = C.gate({ mutation_mode: "adaptive", mutation_armed: "true" }, { currentMode: "observe", safe: false });
check(g.ok && g.confirm === null, "automatic operation needs no arm/mode dialog");
check(!C.gate({ mutation_mode: "observe" }, {}).ok, "legacy passive writes are rejected");
check(!C.gate({ mutation_mode: "adaptive", mutation_armed: "false" }, {}).ok, "inconsistent active config is rejected");

// ---- config.ini editing ----
const raw = [
  "# CoreFlow config, edited by hand",
  "monitor_interval=9",
  "mutation_mode=observe",
  "allow_cpu_governor=yes",
  "grant_memory=true",
  "monitor_interval=7",
  "mutation_armed=false",
  "unknown_future_key=keep-me"
].join("\n");
const parsed = C.parseIni(raw);
check(parsed.monitor_interval === "7", "parseIni: last occurrence wins");
const f = C.formFromConfig(parsed);
check(f.monitor_interval === "7" && f.min_confidence === "0.6" && f.runtime_refresh === true,
  "formFromConfig fills defaults for absent keys");

const v2 = C.validate(Object.assign({}, f, { boost_window_ms: "4000", mutation_mode: "adaptive" }));
const out = C.applyValues(raw, v2.values);
check(out.indexOf("# CoreFlow config, edited by hand") === 0, "comments preserved");
check(out.indexOf("allow_cpu_governor=yes") >= 0 && out.indexOf("grant_memory=true") >= 0,
  "legacy grant/allow lines preserved untouched");
check(out.indexOf("unknown_future_key=keep-me") >= 0, "unknown keys preserved");
check(out.split("\n").filter(function (l) { return l.indexOf("monitor_interval=") === 0; }).length === 1,
  "duplicate owned keys collapse to one line");
check(out.indexOf("mutation_mode=adaptive") >= 0 && out.indexOf("mutation_armed=true") >= 0,
  "mode and armed rewritten together");
check(out.indexOf("boost_window_ms=4000") >= 0, "new numeric values appended or replaced");
check(out.endsWith("\n") && !out.endsWith("\n\n"), "single trailing newline");

// ---- probe & snapshot parsing ----
const snap = "@@PROBE\nPIDS=1234 5678\nSUP=1200\nSUP_ALIVE=1\nSAFE=0\nDISABLE=1\nPENDING=1\nBOOT=2\n" +
  "@@STATUS\nNOW=1760000002\n{\"schema\":1}\n\n@@END\n";
const parts = C.splitSnapshot(snap);
const probe = C.parseProbe(parts.probe);
check(probe.running && probe.pids.length === 2 && probe.supervisorAlive, "probe: pids and supervisor");
check(probe.disable && probe.pending && !probe.safe, "probe: safety flags");
check(parts.status.indexOf("NOW=1760000002") === 0, "status section starts with the device clock");
check(C.parseProbe("PIDS=\n").running === false, "no pids means not running");
check(C.splitSnapshot("garbage").probe === "", "incomplete bridge output yields no probe");

// ---- decision journal ----
const trace = [
  "{\"event\":\"actuator_apply_completed\",\"result\":\"OK\",\"reason\":\"<img src=x>\",\"timestamp\":\"t1\",\"state\":\"Normal\",\"decision\":\"Apply\",\"safety_hold\":\"NONE\"}",
  "not json at all",
  "{\"event\":\"scope_gate_checked\",\"result\":\"DENY\",\"reason\":\"cpu_governor_opt_in_disabled\",\"timestamp\":\"t2\",\"safety_hold\":\"NONE\"}",
  "{\"event\":\"thermal_guard\",\"result\":\"HOLD\",\"reason\":\"hot\",\"timestamp\":\"t3\",\"safety_hold\":\"THERMAL\"}",
  "[1,2,3]"
].join("\n");
const rows = C.parseTrace(trace, 300);
check(rows.length === 3, "journal skips non-JSON and non-object lines");
check(rows[0].event === "thermal_guard" && rows[0].kind === "safety", "newest first; hold classified as safety");
check(rows.some(function (r) { return r.reason === "<img src=x>"; }), "hostile text kept as plain data");
check(rows.some(function (r) { return r.kind === "change"; }), "applied actuator classified as change");
check(C.filterTrace(rows, "errors").length === 2, "errors filter includes safety holds");
check(C.filterTrace(rows, "changes").length === 1, "changes filter");

// ---- real command execution against a sandbox ----
function sandbox() {
  const dir = fs.mkdtempSync(path.join(os.tmpdir(), "cf-webui-"));
  fs.writeFileSync(path.join(dir, "config.ini"), raw + "\n");
  return dir;
}
function rewrite(cmd, dir) {
  return cmd.split(C.PATHS.base).join(dir);
}
function sh(cmd) {
  const r = spawnSync("sh", ["-c", cmd], { encoding: "utf8", timeout: 20000 });
  return { status: r.status, stdout: r.stdout || "", stderr: r.stderr || "" };
}

// Syntax-check every builder, including the ones that are not executed below.
[
  C.snapshotCmd(), C.readConfigCmd(), C.readTraceCmd(), C.readLogCmd(),
  C.startCmd(), C.stopCmd(), C.restartCmd(), C.refreshCmd(),
  C.writeConfigCmd(out, v2.values, { clearDisable: true }),
  C.writeConfigCmd(out, C.validate(Object.assign({}, f, { mutation_mode: "observe" })).values, {})
].forEach(function (cmd, i) {
  const r = sh("sh -n -c " + shq(cmd));
  check(r.status === 0, "sh -n accepts builder #" + i + " " + r.stderr.trim());
});
function shq(s) { return "'" + String(s).replace(/'/g, "'\\''") + "'"; }

// Write Adaptive: must update config, clear DISABLE and mark the config pending.
let dir = sandbox();
fs.writeFileSync(path.join(dir, "DISABLE"), "");
let r = sh(rewrite(C.writeConfigCmd(out, v2.values, { clearDisable: true }), dir));
check(r.status === 0 && r.stdout.indexOf("CONFIG_COMMIT_OK") >= 0, "adaptive write commits: " + r.stderr.trim());
let written = fs.readFileSync(path.join(dir, "config.ini"), "utf8");
check(written.indexOf("mutation_mode=adaptive") >= 0 && written.indexOf("allow_cpu_governor=yes") >= 0,
  "adaptive write keeps legacy lines");
check(fs.existsSync(path.join(dir, "DISABLE")), "saving settings preserves DISABLE even with obsolete clearDisable option");
check(fs.existsSync(path.join(dir, "webui_config_pending")), "write marks restart pending");
check((fs.statSync(path.join(dir, "config.ini")).mode & 0o077) === 0, "config is private (0600)");

// Obsolete passive form inputs cannot create a hidden mode or disarm the engine.
const obs = C.validate(Object.assign({}, f, { mutation_mode: "observe" }));
r = sh(rewrite(C.writeConfigCmd(C.applyValues(written, obs.values), obs.values, {}), dir));
check(r.status === 0 && fs.existsSync(path.join(dir, "DISABLE")), "settings never remove an existing operator stop");
written = fs.readFileSync(path.join(dir, "config.ini"), "utf8");
check(written.indexOf("mutation_armed=true") >= 0 && written.indexOf("mutation_mode=adaptive") >= 0, "legacy passive form is saved as active config");
fs.rmSync(dir, { recursive: true, force: true });

// A missing state directory must fail closed without creating anything.
r = sh(rewrite(C.writeConfigCmd(out, v2.values, {}), "/nonexistent-cf-" + process.pid));
check(r.status !== 0 && r.stdout.indexOf("CONFIG_COMMIT_OK") < 0, "missing directory fails closed");

// Snapshot against a sandbox: status.json and probe flags are read back as data.
dir = sandbox();
fs.writeFileSync(path.join(dir, "status.json"), "{\"schema\":1}");
fs.writeFileSync(path.join(dir, "SAFE_MODE"), "");
r = sh(rewrite(C.snapshotCmd(), dir));
const snapParts = C.splitSnapshot(r.stdout);
const p2 = C.parseProbe(snapParts.probe);
check(p2.safe === true && p2.disable === false, "live snapshot reads SAFE_MODE from disk");
check(typeof S.parse(snapParts.status).ok === "boolean", "status section parses through the status contract");
fs.rmSync(dir, { recursive: true, force: true });

// Stop and start with no daemon and no module: both report and exit cleanly, nothing is killed.
dir = sandbox();
const noPidof = path.join(dir, "bin");
fs.mkdirSync(noPidof);
const env = { PATH: noPidof + ":/usr/bin:/bin" };
// Keep only a fake pidof that reports nothing, so no real process can match.
fs.writeFileSync(path.join(noPidof, "pidof"), "#!/bin/sh\nexit 1\n", { mode: 0o755 });
const stopRun = spawnSync("sh", ["-c", rewrite(C.stopCmd(), dir)], { encoding: "utf8", env: env, timeout: 20000 });
check(stopRun.stdout.indexOf("STOP_OK") >= 0, "stop with no daemon reports STOP_OK");
const startRun = spawnSync("sh", ["-c", rewrite(C.startCmd(), dir)], { encoding: "utf8", env: env, timeout: 20000 });
check(startRun.stdout.indexOf("MODULE_MISSING") >= 0 && startRun.status === 21, "start without a module fails closed");
const refreshRun = spawnSync("sh", ["-c", C.refreshCmd()], { encoding: "utf8", env: env, timeout: 20000 });
check(refreshRun.stdout.indexOf("NOT_RUNNING") >= 0 && refreshRun.status === 2, "refresh without a daemon does nothing");
fs.rmSync(dir, { recursive: true, force: true });

// Explicit Start resumes DISABLE, but refuses SAFE_MODE. A fake running daemon
// avoids launching a service or sending any real process a signal.
dir = sandbox();
const stub = path.join(dir, "service.sh");
fs.writeFileSync(stub, "#!/bin/sh\nexit 0\n", { mode: 0o755 });
fs.mkdirSync(path.join(dir, "bin"));
fs.writeFileSync(path.join(dir, "bin", "pidof"), "#!/bin/sh\necho 999999\n", { mode: 0o755 });
const resumeEnv = { PATH: path.join(dir, "bin") + ":/usr/bin:/bin" };
fs.writeFileSync(path.join(dir, "DISABLE"), "");
fs.writeFileSync(path.join(dir, "SAFE_MODE"), "");
let resumeCmd = rewrite(C.startCmd(), dir).split(C.PATHS.service).join(stub);
let resumed = spawnSync("sh", ["-c", resumeCmd], { encoding: "utf8", env: resumeEnv, timeout: 20000 });
check(resumed.status === 22 && fs.existsSync(path.join(dir, "DISABLE")), "Start preserves operator stop while SAFE_MODE is present");
fs.rmSync(path.join(dir, "SAFE_MODE"));
resumed = spawnSync("sh", ["-c", resumeCmd], { encoding: "utf8", env: resumeEnv, timeout: 20000 });
check(resumed.status === 0 && !fs.existsSync(path.join(dir, "DISABLE")), "explicit Start resumes an operator stop");
fs.rmSync(dir, { recursive: true, force: true });

console.log("CoreFlow WebUI control contract: " + (checks - failures) + "/" + checks + " checks passed");
process.exit(failures ? 1 : 0);
