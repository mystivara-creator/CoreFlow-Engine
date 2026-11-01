#!/usr/bin/env node
"use strict";
// Cross-language contract test: real JSON written by the C++ engine code must be
// understood by the WebUI parser. Usage: node tools/test_webui_status.js <fixture-dir>
import fs from "node:fs";
import path from "node:path";
import vm from "node:vm";
import { fileURLToPath } from "node:url";

const __dirname = path.dirname(fileURLToPath(import.meta.url));
const root = path.resolve(__dirname, "..");
const dir = process.argv[2];
if (!dir) { console.error("usage: test_webui_status.js <fixture-dir>"); process.exit(2); }

function loadUmd(filePath) {
  const code = fs.readFileSync(filePath, "utf8");
  const mod = { exports: {} };
  vm.runInNewContext(code, { module: mod, exports: mod.exports, self: {} }, { filename: filePath });
  return mod.exports;
}
const S = loadUmd(path.join(root, "module/webroot/engine_status.js"));
const jsSource = fs.readFileSync(path.join(root, "module/webroot/engine_status.js"), "utf8");
const cppSource = fs.readFileSync(path.join(root, "src/runtime/status_snapshot.cpp"), "utf8");

let checks = 0, failures = 0;
function check(cond, label) {
  checks++;
  if (!cond) { failures++; console.error("FAIL: " + label); }
}
const load = (name) => fs.readFileSync(path.join(dir, name), "utf8");
const raw = (name, nowDelta) => {
  const j = JSON.parse(load(name));
  return "NOW=" + (j.updated_epoch + (nowDelta == null ? 2 : nowDelta)) + "\n" + load(name);
};

// ---- the writer and the reader agree on every blocker code ----
const arr = /kBlockerCodes\[\]\s*=\s*\{([\s\S]*?)\};/.exec(cppSource);
check(!!arr, "kBlockerCodes array found in C++ source");
const cppCodes = arr ? [...arr[1].matchAll(/"([A-Z_]+)"/g)].map((m) => m[1]) : [];
check(cppCodes.length === 15, "C++ lists 15 blocker codes");
for (const c of cppCodes) check(Object.prototype.hasOwnProperty.call(S.BLOCKERS, c), "WebUI explains blocker " + c);
for (const c of Object.keys(S.BLOCKERS)) check(cppCodes.includes(c), "WebUI has no code the engine never emits: " + c);

// ---- the reader consumes every field the writer emits ----
function keys(o, out) {
  if (Array.isArray(o)) { o.forEach((x) => keys(x, out)); return out; }
  if (o && typeof o === "object") for (const k of Object.keys(o)) { out.add(k); keys(o[k], out); }
  return out;
}
const emitted = keys(JSON.parse(load("stressed_holding.json")), new Set());
for (const k of emitted) {
  if (["eyes", "brain", "hands"].includes(k)) continue;
  check(new RegExp("\\b" + k + "\\b").test(jsSource), "WebUI reads emitted field " + k);
}

// Recovery heartbeat must not refresh old telemetry or hide a later dead heartbeat.
{
  const j = JSON.parse(load("ready.json"));
  j.blocker = "RECOVERY_PENDING";
  j.telemetry_epoch = j.updated_epoch - 176;
  j.recovery_phase = "external_handoff";
  j.recovery_detail = "cpu: release_journal: /sys/test/scaling_governor";
  const q = S.parse("NOW=" + (j.updated_epoch + 1) + "\n" + JSON.stringify(j));
  check(q.ok && !q.stale, "recovery has a live heartbeat");
  check(S.pipeline(q.data).find(x => x.id === "authority").state === "Denied", "recovery cannot advertise an open authority gate");
  const summary = S.summary(q);
  check(summary.code === "RECOVERY_PENDING" && summary.tone === "warn", "recovery is explicitly blocked");
  check(summary.detail.includes("177s old") && summary.detail.includes(j.recovery_detail), "old telemetry and handoff failure are visible");
  const dead = S.parse("NOW=" + (j.updated_epoch + 500) + "\n" + JSON.stringify(j));
  check(S.summary(dead).code === "STALE", "recovery does not mask a stopped heartbeat");
  j.telemetry_epoch = 0;
  check(S.summary(S.parse("NOW=" + (j.updated_epoch + 1) + "\n" + JSON.stringify(j))).detail.includes("unavailable"), "no telemetry is not invented during startup recovery");
}

