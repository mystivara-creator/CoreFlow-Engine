#!/usr/bin/env node
"use strict";
// End-to-end smoke test for the CoreFlow Control WebUI. It loads the real index.html ids and
// attributes into a minimal DOM, runs the real app.js, and answers its KernelSU bridge calls with
// real C++-written status (ready.json from tests/status_dump.cpp) and a simulated device.
// Usage: node tools/test_webui_render.js <fixture-dir>
const fs = require("fs");
const path = require("path");
const vm = require("vm");

const root = path.resolve(__dirname, "..");
const dir = process.argv[2];
if (!dir) {
  console.error("usage: test_webui_render.js <fixture-dir>");
  process.exit(2);
}
const web = path.join(root, "module/webroot");

let checks = 0;
let failures = 0;
function check(cond, label) {
  checks++;
  if (!cond) {
    failures++;
    console.error("FAIL: " + label);
  }
}

// ---- minimal DOM built from the real index.html ----
const missingIds = new Set();
const registry = {};
const docNodes = [];
const createdNodes = [];

class El {
  constructor(tag, inDocument) {
    this.tag = String(tag).toLowerCase();
    this.attrs = {};
    this.children = [];
    this._text = "";
    this.value = "";
    this.style = {};
    this.scrollTop = 0;
    this.handlers = {};
    this.open = false;
    this.returnValue = "";
    (inDocument ? docNodes : createdNodes).push(this);
  }
  getAttribute(n) { return Object.prototype.hasOwnProperty.call(this.attrs, n) ? this.attrs[n] : null; }
  setAttribute(n, v) { this.attrs[n] = String(v); }
  hasAttribute(n) { return Object.prototype.hasOwnProperty.call(this.attrs, n); }
  toggleAttribute(n, force) {
    const on = force === undefined ? !this.hasAttribute(n) : !!force;
    if (on) this.attrs[n] = ""; else delete this.attrs[n];
    return on;
  }
  get className() { return this.attrs.class || ""; }
  set className(v) { this.attrs.class = String(v); }
  get hidden() { return this.hasAttribute("hidden"); }
  set hidden(v) { this.toggleAttribute("hidden", v); }
  get selected() { return this.hasAttribute("selected"); }
  set selected(v) { this.toggleAttribute("selected", v); }
  get disabled() { return this.hasAttribute("disabled"); }
  set disabled(v) { this.toggleAttribute("disabled", v); }
  get firstChild() { return this.children[0] || null; }
  get textContent() {
    return this._text + this.children.map((c) => (c instanceof El ? c.textContent : c.text)).join("");
  }
  set textContent(v) { this._text = String(v); this.children = []; }
  appendChild(c) { this.children.push(c); return c; }
  append(...nodes) { nodes.forEach((n) => this.children.push(n instanceof El ? n : { text: String(n) })); }
  removeChild(c) { this.children = this.children.filter((x) => x !== c); return c; }
  remove() {}
  addEventListener(type, fn) { (this.handlers[type] = this.handlers[type] || []).push(fn); }
  removeEventListener(type, fn) { this.handlers[type] = (this.handlers[type] || []).filter((f) => f !== fn); }
  dispatch(type) { (this.handlers[type] || []).slice().forEach((fn) => fn({ target: this })); }
  click() { this.dispatch("click"); }
  showModal() { this.open = true; }
  close(value) {
    this.open = false;
    this.returnValue = value === undefined ? "" : String(value);
    this.dispatch("close");
  }
  select() {}
}

function selectorTest(sel) {
  let m = /^([a-zA-Z][\w-]*)?\.([\w-]+)$/.exec(sel);
  if (m) return (el) => (!m[1] || el.tag === m[1]) && el.className.split(/\s+/).includes(m[2]);
  m = /^\[([\w-]+)(?:="([^"]*)")?\]$/.exec(sel);
  if (m) return (el) => el.hasAttribute(m[1]) && (m[2] === undefined || el.getAttribute(m[1]) === m[2]);
  throw new Error("test DOM does not support selector " + sel);
}

