#!/usr/bin/env node
"use strict";
// Cross-language contract test: real JSON written by the C++ engine code must be
// understood by the WebUI parser. Usage: node tools/test_webui_status.js <fixture-dir>
const fs = require("fs");
const path = require("path");

const root = path.resolve(__dirname, "..");
const dir = process.argv[2];
if (!dir) { console.error("usage: test_webui_status.js <fixture-dir>"); process.exit(2); }

const S = require(path.join(root, "module/webroot/engine_status.js"));
const jsSource = fs.readFileSync(path.join(root, "module/webroot/engine_status.js"), "utf8");
const cppSource = fs.readFileSync(path.join(root, "src/status_snapshot.cpp"), "utf8");

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
check(cppCodes.length === 13, "C++ lists 13 blocker codes");
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
check(S.brainRows(p.data).some((r) => r[0] === "Mode" && /adaptive/.test(r[1]) && /armed/.test(r[1])), "mode row");

// ---- observe mode explains itself ----
p = S.parse(raw("observe.json"));
sum = S.summary(p);
check(sum.code === "OBSERVE_MODE" && sum.tone === "info", "observe summary");
pipe = S.pipeline(p.data);
check(pipe.find((x) => x.id === "authority").tone === "bad", "authority denied in observe");
check(/observe/.test(pipe.find((x) => x.id === "authority").note), "authority explains why");

// ---- stressed + holding a stabilizing change ----
p = S.parse(raw("stressed_holding.json"));
sum = S.summary(p);
check(sum.code === "CHANGE_HELD" && sum.tone === "ok", "holding summary");
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
const liveBlock = appSrc.slice(appSrc.indexOf("Live engine view"), appSrc.indexOf("function init()"));
check(liveBlock.length > 500, "live block located in app.js");
check(!/innerHTML|insertAdjacentHTML|outerHTML|document\.write|eval\(/.test(liveBlock), "live rendering never uses HTML injection");

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

console.log("CoreFlow WebUI status contract: " + (checks - failures) + "/" + checks + " checks passed");
process.exit(failures ? 1 : 0);