// A thermal plan is explicitly cooling work, not a stopped engine.
{
  const j = JSON.parse(load("ready.json"));
  j.blocker = "THERMAL_STABILIZING"; j.cohort_identity = "cohort:test"; j.cohort_members = 3;
  const q = S.parse("NOW=" + (j.updated_epoch + 1) + "\n" + JSON.stringify(j));
  check(S.summary(q).code === "THERMAL_STABILIZING", "thermal stabilization has its own status");
  check(S.summary(q).detail.includes("3 targets"), "coupled plan size is visible");
  check(S.brainRows(q.data).some(row => row[0] === "Last plan" && row[1] === "3 target(s)"), "last coordinated plan is visible in diagnostics");
}

// ---- ready / adaptive ----
let p = S.parse(raw("ready.json"));
check(p.ok && !p.stale, "ready parses and is fresh");
let sum = S.summary(p);
check(sum.code === "READY" && sum.tone === "ok", "ready summary");
let pipe = S.pipeline(p.data);
check(pipe.length === 4, "four pipeline stages");
check(pipe.every((st) => st.tone === "ok" || st.tone === "info"), "ready pipeline has no blocked stage");
check(S.eyesRows(p.data).some((r) => r[0] === "Temperature" && /^41\.5 °C/.test(r[1])), "temperature row");
check(S.eyesRows(p.data).some((r) => r[0] === "Temperature" && /hottest 43\.0/.test(r[1])), "hottest sensor shown when it differs");
{
  const j = JSON.parse(load("ready.json"));
  j.eyes.thermal_available = false;
  j.eyes.thermal_c = 99; // a stale value must not be shown when there is no sensor
  const q = S.parse("NOW=" + (j.updated_epoch + 1) + "\n" + JSON.stringify(j));
  check(S.eyesRows(q.data).some((r) => r[0] === "Temperature" && r[1] === "n/a"), "no thermal sensor => n/a, not a stale number");
}
check(S.eyesRows(p.data).some((r) => r[0] === "Memory free" && r[1] === "52%"), "memory row");
check(S.brainRows(p.data).some((r) => r[0] === "Operation" && r[1] === "active"), "automatic operation row");
check(S.deviceRows(p.data).some((r) => r[0] === "Manufacturer / model" && /ExampleDevice/.test(r[1])), "live device identity row");
check(S.deviceRows(p.data).some((r) => r[0] === "Capabilities" && /38 discovered/.test(r[1]) && /7 policy-ready/.test(r[1])), "discovery vs policy-ready capabilities remain distinct");
check(S.deviceRows(p.data).some((r) => r[0] === "Block I/O mutation" && r[1] === "capability-driven"), "ready status reports capability-driven block queue policy");

// ---- observe mode explains itself ----
p = S.parse(raw("observe.json"));
sum = S.summary(p);
check(sum.code === "OBSERVE_MODE" && sum.tone === "info", "observe summary");
pipe = S.pipeline(p.data);
check(pipe.find((x) => x.id === "authority").tone === "bad", "authority denied in observe");
check(/configuration/.test(pipe.find((x) => x.id === "authority").note), "authority explains why");

// ---- stressed + holding a stabilizing change ----
p = S.parse(raw("stressed_holding.json"));
sum = S.summary(p);
check(sum.code === "THERMAL_STABILIZING" && sum.tone === "warn", "verified cooling stays active during thermal guard");
pipe = S.pipeline(p.data);
check(pipe.find((x) => x.id === "context").state === "Stabilize only", "context shows stabilize-only");
check(pipe.find((x) => x.id === "hands").state === "Holding", "hands holding");
const hands = S.handsRows(p.data);
check(hands.some((r) => r[0] === "Resource" && r[1] === "53" && r[2] === "/proc/sys/vm/swappiness"), "hands lists the applied resource");
check(hands.some((r) => /3 \/ 1 \/ 0/.test(r[1])), "hands counters");

