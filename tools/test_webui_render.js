#!/usr/bin/env node
"use strict";
// Smoke test: runs the real app.js against a fake DOM and a fake KernelSU bridge that
// returns real C++-written status files. Proves the Live view polls, parses and renders
// end to end. Usage: node tools/test_webui_render.js <fixture-dir>
const fs = require("fs");
const path = require("path");
const vm = require("vm");

const root = path.resolve(__dirname, "..");
const dir = process.argv[2];
if (!dir) { console.error("usage: test_webui_render.js <fixture-dir>"); process.exit(2); }
const web = path.join(root, "module/webroot");

class El {
  constructor(tag, id) {
    this.tag = tag; this.id = id || ""; this.children = []; this._text = ""; this.className = "";
    this.dataset = {}; this.style = {}; this.value = ""; this.checked = false; this.disabled = false; this.handlers = {};
    const self = this;
    this.classList = {
      add: (c) => { if (!self.className.split(" ").includes(c)) self.className += " " + c; },
      remove: (c) => { self.className = self.className.split(" ").filter((x) => x !== c).join(" "); },
      toggle: () => {}, contains: (c) => self.className.split(" ").includes(c),
    };
  }
  set textContent(v) { this._text = String(v); this.children = []; }
  get textContent() { return this._text + this.children.map((c) => c.textContent).join(""); }
  appendChild(c) { this.children.push(c); return c; }
  append(...cs) { cs.forEach((c) => this.children.push(typeof c === "string" ? Object.assign(new El("#text"), { _text: c }) : c)); }
  replaceChildren(...cs) { this.children = []; this._text = ""; this.append(...cs); }
  addEventListener(type, fn) { (this.handlers[type] = this.handlers[type] || []).push(fn); }
  dispatch(type) { (this.handlers[type] || []).forEach((fn) => fn({ target: this })); }
  removeEventListener() {} remove() {} setAttribute() {} querySelector() { return null; }
  querySelectorAll() { return []; } showModal() {} close() {} select() {}
}
const registry = {};
const getEl = (id) => (registry[id] = registry[id] || new El("div", id));

let currentStatusFile = "ready.json";
let statusReads = 0;
let traceReads = 0;
const nowOf = (f) => JSON.parse(fs.readFileSync(path.join(dir, f), "utf8")).updated_epoch + 2;

const ksu = {
  exec(cmd, cb) {
    let out = "", code = 0, err = "";
    if (cmd.includes("status.json")) {
      statusReads++;
      if (currentStatusFile === "__bridge_fail__") { code = 1; err = "su: permission denied"; }
      else if (currentStatusFile === "__missing__") { out = "NOW=1760000002\n"; code = 1; }
      else out = "NOW=" + nowOf(currentStatusFile) + "\n" + fs.readFileSync(path.join(dir, currentStatusFile), "utf8");
    } else if (cmd.includes("decision_trace.jsonl")) {
      traceReads++;
      const tracePath = path.join(dir, "decision_trace.jsonl");
      out = fs.existsSync(tracePath) ? fs.readFileSync(tracePath, "utf8") : "";
    }
    setImmediate(() => window[cb](code, out, err));
  },
};

const liveNav = new El("button", "liveNav"); liveNav.dataset.view = "live"; liveNav.className = "nav-item";
const traceNav = new El("button", "traceNav"); traceNav.dataset.view = "trace"; traceNav.className = "nav-item";
getEl("view-live"); getEl("view-trace");
const document = {
  readyState: "complete", visibilityState: "visible",
  getElementById: getEl, createElement: (t) => new El(t),
  createTextNode: (value) => Object.assign(new El("#text"), { _text: String(value) }),
  querySelectorAll: (selector) => {
    if (selector === "[data-view]" || selector === ".nav-item") return [liveNav, traceNav];
    if (selector === ".view") return [getEl("view-live"), getEl("view-trace")];
    return [];
  }, querySelector: () => null,
  handlers: {}, addEventListener(type, fn) { (this.handlers[type] = this.handlers[type] || []).push(fn); },
  body: new El("body"),
};
const window = { document, ksu, CoreFlowStatus: undefined };
const sandbox = { window, document, ksu, navigator: {}, atob, btoa, console, setTimeout, clearTimeout, Promise, Date, Math, JSON };
sandbox.globalThis = sandbox;
vm.createContext(sandbox);
vm.runInContext(fs.readFileSync(path.join(web, "engine_status.js"), "utf8").replace("typeof window !== \"undefined\" ? window : this", "window"), sandbox);
vm.runInContext(fs.readFileSync(path.join(web, "app.js"), "utf8"), sandbox);

