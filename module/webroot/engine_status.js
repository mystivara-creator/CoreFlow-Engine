/* CoreFlow live engine status: pure logic (no DOM). Consumed by app.js, tested with node.
 *
 * The C++ daemon is the only writer of status.json. This file only validates and
 * explains it. It never writes anywhere and carries no authority over the engine.
 */
(function (root) {
  "use strict";

  var MAX_BYTES = 65536;
  var SCHEMA = 1;

  // Must match blockerCodes() in src/status_snapshot.cpp (checked by tools/test_webui_status.js).
  var BLOCKERS = {
    STOPPED: { tone: "bad", title: "Engine stopped", detail: "The daemon is not running. It restores any change it made when it shuts down." },
    SAFETY_HOLD: { tone: "warn", title: "Safety hold active", detail: "The kill switch or SAFE_MODE is set, so mutation is disabled until it is cleared." },
    OBSERVE_MODE: { tone: "info", title: "Observe mode", detail: "The engine watches and reasons but is not allowed to change anything. Choose Adaptive in Mode & policy to let it act." },
    NOT_ARMED: { tone: "warn", title: "Adaptive is not armed", detail: "Adaptive is requested but mutation_armed is off, so no permit is issued." },
    BASELINE_CAPTURE: { tone: "info", title: "Measuring factory behaviour", detail: "The engine first records how the device behaves untouched, so it can tell later whether a change helped." },
    OBSERVING_OUTCOME: { tone: "info", title: "Measuring the effect of a change", detail: "A change was applied. The engine is checking whether it helped, and will roll it back if it did not." },
    CHANGE_HELD: { tone: "ok", title: "A verified change is in place", detail: "It is held for a short window, then restored so the next experiment starts from the original state." },
    COOLDOWN: { tone: "info", title: "Cooling down after the last outcome", detail: "A pause between changes keeps each measurement attributable to one change." },
    CONTEXT_IDLE: { tone: "info", title: "Device is idle", detail: "Nothing is worth optimizing right now." },
    CONTEXT_LOW_CONFIDENCE: { tone: "warn", title: "Telemetry confidence too low", detail: "Readings are not trustworthy enough to act on safely (needs 70% or more)." },
    CONTEXT_NO_HEADROOM: { tone: "warn", title: "No headroom to tune", detail: "Thermal, memory or battery headroom is missing, and no stress-reducing change applies." },
    NO_CANDIDATES: { tone: "info", title: "No resource qualifies", detail: "No discovered, writable resource is justified under the current plan." },
    READY: { tone: "ok", title: "Ready to act", detail: "The engine will apply a change as soon as its model finds one justified." }
  };

  function isObj(v) { return v !== null && typeof v === "object" && !Array.isArray(v); }
  function num(v) { return (typeof v === "number" && isFinite(v)) ? v : null; }
  function int(v, d) { return (typeof v === "number" && isFinite(v) && v >= 0) ? Math.floor(v) : d; }
  function str(v, max) { return typeof v === "string" ? v.slice(0, max || 160) : ""; }
  function bool(v) { return v === true; }

  function pairs(v, keyName) {
    var out = [];
    if (!Array.isArray(v)) return out;
    for (var i = 0; i < v.length && out.length < 16; i++) {
      var it = v[i];
      if (isObj(it)) out.push({ key: str(it[keyName], 200), value: str(it.value, 120) });
    }
    return out;
  }

  // Validate and normalize. Unknown fields are dropped; wrong types become safe defaults.
  function normalize(j) {
    var e = isObj(j.eyes) ? j.eyes : {};
    var b = isObj(j.brain) ? j.brain : {};
    var h = isObj(j.hands) ? j.hands : {};
    var v = isObj(j.device) ? j.device : {};
    return {
      version: str(j.version, 32),
      pid: int(j.pid, 0),
      sample: int(j.sample, 0),
      updated: int(j.updated_epoch, 0),
      interval: int(j.interval_s, 5),
      running: j.running !== false,
      blocker: str(j.blocker, 40),
      device: {
        present: isObj(j.device),
        manufacturer: str(v.manufacturer, 96), model: str(v.model, 96),
        deviceName: str(v.device_name, 96), product: str(v.product, 96),
        board: str(v.board, 96), hardware: str(v.hardware, 96),
        socManufacturer: str(v.soc_manufacturer, 96), socModel: str(v.soc_model, 96),
        androidRelease: str(v.android_release, 32), sdkLevel: str(v.sdk_level, 16),
        kernelRelease: str(v.kernel_release, 128), abi: str(v.abi, 32),
        procAvailable: bool(v.proc_available), sysAvailable: bool(v.sys_available),
        cgroupV2: bool(v.cgroup_v2), cpuset: bool(v.cpuset_available),
        uclamp: bool(v.uclamp_available), schedulerControls: bool(v.scheduler_controls_available),
        devfreq: bool(v.devfreq_available), discoveredResources: int(v.discovered_resources, 0),
        mutationReadyResources: int(v.mutation_ready_resources, 0),
        blockQueueMutationsQuarantined: bool(v.block_queue_mutations_quarantined)
      },
      eyes: {
        // The engine says whether it has a thermal sensor; without one the numbers mean nothing.
        thermal: e.thermal_available === false ? null : num(e.thermal_c),
        hottest: e.thermal_available === false ? null : num(e.hottest_c),
        thermalTrend: str(e.thermal_trend, 16),
        mem: num(e.mem_available_ratio), memTrend: str(e.memory_trend, 16),
        cpu: num(e.cpu_utilization), load: num(e.load1), loadTrend: str(e.load_trend, 16),
        ioRead: num(e.io_read_kbs), ioWrite: num(e.io_write_kbs),
        battery: num(e.battery_percent), charging: bool(e.charging), confidence: num(e.confidence)
      },
      brain: {
        state: str(b.state, 24), previous: str(b.previous_state, 24), decision: str(b.decision, 32),
        workload: str(b.workload, 32),
        thermalHeadroom: bool(b.thermal_headroom), memoryHeadroom: bool(b.memory_headroom), powerHeadroom: bool(b.power_headroom),
        proactive: bool(b.proactive_allowed), stabilizing: bool(b.stabilizing_allowed),
        planAction: str(b.plan_action, 32), intervention: str(b.intervention, 24),
        eligible: bool(b.mutation_eligible), stabilizingOnly: bool(b.stabilizing_only),
        candidates: int(b.candidates, 0), planReason: str(b.plan_reason, 200),
        agentMode: str(b.agent_mode, 24), agentReason: str(b.agent_reason, 240),
        holdActive: bool(b.hold_active), hold: str(b.safety_hold, 24),
        mode: str(b.mode, 16), armed: bool(b.armed), allowCpu: bool(b.allow_cpu_governor),
        permitResource: bool(b.permit_resource), permitCpu: bool(b.permit_cpu),
        baselineReady: bool(b.baseline_ready), baselineSamples: int(b.baseline_samples, 0), baselineTarget: int(b.baseline_target, 0),
        observationSamples: int(b.observation_samples, 0), observationTarget: int(b.observation_target, 0),
        skipReason: str(b.skip_reason, 200),
        observing: bool(b.observing), cooldown: int(b.cooldown_remaining, 0)
      },
      hands: {
        cpuMutated: bool(h.cpu_mutated), resourceMutated: bool(h.resource_mutated), stabilizingEpoch: bool(h.stabilizing_epoch),
        cpuApplied: pairs(h.cpu_applied, "key"), resourceApplied: pairs(h.resource_applied, "path"),
        lastCpu: str(h.last_cpu_result, 24), lastResource: str(h.last_resource_result, 24),
        lastKind: str(h.last_change_kind, 16), lastResult: str(h.last_change_result, 24), lastSample: int(h.last_change_sample, 0),
        verified: int(h.verified, 0), rolledBack: int(h.rolled_back, 0), failed: int(h.failed, 0)
      }
    };
  }

  // raw: stdout of `printf 'NOW=%s\n' "$(date +%s)"; cat status.json`
  function parse(raw) {
    var text = String(raw == null ? "" : raw);
    var nl = text.indexOf("\n");
    var head = nl < 0 ? text : text.slice(0, nl);
    var body = nl < 0 ? "" : text.slice(nl + 1);
    var m = /^NOW=(\d{9,12})$/.exec(head.trim());
    if (!m) return { ok: false, error: "no device clock in probe output" };
    var now = Number(m[1]);
    body = body.trim();
    if (!body) return { ok: false, error: "status.json not found. The daemon has not written it yet (older build, or not started)." };
    if (body.length > MAX_BYTES) return { ok: false, error: "status.json is too large; ignored" };
    var j;
    try { j = JSON.parse(body); } catch (e) { return { ok: false, error: "status.json is not valid JSON" }; }
    if (!isObj(j)) return { ok: false, error: "status.json has the wrong shape" };
    if (j.schema !== SCHEMA) return { ok: false, error: "unsupported status schema " + String(j.schema).slice(0, 12) };
    var d = normalize(j);
    var age = Math.max(0, now - d.updated);
    // Idle ticks sleep much longer than the nominal interval; allow for that.
    var limit = d.brain.state === "IDLE" ? Math.max(75, d.interval * 5) : Math.max(15, d.interval * 3);
    var stale = d.running && age > limit;
    return { ok: true, data: d, age: age, stale: stale, now: now };
  }

  // The one banner: what is the engine doing, and why not more.
  function summary(p) {
    if (!p.ok) return { tone: "bad", title: "No live data", detail: p.error, code: "" };
    var d = p.data;
    if (!d.running) return { tone: "bad", title: BLOCKERS.STOPPED.title, detail: BLOCKERS.STOPPED.detail, code: "STOPPED" };
    if (p.stale) {
      return { tone: "bad", title: "Engine not responding", detail: "No update for " + p.age + "s. The daemon may have stalled or been killed; check the log.", code: "STALE" };
    }
    var info = BLOCKERS[d.blocker];
    if (!info) return { tone: "info", title: "Unrecognized status", detail: "The engine reported a code this page does not know: " + d.blocker, code: d.blocker };
    var detail = info.detail;
    if (d.blocker === "BASELINE_CAPTURE" && d.brain.baselineTarget) detail += " (" + d.brain.baselineSamples + "/" + d.brain.baselineTarget + " samples)";
    if (d.blocker === "COOLDOWN") detail += " " + d.brain.cooldown + " sample(s) left.";
    if (d.blocker === "CONTEXT_LOW_CONFIDENCE" && d.eyes.confidence != null) detail += " Now " + Math.round(d.eyes.confidence * 100) + "%.";
    if (d.blocker === "OBSERVING_OUTCOME" && d.brain.observationTarget)
      detail += " Epoch " + d.brain.observationSamples + "/" + d.brain.observationTarget + ".";
    if (d.brain.skipReason) detail += " · " + d.brain.skipReason;
    return { tone: info.tone, title: info.title, detail: detail, code: d.blocker };
  }

  // The decision pipeline: where a change is stopped, stage by stage.
  function pipeline(d) {
    var b = d.brain, h = d.hands;
    var ctx;
    if (b.proactive) ctx = { tone: "ok", state: "Open", note: b.workload + " · full tuning allowed" };
    else if (b.stabilizing) ctx = { tone: "warn", state: "Stabilize only", note: "Under stress: small load-reducing changes only" };
    else ctx = { tone: "bad", state: "Closed", note: b.state === "IDLE" ? "Idle" : "No headroom or low confidence" };

    var pol;
    if (b.eligible && b.candidates > 0) {
      pol = { tone: b.stabilizingOnly ? "warn" : "ok", state: b.planAction, note: b.intervention + " intervention · " + b.candidates + " candidate(s)" };
    } else {
      pol = { tone: "bad", state: "Hold", note: b.planReason || "No eligible plan" };
    }

    var auth;
    if (b.permitResource || b.permitCpu) {
      auth = { tone: "ok", state: "Permit", note: "resource " + (b.permitResource ? "yes" : "no") + " · CPU governor " + (b.permitCpu ? "yes" : "no") };
    } else {
      var mode = String(b.mode || "").toLowerCase(); var why = b.holdActive ? "safety hold" : mode !== "adaptive" ? "observe mode" : !b.armed ? "not armed" : "plan not permitted";
      auth = { tone: "bad", state: "Denied", note: why };
    }

    var hands;
    if (h.cpuMutated || h.resourceMutated) hands = { tone: "ok", state: "Holding", note: (h.stabilizingEpoch ? "stabilizing change" : "verified change") + " in place" };
    else if (b.observing) hands = { tone: "info", state: "Measuring", note: "waiting for the outcome" };
    else hands = { tone: "info", state: "Idle", note: "nothing changed" };

    return [
      { id: "context", label: "Context", tone: ctx.tone, state: ctx.state, note: ctx.note },
      { id: "policy", label: "Policy", tone: pol.tone, state: pol.state, note: pol.note },
      { id: "authority", label: "Authority", tone: auth.tone, state: auth.state, note: auth.note },
      { id: "hands", label: "Actuators", tone: hands.tone, state: hands.state, note: hands.note }
    ];
  }

  function pct(v) { return v == null ? "n/a" : (v * 100).toFixed(0) + "%"; }
  function fix(v, n, unit) { return v == null ? "n/a" : v.toFixed(n) + (unit || ""); }

  function eyesRows(d) {
    var e = d.eyes;
    var io = (e.ioRead == null && e.ioWrite == null) ? "n/a" : fix(e.ioRead, 0) + " / " + fix(e.ioWrite, 0) + " KB/s";
    return [
      ["Temperature", fix(e.thermal, 1, " °C") + (e.hottest != null && e.thermal != null && e.hottest - e.thermal >= 1 ? " (hottest " + fix(e.hottest, 1) + ")" : ""), e.thermalTrend],
      ["Memory free", pct(e.mem), e.memTrend],
      ["CPU", pct(e.cpu), ""],
      ["Load (1m)", fix(e.load, 2), e.loadTrend],
      ["Storage I/O", io, ""],
      ["Battery", e.battery == null ? "n/a" : e.battery + "%" + (e.charging ? " · charging" : ""), ""],
      ["Confidence", pct(e.confidence), e.confidence != null && e.confidence < 0.7 ? "below 70% gate" : ""]
    ];
  }

  function deviceRows(d) {
    var v = d.device;
    if (!v || !v.present) {
      return [
        ["Device profile", "not supplied by this daemon build", "status has no device section"],
        ["Storage mutation policy", "unknown", "requires updated engine status"]
      ];
    }
    var identity = [v.manufacturer, v.model].filter(Boolean).join(" · ") || "n/a";
    var soc = [v.socManufacturer, v.socModel || v.hardware].filter(Boolean).join(" · ") || "n/a";
    var android = v.androidRelease ? v.androidRelease : "n/a";
    if (v.sdkLevel) android += " · API " + v.sdkLevel;
    var kernel = [v.kernelRelease, v.abi].filter(Boolean).join(" · ") || "n/a";
    var interfaces = ["proc " + (v.procAvailable ? "yes" : "no"),
      "sys " + (v.sysAvailable ? "yes" : "no"),
      "cgroup v2 " + (v.cgroupV2 ? "yes" : "no"),
      "uClamp " + (v.uclamp ? "yes" : "no"),
      "devfreq " + (v.devfreq ? "yes" : "no")].join(" · ");
    return [
      ["Manufacturer / model", identity, v.deviceName || v.product || "device properties"],
      ["SoC / hardware", soc, v.board ? "board " + v.board : "reported properties"],
      ["Android", android, v.product ? "product " + v.product : "build properties"],
      ["Kernel / ABI", kernel, "uname / runtime properties"],
      ["Exposed interfaces", interfaces, v.cpuset ? "cpuset available" : "cpuset not detected"],
      ["Capabilities", v.discoveredResources + " discovered · " + v.mutationReadyResources + " policy-ready", "writability alone does not grant authorization"],
      ["Block I/O mutation", v.blockQueueMutationsQuarantined ? "QUARANTINED" : "not quarantined", v.blockQueueMutationsQuarantined ? "new queue writes are blocked pending device/topology validation" : "check device policy"]
    ];
  }

  function brainRows(d) {
    var b = d.brain;
    return [
      ["State", b.state, b.previous && b.previous !== b.state ? "was " + b.previous : ""],
      ["Workload", b.workload, ""],
      ["Decision", b.decision, ""],
      ["Plan", b.planAction + " · " + b.intervention, b.stabilizingOnly ? "stabilizing only" : ""],
      ["Mode", b.mode + (b.armed ? " · armed" : " · not armed"), b.allowCpu ? "CPU governor allowed" : "CPU governor off"],
      ["Decision agent", b.agentMode || "ScoringOnly", b.agentReason || "no proposal this cycle"],
      ["Baseline", b.baselineReady ? "ready" : (b.baselineSamples + "/" + b.baselineTarget), ""],
      ["Observation epoch", b.observing ? (b.observationSamples + "/" + (b.observationTarget || 5)) : "idle", b.observing ? "measuring mutation outcome" : ""],
      ["Skip / wait reason", b.skipReason || "—", b.cooldown ? (b.cooldown + " cooldown left") : ""],
      ["Safety hold", b.holdActive ? b.hold : "none", ""]
    ];
  }

  function handsRows(d) {
    var h = d.hands;
    var rows = [];
    h.cpuApplied.forEach(function (x) { rows.push(["CPU governor", x.value || x.key, x.key]); });
    h.resourceApplied.forEach(function (x) { rows.push(["Resource", x.value, x.key]); });
    if (!rows.length) rows.push(["Changes held", "none", "the device is at its original settings"]);
    rows.push(["Verified / rolled back / failed", h.verified + " / " + h.rolledBack + " / " + h.failed, "since daemon start"]);
    rows.push(["Last change", h.lastKind ? (h.lastKind + " " + h.lastResult) : "none yet", h.lastKind ? "sample " + h.lastSample : ""]);
    return rows;
  }

  var api = {
    SCHEMA: SCHEMA, MAX_BYTES: MAX_BYTES, BLOCKERS: BLOCKERS,
    parse: parse, summary: summary, pipeline: pipeline,
    eyesRows: eyesRows, deviceRows: deviceRows, brainRows: brainRows, handsRows: handsRows
  };
  if (typeof module !== "undefined" && module.exports) module.exports = api;
  else root.CoreFlowStatus = api;
})(typeof window !== "undefined" ? window : this);