// ---- hostile content stays inert text and NaN is n/a, never 0 ----
p = S.parse(raw("hostile.json"));
check(p.ok, "hostile status parses");
check(S.eyesRows(p.data).some((r) => r[0] === "Temperature" && r[1] === "n/a"), "NaN temperature shows n/a");
check(p.data.brain.planReason.includes("<img"), "hostile text preserved as data (rendered via textContent only)");
check(S.handsRows(p.data).some((r) => r[1] === "<b>1</b>"), "hostile value kept as plain string");
const appSrc = fs.readFileSync(path.join(root, "module/webroot/app.js"), "utf8");
check(appSrc.length > 1000, "Compiled React WebUI bundle exists in module/webroot/app.js");
const walkTs = (d) => fs.readdirSync(d, { withFileTypes: true }).flatMap((e) => e.isDirectory() ? walkTs(path.join(d, e.name)) : /\.(ts|tsx)$/.test(e.name) ? [fs.readFileSync(path.join(d, e.name), "utf8")] : []);
const webuiTsSrc = fs.existsSync(path.join(root, "webui")) ? walkTs(path.join(root, "webui")).join("\n") : "";
check(!/dangerouslySetInnerHTML|\.innerHTML\s*=|insertAdjacentHTML|outerHTML|document\.write|\beval\(/.test(webuiTsSrc), "WebUI source never uses HTML injection or eval");

// ---- stopped / stale / idle ----
p = S.parse(raw("stopped.json"));
check(S.summary(p).code === "STOPPED", "stopped summary");
p = S.parse(raw("ready.json", 200));
check(p.ok && p.stale, "old update is stale");
check(S.summary(p).code === "STALE" && S.summary(p).tone === "bad", "stale summary");
p = S.parse(raw("idle.json", 40));
check(!p.stale, "an idle engine sleeping longer is not stale at 40s");
p = S.parse(raw("idle.json", 400));
check(p.stale, "an idle engine is stale eventually");
p = S.parse(raw("idle.json"));
check(S.summary(p).code === "CONTEXT_IDLE", "idle summary");
p = S.parse(raw("ready.json", -50));
check(p.ok && p.age === 0 && !p.stale, "clock skew never yields a negative age");

// ---- bad inputs fail closed with a reason ----
for (const [label, input] of [
  ["missing file", "NOW=1760000000\n"],
  ["no clock", load("ready.json")],
  ["bad json", "NOW=1760000000\n{not json"],
  ["array root", "NOW=1760000000\n[1,2]"],
  ["wrong schema", "NOW=1760000000\n" + JSON.stringify({ schema: 99 })],
  ["oversize", "NOW=1760000000\n" + JSON.stringify({ schema: 1, pad: "x".repeat(70000) })],
  ["empty", ""],
  ["null", null],
]) {
  const r = S.parse(input);
  check(r.ok === false && typeof r.error === "string" && r.error.length > 0, "rejects: " + label);
  check(S.summary(r).tone === "bad", "summary of rejected input is an error: " + label);
}
// wrong-typed fields degrade to safe defaults instead of throwing
p = S.parse("NOW=1760000000\n" + JSON.stringify({ schema: 1, updated_epoch: 1760000000, eyes: "x", brain: [], hands: 5, running: true, blocker: "READY" }));
check(p.ok, "garbage sub-objects do not throw");
p = S.parse("NOW=1760000000\n" + JSON.stringify({ schema: 1, updated_epoch: 1760000000, blocker: "TOTALLY_NEW", eyes: {}, brain: {}, hands: {} }));
check(S.summary(p).code === "TOTALLY_NEW" && S.summary(p).tone === "info", "unknown blocker code is reported, not hidden");



// Integrated runtime diagnostics are additive and remain optional for older daemons.
{
  const j = JSON.parse(load("ready.json"));
  j.intelligence = { pressure: "IO_STALL", confidence: 0.8,
    control_conflict: true, external_control_paths: 1, rendering_available: true, frame_jank_rate: 0.1,
    storage_available: true, storage_caution: true, ufs_life_a: 10,
    ufs_life_b: 4, ufs_pre_eol: 2, learned_candidates: 7,
    power_hal_backend: "vendor_profiles_and_continuity", adpf_backend: "application_integration_required" };
  const q = S.parse("NOW=" + (j.updated_epoch + 1) + "\n" + JSON.stringify(j));
  check(q.ok && q.data.intelligence.controlConflict && q.data.intelligence.pressure === "IO_STALL" &&
      q.data.intelligence.storageCaution && q.data.intelligence.learnedCandidates === 7 &&
      S.brainRows(q.data).some((r) => r[0] === "Control continuity" && /controlled externally/.test(r[1])),
      "integrated runtime evidence is parsed and displayed");
}

// Real writer fixtures distinguish no measured demand from an inadmissible plan.
{
  const stable = S.parse(raw("selection_stable.json"));
  const blocked = S.parse(raw("selection_blocked.json"));
  check(stable.ok && stable.data.blocker === "NO_CANDIDATES", "current KEEP_CURRENT replaces READY");
  check(S.summary(stable).title === "No measured tuning need", "stable does not claim proven optimality");
  check(blocked.ok && S.summary(blocked).title === "Tuning need detected; no admissible plan", "pressure is not reported as no need");
  check(blocked.data.selection.granted === 20 && blocked.data.selection.relevant === 8 &&
    blocked.data.selection.selectable === 0, "candidate funnel survives native JSON roundtrip");
  check(S.brainRows(blocked.data).some(r => r[0] === "Last outcome" && /regression_outcome/.test(r[1])), "measured rejection is visible separately from predictions");
  const j=JSON.parse(load("selection_stable.json"));
  j.selection.current=false;
  const previous=S.parse("NOW="+(j.updated_epoch+1)+"\n"+JSON.stringify(j));
  check(S.summary(previous).title !== "No measured tuning need", "historical selection cannot override current blocker");
  delete j.selection; delete j.outcome_reason;
  const legacy=S.parse("NOW="+(j.updated_epoch+1)+"\n"+JSON.stringify(j));
  check(legacy.ok && !legacy.data.selection.assessed && legacy.data.outcomeReason === "", "older snapshots have safe empty selection defaults");
  j.selection={current: true, cpu_utility: "NaN", resource_utility: {}, discovered: "oops", reason: "<script>x</script>"};
  const malformed=S.parse("NOW="+(j.updated_epoch+1)+"\n"+JSON.stringify(j));
  check(malformed.ok && malformed.data.selection.cpuUtility === null && malformed.data.selection.resourceUtility === null && malformed.data.selection.discovered === 0, "malformed selection numbers are unavailable");
}

{
  const parsed=S.parse(raw("cpu_outcome.json")), d=parsed.data;
  check(parsed.ok && d.cpuPolicies.length === 1, "live CPU policies exist even without an owned CPU write");
  check(d.cpuPolicies[0].currentGovernor === "walt" && d.cpuPolicies[0].availableGovernors.includes("schedutil"), "live and advertised governors roundtrip");
  check(d.cpuPolicies[0].currentKhz === 800000 && d.cpuPolicies[0].maxKhz === 2400000, "frequency telemetry remains read-only data");
  check(d.selection.cpuReason === "no_higher_scoring_governor_than_current", "CPU selection has an explicit no-write reason");
  check(d.lastEvaluation.reason === "weighted_metrics_regressed" && d.lastEvaluation.metrics.psi_fraction.before === 0 && d.lastEvaluation.metrics.psi_fraction.after === .3, "actual native evaluation preserves zero-to-stall metrics");
  check(d.lastEvaluation.metrics.io_latency_ms.afterSamples === 4 && d.lastEvaluation.metrics.io_latency_ms.score < .5, "metric coverage and scores survive JSON roundtrip");
  const traceParser=loadUmd(path.join(root,"module/webroot/control_logic.js"));
  const evaluation=JSON.parse(load("cpu_outcome.json")).last_evaluation;
  const rows=traceParser.parseTrace(JSON.stringify({event:"outcome_resolved",result:"ROLLED_BACK",session_id:"live-test",cycle:42,event_seq:17,decision_reason:"MUTATION_ROLLED_BACK",evaluation}));
  check(rows[0].evaluation.metrics.io_latency_ms.after === 50, "trace export retains the measured evaluation instead of dropping it");
  check(rows[0].sessionId === "live-test" && rows[0].cycle === 42 && rows[0].eventSeq === 17, "trace export preserves session and sample correlation");
  check(rows[0].decisionReason === "MUTATION_ROLLED_BACK", "policy decision and actual mutation reason remain separate");
  check(JSON.parse(JSON.stringify(rows))[0].evaluation.reason === d.lastEvaluation.reason, "downloaded trace JSON retains the exact outcome cause");
  const j=JSON.parse(load("cpu_outcome.json")); j.brain.observing=true; j.brain.decision="NO_ACTION";
  let q=S.parse("NOW="+(j.updated_epoch+1)+"\n"+JSON.stringify(j));
  check(S.activity(q.data) === "MEASURING", "real measurement overrides the policy NO_ACTION badge");
  j.brain.hold_active=true;
  q=S.parse("NOW="+(j.updated_epoch+1)+"\n"+JSON.stringify(j));
  check(S.activity(q.data) === "HELD", "authority loss takes priority over activity");
  j.cpu_policies=[{path:"/test",available_governors:[null,{},"walt"],current_khz:"oops"}];
  q=S.parse("NOW="+(j.updated_epoch+1)+"\n"+JSON.stringify(j));
  check(q.data.cpuPolicies[0].availableGovernors.length===1 && q.data.cpuPolicies[0].currentKhz===null, "hostile CPU inventory cannot masquerade as live telemetry");
  for (const k of keys(JSON.parse(load("cpu_outcome.json")),new Set())) {
    check(new RegExp("\\b"+k+"\\b").test(jsSource),"WebUI consumes new native diagnostic field "+k);
  }
}

console.log("CoreFlow WebUI status contract: " + (checks - failures) + "/" + checks + " checks passed");
process.exit(failures ? 1 : 0);