function loadDom(html) {
  const stripped = html
    .replace(/<!--[\s\S]*?-->/g, "")
    .replace(/<script[\s\S]*?<\/script>/g, "")
    .replace(/<style[\s\S]*?<\/style>/g, "");
  const tagRe = /<([a-zA-Z][a-zA-Z0-9-]*)((?:\s+[^\s=>\/]+(?:="[^"]*")?)*)\s*\/?>/g;
  const attrRe = /([^\s=]+)(?:="([^"]*)")?/g;
  let m;
  while ((m = tagRe.exec(stripped))) {
    const node = new El(m[1], true);
    let a;
    attrRe.lastIndex = 0;
    while ((a = attrRe.exec(m[2] || ""))) node.setAttribute(a[1], a[2] === undefined ? "" : a[2]);
    if (node.getAttribute("id")) registry[node.getAttribute("id")] = node;
  }
}
const indexHtml = fs.readFileSync(path.join(web, "index.html"), "utf8");
loadDom(indexHtml);

// ---- simulated device and KernelSU bridge ----
const readyJson = fs.readFileSync(path.join(dir, "ready.json"), "utf8");
const readyObj = JSON.parse(readyJson);
const device = {
  pids: "4242", supervisor: "4200", supAlive: "1", safe: "0", disable: "0", pending: "0", boot: "0",
  config: [
    "# CoreFlow config (test device)",
    "mutation_mode=observe",
    "mutation_armed=false",
    "monitor_interval=5",
    "min_confidence=0.6",
    "allow_cpu_governor=yes",
    "grant_memory=true",
    "boost_window_ms=3000",
    ""
  ].join("\n"),
  trace: [
    JSON.stringify({ event: "actuator_apply_completed", result: "OK", reason: "verified", timestamp: "t1", state: "Normal", decision: "Apply", safety_hold: "NONE" }),
    JSON.stringify({ event: "scope_gate_checked", result: "DENY", reason: "cpu_governor_opt_in_disabled", timestamp: "t2", safety_hold: "NONE" }),
    JSON.stringify({ event: "thermal_guard", result: "HOLD", reason: "hot", timestamp: "t3", safety_hold: "THERMAL" })
  ].join("\n"),
  log: "2026-10-19 10:00:00 CoreFlowAutonomous started\n2026-10-19 10:00:03 WARN thermal headroom low\n"
};
const commands = [];

function reply(cb, errno, stdout, stderr) {
  setImmediate(() => window[cb](errno, stdout, stderr || ""));
}

function bridgeExec(cmd, cb) {
  commands.push(cmd);
  if (cmd.includes("@@PROBE")) {
    const now = readyObj.updated_epoch + 2;
    const probe = [
      "@@PROBE",
      "PIDS=" + device.pids,
      "SUP=" + device.supervisor,
      "SUP_ALIVE=" + device.supAlive,
      "SAFE=" + device.safe,
      "DISABLE=" + device.disable,
      "PENDING=" + device.pending,
      "BOOT=" + device.boot,
      "@@STATUS",
      "NOW=" + now,
      readyJson,
      "@@END"
    ].join("\n");
    return reply(cb, 0, probe + "\n");
  }
  if (cmd.includes("CONFIG_COMMIT_OK")) {
    const m = /printf %s '([\s\S]*?)' > "\$TMP"/.exec(cmd);
    if (m) device.config = m[1].split("'\\''").join("'");
    if (cmd.includes("rm -f \"$DIR/DISABLE\"")) device.disable = "0";
    if (cmd.includes("touch \"$DIR/DISABLE\"")) device.disable = "1";
    device.pending = "1";
    return reply(cb, 0, "CONFIG_COMMIT_OK\n");
  }
  if (cmd.startsWith("cat ") && cmd.includes("config.ini")) return reply(cb, 0, device.config);
  if (cmd.includes("decision_trace.jsonl")) return reply(cb, 0, device.trace + "\n");
  if (cmd.includes("coreflowd.log")) return reply(cb, 0, device.log);
  if (cmd.includes("webui_restart_request")) return reply(cb, 0, "RESTART_OK\n");
  if (cmd.includes("nohup")) return reply(cb, 0, "START_REQUESTED\n");
  if (cmd.includes("kill -USR1")) return reply(cb, 0, "REFRESH_SIGNAL_SENT\n");
  if (cmd.includes("kill -TERM")) return reply(cb, 0, "STOP_OK\n");
  return reply(cb, 1, "", "unexpected command in test");
}

const ksu = { exec: bridgeExec };
const document = {
  readyState: "complete",
  visibilityState: "visible",
  getElementById(id) {
    if (registry[id]) return registry[id];
    missingIds.add(id);
    return new El("div", false);
  },
  createElement: (tag) => new El(tag, false),
  querySelectorAll(sel) {
    const test = selectorTest(sel);
    return docNodes.filter(test);
  },
  addEventListener() {},
  body: new El("body", false),
  execCommand: () => true
};

const window = { document, customElements: undefined };
const sandbox = {
  window, document, ksu, navigator: {}, console, setTimeout, clearTimeout, setImmediate,
  Promise, Date, Math, JSON, String, Number, Object, Array, RegExp, Error
};
sandbox.globalThis = sandbox;
vm.createContext(sandbox);
["engine_status.js", "control_logic.js", "app.js"].forEach((f) => {
  vm.runInContext(fs.readFileSync(path.join(web, f), "utf8"), sandbox, { filename: f });
});

const tick = (ms) => new Promise((r) => setTimeout(r, ms || 15));
const S = window.CoreFlowStatus;
const txt = (id) => registry[id].textContent;
const navTo = (page) => document.querySelectorAll("[data-page-link=\"" + page + "\"]")[0].click();

(async () => {
  // The vendor notice is checked 2.5 s after start-up, so wait for that check.
  await tick(2600);

  // Engine: the hero is the daemon's own state, rendered from the C++ status file.
  const expected = S.summary(S.parse("NOW=" + (readyObj.updated_epoch + 2) + "\n" + readyJson));
  check(txt("heroTitle") === expected.title, "hero title is the engine summary: " + txt("heroTitle"));
  check(txt("heroCode") === (expected.code || "—"), "hero code matches the summary code");
  check(registry.headerChip.textContent === "ONLINE", "header shows ONLINE when the probe finds pids");
  check(txt("metaPid") === "4242", "pid comes from the live probe");
  check(registry.pipeline.children.length === 4, "pipeline renders the four decision stages");
  check(registry.eyesRows.children.length > 0 && registry.handsRows.children.length > 0, "eyes and hands render rows");
  check(registry.safetyRows.children.length === 4, "safety markers render from the probe");
  check(registry.btnStart.disabled === true && registry.btnStop.disabled === false, "online: Start off, Stop on");
  check(registry.bridgeNotice.hidden === true, "bridge notice hidden while the bridge answers");
  check(!/material\/all\.js|<importmap|<md-(button|switch|text-field)/i.test(indexHtml),
    "WebUI has no vendored component runtime or custom-element dependency");
  check(indexHtml.includes('href="https://www.perplexity.ai/"'), "Project & AI includes Perplexity");
  check(indexHtml.includes('data-page-link="about"') && indexHtml.includes('>Project &amp; AI</span>'),
    "Project & AI remains reachable in the bottom navigation");

  // Re-discover sends SIGUSR1 and writes no tunable.
  registry.btnRefresh.click();
  await tick();
  check(commands.some((c) => c.includes("kill -USR1")), "Re-discover sends SIGUSR1");

  // Stop asks first. Cancelling sends no signal.
  const dialog = registry.confirmDialog;
  registry.btnStop.click();
  await tick();
  check(dialog.open === true && txt("dialogTitle") === "Stop coreflowd?", "Stop opens its confirmation");
  const stopsBefore = commands.filter((c) => c.includes("kill -TERM")).length;
  registry.dialogCancel.click();
  await tick();
  check(commands.filter((c) => c.includes("kill -TERM")).length === stopsBefore, "cancelled Stop sends no signal");

  // Confirmed Restart uses the supervisor's restart marker.
  registry.btnRestart.click();
  await tick();
  registry.dialogConfirm.click();
  await tick(40);
  check(commands.some((c) => c.includes("webui_restart_request")), "confirmed Restart writes the supervisor restart marker");

  // Policy: values come from config.ini, a valid save writes it, legacy lines survive.
  navTo("policy");
  await tick(40);
  check(registry.runtimeFields.children.length === 2, "policy builds the runtime numeric fields");
  const boostWrap = registry.interactivityFields.children.find((w) =>
    (w.children[0].textContent || "").startsWith("Boost window"));
  check(boostWrap && boostWrap.children[1].value === "3000", "policy reads boost_window_ms from config.ini");
  boostWrap.children[1].value = "4000";
  registry.btnSavePolicy.click();
  await tick(60);
  check(device.config.indexOf("boost_window_ms=4000") >= 0, "the saved value reaches config.ini");
  check(device.config.indexOf("allow_cpu_governor=yes") >= 0 && device.config.indexOf("grant_memory=true") >= 0,
    "legacy grant lines survive a save");
  check(registry.pendingNotice.hidden === false, "pending-restart banner appears after a save");

  // Adaptive is refused while SAFE_MODE is present: the probe must report it first.
  device.safe = "1";
  navTo("engine");
  await tick(40);
  navTo("policy");
  await tick(40);
  const writesBefore = commands.filter((c) => c.includes("CONFIG_COMMIT_OK")).length;
  document.querySelectorAll("[data-mode=\"adaptive\"]")[0].click();
  registry.btnSavePolicy.click();
  await tick(40);
  check(commands.filter((c) => c.includes("CONFIG_COMMIT_OK")).length === writesBefore,
    "SAFE_MODE blocks an Adaptive save before any write");
  check(dialog.open === false, "a blocked save does not open a confirmation");
  device.safe = "0";

  // Journal: records are classified, the filter narrows them, the log filter matches.
  navTo("journal");
  await tick(40);
  check(registry.traceList.children.length === 3, "journal lists the three decision records");
  document.querySelectorAll("[data-jfilter=\"errors\"]")[0].click();
  await tick();
  check(registry.traceList.children.length === 2, "blocks & errors filter keeps the DENY and HOLD records");
  document.querySelectorAll("[data-jview=\"log\"]")[0].click();
  await tick();
  registry.logSearch.value = "thermal";
  registry.logSearch.dispatch("input");
  check(registry.logBox.textContent.indexOf("thermal") >= 0 && registry.logBox.textContent.indexOf("started") < 0,
    "log filter keeps only matching lines");

  // Missing bridge: the page reports it and does not pretend to act.
  sandbox.ksu = undefined;
  navTo("engine");
  await tick(40);
  check(registry.bridgeNotice.hidden === false, "a missing KernelSU bridge is reported");
  check(registry.btnStart.disabled === true && registry.btnStop.disabled === true, "no action is offered without the bridge");

  // Static invariants: the UI never builds markup from device data.
  const appSource = fs.readFileSync(path.join(web, "app.js"), "utf8");
  const logicSource = fs.readFileSync(path.join(web, "control_logic.js"), "utf8");
  check(!/innerHTML|insertAdjacentHTML|outerHTML|document\.write|\beval\(/.test(appSource + logicSource),
    "app.js and control_logic.js never use HTML injection");
  check(missingIds.size === 0, "every id used by app.js exists in index.html: " + Array.from(missingIds).join(", "));

  console.log("CoreFlow WebUI render contract: " + (checks - failures) + "/" + checks + " checks passed");
  process.exit(failures ? 1 : 0);
})().catch((e) => {
  console.error("FAIL: render test crashed: " + ((e && e.stack) || e));
  process.exit(1);
});