let checks = 0, failures = 0;
const check = (c, l) => { checks++; if (!c) { failures++; console.error("FAIL: " + l); } };
const wait = (ms) => new Promise((r) => setTimeout(r, ms));
const text = (id) => getEl(id).textContent;

(async () => {
  await wait(250);
  check(statusReads >= 1, "Live view polls status.json on load");
  check(text("liveTitle") === "Ready to act", "banner shows the engine's own verdict: " + text("liveTitle"));
  check(getEl("liveBanner").className.includes("tone-ok"), "banner tone ok");
  check(text("liveVersion") === "v2.1.0" && text("liveSample") === "#120", "meta row filled");
  check(getEl("livePipeline").children.length === 4, "pipeline rendered 4 stages");
  check(getEl("liveEyes").children.length >= 6, "eyes rows rendered");
  check(getEl("liveDevice").children.length >= 6, "device & ecosystem rows rendered");
  check(getEl("liveDevice").textContent.includes("QUARANTINED"), "block queue safety policy is visible");
  check(getEl("liveBrain").children.length >= 6, "brain rows rendered");
  check(getEl("liveHands").children.length >= 2, "hands rows rendered");
  check(getEl("liveEyes").textContent.includes("41.5"), "temperature visible in eyes");

  getEl("traceSearch").dispatch("input");
  check(text("traceSummary").includes("structured record(s) shown"), "WebUI parses the C++ event journal");
  check(text("traceList").includes("scope_gate_checked"), "trace UI renders a real scope-gate event");
  check(text("traceList").includes("cpu_governor_opt_in_disabled"), "trace UI exposes the causal gate reason");
  check(text("traceList").includes("coarse scope denied"), "trace UI distinguishes a denied coarse gate from a target permit");
  check(text("traceList").includes("/sys/test/tunable"), "trace UI displays the concrete actuator target");
  check(text("traceList").includes("writes attempted/verified 1/1"), "trace UI displays actuator write/readback counts");
  check(text("traceList").includes("target-bound permit valid"), "trace UI labels target-bound permit separately from coarse gate");

  traceNav.dispatch("click");
  await wait(250);
  const initialTraceReads = traceReads;
  fs.appendFileSync(path.join(dir, "decision_trace.jsonl"), "\n" + JSON.stringify({
    schema: "coreflow.event.v2", session_id: "host-test-1", event_seq: 3,
    cycle: 8, event: "trace_live_poll_marker", scope: "engine", result: "INFO",
    reason: "new event appended while the trace page is open"
  }) + "\n");
  await wait(2850);
  check(traceReads > initialTraceReads, "active trace view polls the event journal without manual refresh");
  check(text("traceList").includes("trace_live_poll_marker"), "new daemon event appears in the open timeline");
  liveNav.dispatch("click");
  await wait(250);

  currentStatusFile = "hostile.json";
  await wait(3300);
  const hands = getEl("liveHands").textContent;
  check(hands.includes("<b>1</b>"), "hostile value shown literally as text");
  check(getEl("liveBrain").children.length >= 6, "hostile status still renders");
  check(text("liveEyes").includes("n/a"), "NaN renders as n/a");

  currentStatusFile = "__missing__";
  await wait(3300);
  check(text("liveTitle") === "No live data", "missing status.json explained: " + text("liveTitle"));
  check(/not found/i.test(text("liveDetail")), "missing status.json reason shown");
  check(getEl("livePipeline").children.length === 0, "no stale pipeline left behind");

  currentStatusFile = "__bridge_fail__";
  await wait(3300);
  check(text("liveTitle") === "No live data", "bridge failure is shown, not silent: " + text("liveTitle"));
  check(/permission denied/.test(text("liveDetail")), "bridge error text reaches the user");

  currentStatusFile = "observe.json";
  await wait(3300);
  check(text("liveTitle") === "Observe mode", "observe verdict: " + text("liveTitle"));

  // A hidden page must not keep polling the root shell (battery, bridge load).
  document.visibilityState = "hidden";
  const before = statusReads;
  currentStatusFile = "ready.json";
  await wait(3600);
  check(statusReads === before, "no polling while the page is hidden");
  document.visibilityState = "visible";
  (document.handlers.visibilitychange || []).forEach((fn) => fn());
  await wait(400);
  check(statusReads > before, "polling resumes when the page is visible again");
  check(text("liveTitle") === "Ready to act", "view recovers after being hidden");

  console.log("CoreFlow WebUI render smoke: " + (checks - failures) + "/" + checks + " checks passed");
  process.exit(failures ? 1 : 0);
})().catch((e) => { console.error("FAIL: render threw: " + (e && e.stack || e)); process.exit(1); });
