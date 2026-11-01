/* CoreFlow live engine status: pure logic (no DOM). Consumed by app.js, tested with node.
 *
 * The C++ daemon is the only writer of status.json. This file only validates and
 * explains it. It never writes anywhere and carries no authority over the engine.
 */
(function (root) {
  "use strict";

  var MAX_BYTES = 65536;
  var SCHEMA = 1;
  // Stable consultation-ledger keys emitted by status_snapshot.cpp.
  var CONSULT_REASONS = ["NO_WRITE_ACTUATOR", "OUT_OF_SCOPE", "SURFACE_DENIED", "NOT_OBSERVABLE",
    "NO_ROLLBACK_BASELINE", "NO_SEMANTIC_PRIOR", "STRESSED_NON_STABILIZING", "NO_NEED",
    "NO_BENEFIT_EVIDENCE", "AWAITING_USER_GRANT", "ALLOWED"];
  var CONSULT_NAMESPACES = ["VM_SYSCTL", "BLOCK_QUEUE", "SCHEDULER", "GPU", "CGROUP", "CPUFREQ_TUNABLE", "KERNEL_TUNABLE", "CPU_FREQ", "BUILD_PROP", "SETTINGS_DB", "OTHER"];

  // Must match blockerCodes() in src/status_snapshot.cpp (checked by tools/test_webui_status.js).
  var BLOCKERS = {
    STOPPED: { tone: "bad", title: "Engine stopped", detail: "The daemon is not running. It restores any change it made when it shuts down." },
    THERMAL_STABILIZING: { tone: "warn", title: "Thermal stabilization", detail: "Performance boosts are withdrawn. The engine can apply verified, load-reducing plans from available kernel capabilities; existing cooling settings stay active while the next plan is measured." },
    RECOVERY_PENDING: { tone: "warn", title: "Engine recovery pending", detail: "Tuning is paused while ownership handoff or restoration completes. Displayed sensor values are from the last telemetry snapshot." },
    SAFETY_HOLD: { tone: "warn", title: "Safety hold active", detail: "The kill switch or SAFE_MODE is set, so mutation is disabled until it is cleared." },
    OBSERVE_MODE: { tone: "info", title: "Configuration blocked", detail: "The installed engine configuration does not enable tuning. Save Engine Settings and restart; recovery and operator stop markers remain authoritative." },
    NOT_ARMED: { tone: "warn", title: "Configuration incomplete", detail: "The compatibility activation fields are inconsistent. Save Engine Settings and restart; no extra operating mode needs to be selected." },
    BASELINE_CAPTURE: { tone: "info", title: "Measuring factory behaviour", detail: "The engine first records how the device behaves untouched, so it can tell later whether a change helped." },
    OBSERVING_OUTCOME: { tone: "info", title: "Measuring the effect of a change", detail: "A change was applied. The engine is checking whether it helped, and will roll it back if it did not." },
    CHANGE_HELD: { tone: "ok", title: "A verified change is in place", detail: "It stays active while further justified plans are evaluated against the current configuration. Stop, recovery or a safety transition restores the settings when required." },
    COOLDOWN: { tone: "info", title: "Cooling down after the last outcome", detail: "A pause between changes keeps each measurement attributable to one change." },
    CONTEXT_IDLE: { tone: "info", title: "Device is idle", detail: "Nothing is worth optimizing right now." },
    CONTEXT_LOW_CONFIDENCE: { tone: "warn", title: "Telemetry confidence too low", detail: "Readings are below the configured confidence threshold." },
    CONTEXT_NO_HEADROOM: { tone: "warn", title: "No headroom to tune", detail: "Thermal, memory or battery headroom is missing, and no stress-reducing change applies." },
    NO_CANDIDATES: { tone: "info", title: "No resource qualifies", detail: "No discovered, writable resource is justified under the current plan." },
    READY: { tone: "ok", title: "Ready to act", detail: "The engine will apply a change as soon as its model finds one justified." }
  };

  function isObj(v) { return v !== null && typeof v === "object" && !Array.isArray(v); }
  function num(v) { return (typeof v === "number" && isFinite(v)) ? v : null; }
  function int(v, d) { return (typeof v === "number" && isFinite(v) && v >= 0) ? Math.floor(v) : d; }
  function str(v, max) { return typeof v === "string" ? v.slice(0, max || 160) : ""; }
  function bool(v) { return v === true; }

  function evaluationSnapshot(v) {
    if (!v || typeof v !== "object" || Array.isArray(v)) return null;
    function n(x) { return typeof x === "number" && isFinite(x) ? x : null; }
    function count(x) { return typeof x === "number" && isFinite(x) && x >= 0 ? Math.floor(x) : 0; }
    var metrics = {}, source = v.metrics && typeof v.metrics === "object" ? v.metrics : {};
    ["thermal_c","load1","cpu_fraction","memory_fraction","psi_fraction","io_latency_ms","jank_fraction"].forEach(function(key) {
      var m = source[key]; if (!m || typeof m !== "object" || Array.isArray(m)) return;
      metrics[key] = { before: n(m.before), after: n(m.after), score: n(m.score),
        beforeSamples: count(m.before_samples), afterSamples: count(m.after_samples) };
    });
    return { reason: typeof v.reason === "string" ? v.reason.slice(0,100) : "unknown",
      outcome: count(v.outcome), valid: v.valid === true, score: n(v.score), confidence: n(v.confidence),
      contextCompatible: v.context_compatible === true, comparableMetrics: count(v.comparable_metrics),
      baselineSamples: count(v.baseline_samples), observationSamples: count(v.observation_samples), metrics: metrics };
  }

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
    var sel = isObj(j.selection) ? j.selection : {};
    var insight = isObj(j.intelligence) ? j.intelligence : {};
    var e = isObj(j.eyes) ? j.eyes : {};
    var b = isObj(j.brain) ? j.brain : {};
    var h = isObj(j.hands) ? j.hands : {};
    var v = isObj(j.device) ? j.device : {};
    return {
      version: str(j.version, 32),
      pid: int(j.pid, 0),
      sample: int(j.sample, 0),
      updated: int(j.updated_epoch, 0),
      telemetryUpdated: int(j.telemetry_epoch, int(j.updated_epoch, 0)),
      selection: { current: bool(sel.current), reason: str(sel.reason, 160), winner: str(sel.winner, 64), cpuReason: str(sel.cpu_reason, 200), resourceReason: str(sel.resource_reason, 200),
        cpuUtility: num(sel.cpu_utility), resourceUtility: num(sel.resource_utility), assessed: bool(sel.assessed),
        discovered: int(sel.discovered, 0), structural: int(sel.structural, 0), baseline: int(sel.baseline, 0),
        granted: int(sel.granted, 0), relevant: int(sel.relevant, 0), ranked: int(sel.ranked, 0), selectable: int(sel.selectable, 0), historyHeld: int(sel.history_held,0) },
      cpuPolicies: (Array.isArray(j.cpu_policies) ? j.cpu_policies : []).slice(0,16).filter(isObj).map(function(p) {
        return { path: str(p.path,4096), currentGovernor: str(p.current_governor,96),
          governorReadable: bool(p.governor_readable), governorWritable: bool(p.governor_writable),
          currentKhz: num(p.current_khz), minKhz: num(p.min_khz), maxKhz: num(p.max_khz),
          hardwareMinKhz: num(p.hardware_min_khz), hardwareMaxKhz: num(p.hardware_max_khz),
          relatedCpus: str(p.related_cpus,128),
          availableFrequenciesKhz: (Array.isArray(p.available_frequencies_khz) ? p.available_frequencies_khz : [])
            .slice(0,64).filter(function(v) { return Number.isSafeInteger(v) && v > 0; }),
          availableGovernors: (Array.isArray(p.available_governors) ? p.available_governors : []).slice(0,32).filter(function(t) { return typeof t === "string"; }).map(function(t) { return str(t,96); }) };
      }),
      trialHistory: { available: isObj(j.trial_history), ready: isObj(j.trial_history) ? bool(j.trial_history.ready) : true,
        records: isObj(j.trial_history) ? int(j.trial_history.records,0) : 0 },
      lastEvaluation: evaluationSnapshot(j.last_evaluation),
      outcomeReason: str(j.outcome_reason, 200),
      cohortIdentity: str(j.cohort_identity, 512),
      cohortMembers: int(j.cohort_members, 0),
      recoveryPhase: str(j.recovery_phase, 40),
      recoveryDetail: str(j.recovery_detail, 512),
      interval: int(j.interval_s, 5),
      running: j.running !== false,
      blocker: str(j.blocker, 40),
      memory: { lifetimeSample: isObj(j.memory) && Number.isSafeInteger(j.memory.lifetime_sample) && j.memory.lifetime_sample >= 0 ? j.memory.lifetime_sample : null, baselineRestartsAfterBoot: isObj(j.memory) && bool(j.memory.baseline_restarts_after_boot), sampleCheckpointOk: isObj(j.memory) && bool(j.memory.sample_checkpoint_ok) },
      device: {
        present: isObj(j.device),
        manufacturer: str(v.manufacturer, 96), model: str(v.model, 96),
        deviceName: str(v.device_name, 96), product: str(v.product, 96),
        board: str(v.board, 96), hardware: str(v.hardware, 96),
        socManufacturer: str(v.soc_manufacturer, 96), socModel: str(v.soc_model, 96),
        androidRelease: str(v.android_release, 32), sdkLevel: str(v.sdk_level, 16),
        securityPatch: str(v.security_patch, 40), firmwareIncremental: str(v.firmware_incremental, 80),
        kernelRelease: str(v.kernel_release, 128), abi: str(v.abi, 32),
        procAvailable: bool(v.proc_available), sysAvailable: bool(v.sys_available),
        cgroupV2: bool(v.cgroup_v2), cpuset: bool(v.cpuset_available),
        uclamp: bool(v.uclamp_available), schedulerControls: bool(v.scheduler_controls_available),
        devfreq: bool(v.devfreq_available), discoveredResources: int(v.discovered_resources, 0),
        mutationReadyResources: int(v.mutation_ready_resources, 0),
        vendorFilesParsed: int(v.vendor_files_parsed, 0), vendorKnobs: int(v.vendor_knobs, 0),
        discoveryTruncated: bool(v.discovery_truncated),
        expandedDomains: (function () {
          var out = [];
          if (!Array.isArray(v.expanded_domains)) return out;
          for (var i = 0; i < v.expanded_domains.length && i < 15; i++) {
            var x = v.expanded_domains[i];
            if (!isObj(x)) continue;
            var evidence = str(x.evidence, 16);
            if (["unavailable", "detected", "observable"].indexOf(evidence) < 0) evidence = "unavailable";
            out.push({ id: int(x.id, 0), evidence: evidence,
              matched: int(x.matched, 0), readable: int(x.readable, 0),
              mutationAuthorized: bool(x.mutation_authorized) });
          }
          return out;
        })(),
        blockQueueMutationsQuarantined: bool(v.block_queue_mutations_quarantined)
      },
      eyes: {
        // The engine says whether it has a thermal sensor; without one the numbers mean nothing.
        thermal: e.thermal_available === false ? null : num(e.thermal_c),
        hottest: e.thermal_available === false ? null : num(e.hottest_c),
        thermalTrend: str(e.thermal_trend, 16),
        mem: num(e.mem_available_ratio), memTrend: str(e.memory_trend, 16),
        cpu: num(e.cpu_utilization), load: e.load_available === true ? num(e.load1) : null, loadTrend: str(e.load_trend, 16),
        ioRead: num(e.io_read_kbs), ioWrite: num(e.io_write_kbs),
        battery: num(e.battery_percent), charging: bool(e.charging), confidence: num(e.confidence)
      },
      intelligence: {
        pressure: str(insight.pressure, 32), confidence: num(insight.confidence),
        foregroundAvailable: bool(insight.foreground_available), foregroundProcess: str(insight.foreground_process, 256),
        renderingLayer: str(insight.rendering_layer, 1024), renderingAvailability: str(insight.rendering_availability, 64),
        interactionState: str(insight.interaction_state, 32),
        renderingMetric: str(insight.rendering_metric, 64), launchMetric: str(insight.launch_metric, 64),
        artBackend: str(insight.art_backend, 64), framePacingBackend: str(insight.frame_pacing_backend, 64),
        renderingAvailable: bool(insight.rendering_available), frameJankRate: num(insight.frame_jank_rate),
        controlConflict: bool(insight.control_conflict), externalControlPaths: int(insight.external_control_paths, 0), learnedCandidates: int(insight.learned_candidates, 0),
        onnxSessionReady: bool(insight.onnx_session_ready), onnxLastInference: bool(insight.onnx_last_inference),
        onnxInferences: int(insight.onnx_inferences, 0), heuristicFallbacks: int(insight.heuristic_fallbacks, 0),
        storageAvailable: bool(insight.storage_available), storageCaution: bool(insight.storage_caution),
        lifeA: int(insight.ufs_life_a, 0), lifeB: int(insight.ufs_life_b, 0), preEol: int(insight.ufs_pre_eol, 0),
        powerHalBackend: str(insight.power_hal_backend, 64), adpfBackend: str(insight.adpf_backend, 64)
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
        observing: bool(b.observing), cooldown: int(b.cooldown_remaining, 0),
        consultation: (function () {
          var c = isObj(b.consultation) ? b.consultation : {};
          var reasons = isObj(c.by_reason) ? c.by_reason : {};
          var namespaces = isObj(c.by_namespace) ? c.by_namespace : {};
          var reasonCounts = {};
          CONSULT_REASONS.forEach(function (key) { if (Object.prototype.hasOwnProperty.call(reasons, key)) reasonCounts[key] = int(reasons[key], 0); });
          var last = {};
          CONSULT_NAMESPACES.forEach(function (key) {
            if (!Object.prototype.hasOwnProperty.call(namespaces, key)) return;
            var item = isObj(namespaces[key]) ? namespaces[key] : {};
            last[str(key, 32)] = { reason: str(item.last_reason, 48), need: bool(item.need_present) };
          });
          return { reasons: reasonCounts, namespaces: last };
        })()
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
    if (d.trialHistory.available && !d.trialHistory.ready &&
        ["NO_CANDIDATES","READY","CHANGE_HELD"].indexOf(d.blocker)!==-1)
      return { tone: "warn", title: "Trial memory checkpoint pending", detail: "New trials are held until admission history is available. Existing accepted settings and recovery obligations are preserved.", code: d.blocker };
    if (d.selection.current && d.blocker === "NO_CANDIDATES") {
      if (d.selection.reason === "no_measured_tuning_need_keep_current")
        return { tone: "ok", title: "No measured tuning need", detail: "Current telemetry does not justify a new experiment. The current configuration is kept; this does not prove it is globally optimal.", code: "NO_CANDIDATES" };
      if (d.selection.historyHeld > 0 && d.selection.reason === "measured_need_but_no_admissible_gain")
        return { tone: "info", title: "Previous trials held for current conditions", detail: "Recorded trials or accepted settings hold " + d.selection.historyHeld + " proposal(s). A material workload or device-condition change is required; elapsed cooldown alone does not reopen them.", code: "NO_CANDIDATES" };
      if (d.selection.reason === "measured_need_but_no_admissible_gain")
        return { tone: "warn", title: "Tuning need detected; no admissible plan", detail: "Inventory " + d.selection.discovered + "; granted baselines " + d.selection.granted + "; relevant " + d.selection.relevant + "; selectable reviewed proposals " + d.selection.selectable + ". Capability discovery alone does not justify a write.", code: "NO_CANDIDATES" };
    }
    var info = BLOCKERS[d.blocker];
    if (!info) return { tone: "info", title: "Unrecognized status", detail: "The engine reported a code this page does not know: " + d.blocker, code: d.blocker };
    var detail = info.detail;
    if (d.blocker === "THERMAL_STABILIZING" && !d.brain.baselineReady)
      detail += " Capturing a matched thermal baseline " + d.brain.baselineSamples + "/" + d.brain.baselineTarget + ".";
    if (d.cohortMembers > 1) detail += " Last coordinated plan: " + d.cohortMembers + " targets.";
    if (d.blocker === "BASELINE_CAPTURE" && d.brain.baselineTarget) detail += " (" + d.brain.baselineSamples + "/" + d.brain.baselineTarget + " samples)";
    if (d.blocker === "COOLDOWN") detail += " " + d.brain.cooldown + " sample(s) left.";
    if (d.blocker === "CONTEXT_LOW_CONFIDENCE" && d.eyes.confidence != null) detail += " Now " + Math.round(d.eyes.confidence * 100) + "%.";
    if (d.blocker === "OBSERVING_OUTCOME" && d.brain.observationTarget)
      detail += " Epoch " + d.brain.observationSamples + "/" + d.brain.observationTarget + ".";
    if (d.blocker === "RECOVERY_PENDING") {
      detail += " Heartbeat " + p.age + "s; telemetry " + (d.telemetryUpdated ? Math.max(0, p.now - d.telemetryUpdated) + "s old" : "unavailable") + ".";
      if (d.recoveryDetail) detail += " " + d.recoveryDetail;
    }
    if (d.brain.skipReason && d.blocker !== "RECOVERY_PENDING") detail += " · " + d.brain.skipReason;
    return { tone: info.tone, title: info.title, detail: detail, code: d.blocker };
  }

  // The decision pipeline: where a change is stopped, stage by stage.
  function pipeline(d) {
    var b = d.brain, h = d.hands;
    if (d.recoveryPhase) return [
      { id: "context", label: "Context", tone: "warn", state: "Paused", note: "Telemetry snapshot predates this recovery heartbeat" },
      { id: "policy", label: "Policy", tone: "warn", state: "Hold", note: "No new mutation while recovery is pending" },
      { id: "authority", label: "Authority", tone: "bad", state: "Denied", note: "recovery pending" },
      { id: "hands", label: "Actuators", tone: "warn", state: "Recovering", note: d.recoveryDetail || d.recoveryPhase }
    ];
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
      var mode = String(b.mode || "").toLowerCase(); var why = b.holdActive ? "safety hold" : mode !== "adaptive" ? "configuration blocked" : !b.armed ? "activation incomplete" : "plan not permitted";
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
      ["Confidence", pct(e.confidence), e.confidence != null && e.confidence < 0.6 ? "below 60% default gate" : ""]
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
      ["Expanded domains", v.expandedDomains.filter(function (x) { return x.evidence !== "unavailable"; }).length + " / 15 detected · " + v.expandedDomains.filter(function (x) { return x.evidence === "observable"; }).length + " observable", "discovery is observation-only; domain status never grants mutation authority"],
      ["Block I/O mutation", v.blockQueueMutationsQuarantined ? "QUARANTINED" : "capability-driven", v.blockQueueMutationsQuarantined ? "new queue writes are blocked" : "every discovered non-fatal queue / iosched tunable; legacy grant flags are ignored"]
    ];
  }

  function activity(d) {
    if (d.recoveryPhase) return "RECOVERING";
    if (d.brain.holdActive || d.brain.mode !== "adaptive" || !d.brain.armed) return "HELD";
    if (d.brain.observing) return "MEASURING";
    if (d.hands.lastCpu === "FAILED" || d.hands.lastResource === "FAILED") return "FAILED";
    if (d.hands.lastCpu === "ROLLED_BACK" || d.hands.lastResource === "ROLLED_BACK") return "ROLLED_BACK";
    if (d.hands.cpuMutated || d.hands.resourceMutated) return "HOLDING";
    if (d.brain.cooldown) return "COOLDOWN";
    if (!d.brain.baselineReady) return "BASELINE";
    if (d.selection.current && d.selection.winner === "KEEP_CURRENT") return "KEEP_CURRENT";
    return "ASSESSING";
  }

  function brainRows(d) {
    var b = d.brain;
    var i = d.intelligence || {};
    return [
      ["Measured pressure", i.pressure || "not supplied", "evidence, not proof of application causality"],
      ["Control continuity", i.controlConflict ? (String(i.externalControlPaths || 0) + " resource(s) controlled externally") : "no conflict reported", "Adaptive continues on uncontested resources; rediscovery retries excluded paths"],
      ["Rendering", i.renderingAvailable ? pct(i.frameJankRate) : "unavailable", i.renderingAvailability || "presentation interval proxy; app target unknown"],
      ["Foreground", i.foregroundAvailable ? i.foregroundProcess : "unavailable", "cgroup process heuristic; not measured launch time"],
      ["Render surface", i.renderingLayer || "unresolved", "package-qualified layer; display-period proxy"],
      ["ART management", i.artBackend || "not supplied", "Android manages compilation and background maintenance"],
      ["Frame pacing", i.framePacingBackend || "not supplied", "in-app integration required for Swappy"],
      ["Storage health", i.storageAvailable ? (i.storageCaution ? "caution" : "no warning reported") : "unavailable", "UFS descriptor codes; not an exact remaining lifetime"],
      ["Learning", String(i.learnedCandidates || 0) + " candidates", "measured local experience"],
      ["Thermal model", i.onnxSessionReady ? "ONNX session loaded" : "heuristic only", "session load alone is not inference"],
      ["Thermal inference", i.onnxInferences + " ONNX / " + i.heuristicFallbacks + " heuristic", i.onnxInferences + i.heuristicFallbacks ? (i.onnxLastInference ? "last prediction: model" : "last prediction: heuristic") : "no predictions yet"],

      ["State", b.state, b.previous && b.previous !== b.state ? "was " + b.previous : ""],
      ["Workload", b.workload, ""],
      ["Policy decision", b.decision, "State-policy recommendation; actuator activity is reported separately"],
      ["Plan", b.planAction + " · " + b.intervention, b.stabilizingOnly ? "stabilizing only" : ""],
      ["Operation", b.mode === "adaptive" && b.armed ? "active" : "held", b.permitCpu ? "CPU scope gate open" : "CPU scope gate held"],
      ["Decision agent", b.agentMode || "ScoringOnly", b.agentReason || "no proposal this cycle"],
      ["Baseline", b.baselineReady ? "ready" : (b.baselineSamples + "/" + b.baselineTarget), ""],
      ["Observation epoch", b.observing ? (b.observationSamples + "/" + (b.observationTarget || 5)) : "idle", b.observing ? "measuring mutation outcome" : ""],
      ["Last selection", d.selection.winner || "—", d.selection.reason || "—"],
      ["CPUFreq selection", d.selection.cpuReason || "not assessed", "Advertised alternatives compete with the live governor"],
      ["Resource selection", d.selection.resourceReason || "not assessed", "Requires per-domain evidence and authority"],
      ["Predicted net gain", "CPU " + fix(d.selection.cpuUtility, 3) + " · resources " + fix(d.selection.resourceUtility, 3), "relative to keeping the current configuration"],
      ["Candidate review", d.selection.assessed ? (d.selection.selectable + " selectable / " + d.selection.ranked + " reviewed") : "not assessed", d.selection.discovered + " inventory; " + d.selection.granted + " granted baselines; " + d.selection.relevant + " relevant"],
      ["Trial memory", !d.trialHistory.available ? "not supplied" : d.trialHistory.ready ? "ready" : "checkpoint pending", d.trialHistory.records + " records; " + d.selection.historyHeld + " proposal holds"],
      ["Last outcome", d.outcomeReason || "—", "Measured result; separate from predicted selection scores"],
      ["Last plan", (d.cohortMembers || 0) + " target(s)", d.cohortIdentity || "—"],
      ["Skip / wait reason", b.skipReason || "—", b.cooldown ? (b.cooldown + " cooldown left") : ""],
      ["Safety hold", b.holdActive ? b.hold : "none", ""],
      ["Consultation ledger", Object.keys(b.consultation.reasons).length ? Object.keys(b.consultation.reasons).map(function (k) { return k + " " + b.consultation.reasons[k]; }).join(" · ") : "not supplied by this daemon", "cumulative since daemon start"],
      ["Last namespace reasons", Object.keys(b.consultation.namespaces).map(function (k) { return k + ": " + b.consultation.namespaces[k].reason + (b.consultation.namespaces[k].need ? " (need present)" : ""); }).join(" · ") || "not supplied by this daemon", "read-only consultation telemetry"]
    ];
  }

  function handsRows(d) {
    var h = d.hands;
    var rows = [];
    h.cpuApplied.forEach(function (x) { rows.push(["CPU governor", x.value || x.key, x.key]); });
    h.resourceApplied.forEach(function (x) { rows.push(["Resource", x.value, x.key]); });
    if (!rows.length) rows.push(["Changes held", "none", "no engine-owned changes reported"]);
    rows.push(["Verified / rolled back / failed", h.verified + " / " + h.rolledBack + " / " + h.failed, "since daemon start"]);
    rows.push(["Last change", h.lastKind ? (h.lastKind + " " + h.lastResult) : "none yet", h.lastKind ? "sample " + h.lastSample : ""]);
    return rows;
  }

  var api = {
    SCHEMA: SCHEMA, MAX_BYTES: MAX_BYTES, BLOCKERS: BLOCKERS,
    parse: parse, summary: summary, pipeline: pipeline,
    activity: activity, eyesRows: eyesRows, deviceRows: deviceRows, brainRows: brainRows, handsRows: handsRows
  };
  if (typeof module !== "undefined" && module.exports) module.exports = api;
  else root.CoreFlowStatus = api;
})(typeof window !== "undefined" ? window : this);
