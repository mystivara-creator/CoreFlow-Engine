/* CoreFlow Pulse WebUI: lightweight native HTML/CSS presentation for the coreflowd daemon.
 * Controls use fixed command builders; the daemon remains the sole authority for kernel writes.
 * Runtime status, logs and journal values are always rendered as text, never as HTML.
 */
(function () {
  "use strict";

  var S = window.CoreFlowStatus;
  var C = window.CoreFlowControl;

  var TITLES = { engine: "Engine", policy: "Policy", journal: "Journal", about: "Project & AI" };

  var state = {
    page: "engine",
    bridge: null,          // null = not probed yet, true = responding, false = no KernelSU bridge
    probe: null,
    status: null,
    statusFails: 0,
    pollTimer: null,
    pollBusy: false,
    busy: false,
    configLoaded: false,
    configRaw: "",
    config: {},
    form: null,
    numInputs: {},
    journalView: "decisions",
    traceFilter: "all",
    trace: [],
    logText: ""
  };

  var MESSAGES = {
    START_REQUESTED: "Start requested. The supervisor is launching coreflowd.",
    ALREADY_RUNNING: "coreflowd is already running.",
    STOP_OK: "Engine stopped.",
    STOP_TIMEOUT: "Stop timed out. The daemon is still running.",
    RESTART_OK: "Engine restarted with the saved configuration.",
    RESTART_TIMEOUT: "Restart timed out. Check the Journal for the daemon log.",
    REFRESH_SIGNAL_SENT: "Re-discovery requested. Results appear in the Journal.",
    NOT_RUNNING: "coreflowd is not running, so there is nothing to signal.",
    CONFIG_COMMIT_OK: "Configuration saved."
  };

  var DIALOGS = {
    stop: {
      title: "Stop coreflowd?",
      message: "The daemon restores any change it made before it exits. The supervisor stops too, so the engine stays off until you start it again or reboot.",
      detail: "SIGTERM -> supervisor and coreflowd",
      label: "Stop engine"
    },
    restart: {
      title: "Restart coreflowd?",
      message: "Loads the saved configuration. The daemon recovers its mutation journal on start, the same as after a reboot.",
      detail: "relaunch under the supervisor",
      label: "Restart"
    },
    arm: {
      title: "Enable Adaptive mode?",
      message: "The engine may apply verified changes across supported non-fatal namespaces. Thermal guards, fatal-surface exclusions, the journal and rollback stay active. Takes effect after restart.",
      detail: "mutation_mode=adaptive\nmutation_armed=true",
      label: "Enable Adaptive"
    },
    resume: {
      title: "Resume Adaptive mode?",
      message: "This removes the DISABLE kill switch and arms Adaptive. Continue only if you intend to resume. SAFE_MODE is never bypassed.",
      detail: "remove DISABLE\nmutation_mode=adaptive\nmutation_armed=true",
      label: "Resume Adaptive"
    },
    observe: {
      title: "Return to Observe?",
      message: "Writes mutation_mode=observe and creates the DISABLE kill switch, so a running Adaptive daemon enters its safety hold and restores state.",
      detail: "mutation_mode=observe\nmutation_armed=false\ncreate DISABLE",
      label: "Switch to Observe"
    }
  };

  var $ = function (id) { return document.getElementById(id); };

  // ---- bridge & small helpers ----

  function execShell(command, timeoutMs) {
    return new Promise(function (resolve) {
      var done = false;
      function finish(errno, stdout, stderr) {
        if (done) return;
        done = true;
        clearTimeout(timer);
        resolve({
          errno: Number(errno) || 0,
          stdout: stdout == null ? "" : String(stdout),
          stderr: stderr == null ? "" : String(stderr)
        });
      }
      var timer = setTimeout(function () { finish(1, "", "WebUI execution timed out"); }, timeoutMs || 15000);
      try {
        if (typeof ksu === "undefined" || typeof ksu.exec !== "function") {
          finish(1, "", "No KernelSU WebUI exec bridge.");
          return;
        }
        var cb = "cf_cb_" + Date.now() + "_" + Math.floor(Math.random() * 1e6);
        window[cb] = function (e, o, s) {
          try { delete window[cb]; } catch (_) { /* ignore */ }
          finish(e, o, s);
        };
        try {
          ksu.exec(command, cb);
        } catch (e) {
          try { ksu.exec(command, "{}", cb); } catch (e2) { finish(1, "", String(e2 || e)); }
        }
      } catch (e) {
        finish(1, "", String(e));
      }
    });
  }

  function el(tag, cls, text) {
    var n = document.createElement(tag);
    if (cls) n.className = cls;
    if (text != null) n.textContent = String(text);
    return n;
  }

  function setText(id, value) {
    var n = $(id);
    if (n) n.textContent = value;
  }

  function clear(node) {
    while (node.firstChild) node.removeChild(node.firstChild);
  }

  function toast(message, isError) {
    var n = el("div", "toast" + (isError ? " error" : ""), message);
    n.setAttribute("role", "status");
    $("toastRegion").appendChild(n);
    setTimeout(function () { n.remove(); }, 4200);
  }

  function friendly(stdout, stderr) {
    var last = String(stdout || "").trim().split(/\r?\n/).pop() || "";
    return MESSAGES[last] || last || String(stderr || "").trim();
  }

  function rangeText(f) {
    return f.max != null ? "between " + f.min + " and " + f.max : "at least " + f.min;
  }

  // ---- confirmation dialog (Promise<boolean>) ----

  function confirmDialog(key) {
    var spec = DIALOGS[key];
    return new Promise(function (resolve) {
      var d = $("confirmDialog");
      setText("dialogTitle", spec.title);
      setText("dialogMessage", spec.message);
      setText("dialogDetail", spec.detail);
      setText("dialogConfirm", spec.label);
      var onClose = function () {
        d.removeEventListener("close", onClose);
        resolve(d.returnValue === "confirm");
      };
      d.addEventListener("close", onClose);
      d.showModal();
    });
  }

  // ---- navigation & polling ----

  function showPage(name) {
    var sections = document.querySelectorAll("section.page");
    var exists = Array.prototype.some.call(sections, function (s) { return s.getAttribute("data-page") === name; });
    if (!TITLES[name] || !exists) return;
    state.page = name;
    Array.prototype.forEach.call(sections, function (s) {
      s.hidden = s.getAttribute("data-page") !== name;
    });
    document.querySelectorAll("[data-page-link]").forEach(function (b) {
      b.toggleAttribute("selected", b.getAttribute("data-page-link") === name);
    });
    setText("pageTitle", TITLES[name]);
    $("main").scrollTop = 0;
    if (name === "engine") {
      pollEngine();
    } else {
      clearTimeout(state.pollTimer);
      state.pollTimer = null;
    }
    if (name === "policy") loadPolicy();
    if (name === "journal") loadJournal();
  }

  function scheduleEngine() {
    clearTimeout(state.pollTimer);
    state.pollTimer = null;
    if (state.page !== "engine" || document.visibilityState === "hidden") return;
    state.pollTimer = setTimeout(pollEngine, state.statusFails > 2 ? 10000 : 3000);
  }

  function pollEngine() {
    if (state.pollBusy) return;
    state.pollBusy = true;
    return execShell(C.snapshotCmd(), 15000).then(function (r) {
      state.bridge = !/No KernelSU WebUI exec bridge/.test(r.stderr);
      var parts = C.splitSnapshot(r.stdout);
      state.probe = parts.probe ? C.parseProbe(parts.probe) : null;
      state.status = parts.status ? S.parse(parts.status) : { ok: false, error: r.stderr || "The bridge returned no data." };
      state.statusFails = r.errno === 0 ? 0 : state.statusFails + 1;
      renderEngine();
      renderPolicyBanners();
    }).catch(function (e) {
      state.statusFails += 1;
      state.status = { ok: false, error: String(e) };
      renderEngine();
    }).finally(function () {
      state.pollBusy = false;
      scheduleEngine();
    });
  }

  // ---- Structured runtime dashboard ----

  function pct(v) { return v == null ? "n/a" : Math.round(v * 100) + "%"; }
  function metric(id, noteId, barId, value, note, ratio) {
    setText(id, value);
    setText(noteId, note || "No telemetry reported");
    var bar = $(barId);
    if (bar) bar.style.width = ratio == null ? "0%" : Math.max(0, Math.min(100, ratio)) + "%";
  }
  function fillRows(id, rows) {
    var host = $(id);
    clear(host);
    if (!rows || !rows.length) {
      host.appendChild(el("div", "empty", "No data reported by this daemon build."));
      return;
    }
    rows.forEach(function (r) {
      var row = el("div");
      row.append(el("dt", "", r[0]), el("dd", "", r[1] == null || r[1] === "" ? "—" : r[1]));
      if (r[2]) row.appendChild(el("small", "", r[2]));
      host.appendChild(row);
    });
  }
  function progress(id, value, target) {
    var pctValue = target > 0 ? Math.round(Math.min(1, value / target) * 100) : 0;
    var node = $(id);
    if (node) node.style.width = pctValue + "%";
  }
  function toneForFlag(flag) { return flag ? "warn" : "ok"; }

  function renderConsultation(d) {
    var reasons = d && d.brain && d.brain.consultation ? d.brain.consultation.reasons : {};
    var namespaces = d && d.brain && d.brain.consultation ? d.brain.consultation.namespaces : {};
    var reasonKeys = ["NO_WRITE_ACTUATOR", "OUT_OF_SCOPE", "SURFACE_DENIED", "NOT_OBSERVABLE",
      "NO_ROLLBACK_BASELINE", "NO_SEMANTIC_PRIOR", "STRESSED_NON_STABILIZING", "NO_NEED",
      "NO_BENEFIT_EVIDENCE", "AWAITING_USER_GRANT", "ALLOWED"];
    var reasonHost = $("consultReasonGrid");
    clear(reasonHost);
    var total = 0;
    reasonKeys.forEach(function (key) {
      var count = Object.prototype.hasOwnProperty.call(reasons || {}, key) ? Number(reasons[key]) || 0 : 0;
      total += count;
      var card = el("div", "ledger-item" + (key === "ALLOWED" ? " allowed" : ""));
      card.append(el("span", "", key.replace(/_/g, " ")));
      card.append(el("b", "", String(count)));
      reasonHost.appendChild(card);
    });
    setText("consultTotal", total + " total");

    var nsHost = $("namespaceList");
    clear(nsHost);
    var keys = Object.keys(namespaces || {});
    if (!keys.length) {
      nsHost.appendChild(el("div", "empty", "Namespace consultation has not been supplied by this daemon build."));
      return;
    }
    keys.forEach(function (key) {
      var item = namespaces[key] || {};
      var row = el("div", "namespace-row");
      var label = el("div", "namespace-name", key.replace(/_/g, " "));
      var value = el("div", "namespace-reason");
      value.appendChild(el("b", "", item.reason || "NO RECENT REASON"));
      value.appendChild(el("small", "", item.need ? "Need present in current consultation" : "No need marker reported"));
      row.append(label, value);
      nsHost.appendChild(row);
    });
  }

  function renderCapabilities(d) {
    var host = $("capabilityGrid");
    var domainHost = $("domainGrid");
    clear(host); clear(domainHost);
    var v = d && d.device;
    if (!v || !v.present) {
      host.appendChild(el("div", "empty", "Capability matrix is not available in this status snapshot."));
      return;
    }
    var domains = v.expandedDomains || [];
    var detected = domains.filter(function (x) { return x.evidence !== "unavailable"; }).length;
    var observable = domains.filter(function (x) { return x.evidence === "observable"; }).length;
    var authorized = domains.filter(function (x) { return x.mutationAuthorized; }).length;
    [
      ["Discovered resources", v.discoveredResources, "Interfaces found by discovery"],
      ["Policy-ready", v.mutationReadyResources, "Do not imply authorization"],
      ["Expanded domains", detected + " / " + domains.length, "Detected or observable"],
      ["Observable domains", observable, "Telemetry available"],
      ["Authorized domains", authorized, "Explicit permission reported"],
      ["Kernel interfaces", (v.procAvailable ? "proc ✓" : "proc —") + " · " + (v.sysAvailable ? "sys ✓" : "sys —"), "Runtime availability"]
    ].forEach(function (a) {
      var card = el("article", "capability-card");
      card.append(el("span", "", a[0]), el("b", "", String(a[1])), el("small", "", a[2]));
      host.appendChild(card);
    });
    if (!domains.length) {
      domainHost.appendChild(el("div", "empty", "No expanded-domain inventory was reported."));
      return;
    }
    domains.forEach(function (x) {
      var card = el("article", "domain-card");
      var head = el("div");
      head.append(el("b", "", "Domain " + x.id), el("span", "", x.evidence));
      card.appendChild(head);
      card.appendChild(el("p", "", x.matched + " matched · " + x.readable + " readable · mutation " + (x.mutationAuthorized ? "authorized" : "not authorized")));
      domainHost.appendChild(card);
    });
  }

  function renderEngine() {
    var p = state.status;
    var probe = state.probe;
    var online = !!(probe && probe.running);
    var ok = !!(p && p.ok);
    var d = ok ? p.data : null;
    var b = d ? d.brain : null;
    var e = d ? d.eyes : null;
    var sum = p ? S.summary(p) : { tone: "info", title: "Reading daemon state", detail: "Waiting for the first probe.", code: "INITIALIZING" };

    var hero = $("heroCard");
    hero.setAttribute("data-tone", sum.tone);
    setText("heroCode", sum.code || "—");
    setText("heroTitle", sum.title);
    setText("heroDetail", sum.detail);

    var chip = $("headerChip");
    if (state.bridge === false) {
      chip.setAttribute("data-tone", "bad"); chip.textContent = "NO BRIDGE";
    } else if (!probe) {
      chip.setAttribute("data-tone", "info"); chip.textContent = "CHECKING";
    } else {
      chip.setAttribute("data-tone", !online ? "bad" : sum.tone === "bad" ? "bad" : sum.tone === "warn" ? "warn" : "ok");
      chip.textContent = online ? "ONLINE" : "OFFLINE";
    }
    setText("headerSub", "coreflowd" + (online ? " · pid " + probe.pids.join(" ") : probe ? " · not running" : " · connecting"));
    setText("metaPid", online ? probe.pids.join(", ") : "—");
    setText("metaVersion", ok && d.version ? d.version : "—");
    setText("metaUpdated", ok ? (p.age === 0 ? "just now" : p.age + "s ago") : "—");

    if (e) {
      var tempText = e.thermal == null ? "n/a" : e.thermal.toFixed(1) + "°C";
      var tempNote = e.hottest != null ? "Hottest sensor " + e.hottest.toFixed(1) + "°C" : (e.thermalTrend || "Thermal probe");
      metric("metricTemp", "metricTempNote", "metricTempBar", tempText, tempNote, e.thermal == null ? null : e.thermal / 65 * 100);
      metric("metricCpu", "metricCpuNote", "metricCpuBar", pct(e.cpu), e.load == null ? "CPU utilization" : "1m load " + e.load.toFixed(2), e.cpu == null ? null : e.cpu * 100);
      metric("metricMemory", "metricMemoryNote", "metricMemoryBar", pct(e.mem), e.memTrend || "Available memory", e.mem == null ? null : e.mem * 100);
      metric("metricBattery", "metricBatteryNote", "metricBatteryBar", e.battery == null ? "n/a" : e.battery + "%", e.charging ? "Charging" : "Not charging", e.battery);
      metric("metricConfidence", "metricConfidenceNote", "metricConfidenceBar", pct(e.confidence), e.confidence != null && e.confidence < 0.7 ? "Below 70% confidence gate" : "Decision telemetry quality", e.confidence == null ? null : e.confidence * 100);
    } else {
      [["metricTemp","metricTempNote","metricTempBar"],["metricCpu","metricCpuNote","metricCpuBar"],["metricMemory","metricMemoryNote","metricMemoryBar"],["metricBattery","metricBatteryNote","metricBatteryBar"],["metricConfidence","metricConfidenceNote","metricConfidenceBar"]].forEach(function (a) { metric(a[0], a[1], a[2], "—", "Waiting for telemetry", null); });
    }

    var pipe = $("pipeline"); clear(pipe);
    if (ok) {
      S.pipeline(d).forEach(function (stage, i) {
        var row = el("div", "stage"); row.setAttribute("data-tone", stage.tone);
        var body = el("div"); body.append(el("b", "", stage.label), el("p", "", stage.note));
        row.append(el("span", "stage-index", String(i + 1)), body, el("span", "state", stage.state));
        pipe.appendChild(row);
      });
    } else pipe.appendChild(el("p", "empty", "The decision pipeline appears after the daemon writes its first status snapshot."));

    fillRows("eyesRows", ok ? S.eyesRows(d) : []);
    fillRows("brainRows", ok ? S.brainRows(d).filter(function (r) { return r[0] !== "Consultation ledger" && r[0] !== "Last namespace reasons"; }) : []);
    fillRows("handsRows", ok ? S.handsRows(d) : []);
    fillRows("deviceRows", ok ? S.deviceRows(d) : []);
    fillRows("safetyRows", probe ? [
      ["SAFE_MODE", probe.safe ? "PRESENT" : "absent", probe.safe ? "Adaptive is blocked" : ""],
      ["DISABLE kill switch", probe.disable ? "PRESENT" : "absent", probe.disable ? "mutation held" : ""],
      ["Config restart pending", probe.pending ? "yes" : "no", "config.ini is loaded at daemon start"],
      ["Boot attempts", probe.bootAttempts || "0", "unconfirmed starts"]
    ] : []);

    if (b) {
      var baselineTarget = b.baselineTarget || 0;
      var observationTarget = b.observationTarget || 0;
      setText("baselineMeta", b.baselineReady ? "Ready" : (b.baselineSamples + " / " + (baselineTarget || "—")));
      setText("observationMeta", b.observing ? (b.observationSamples + " / " + (observationTarget || 5) + " samples") : "Idle");
      progress("baselineProgress", b.baselineReady ? 1 : b.baselineSamples, b.baselineReady ? 1 : baselineTarget);
      progress("observationProgress", b.observing ? b.observationSamples : 0, b.observing ? (observationTarget || 5) : 1);
      setText("stateMeta", b.state || "—"); setText("workloadMeta", b.workload || "—");
      setText("cooldownMeta", b.cooldown ? b.cooldown + " samples" : "None");
      setText("skipMeta", b.skipReason || "None"); setText("holdMeta", b.holdActive ? (b.hold || "Active") : "None");
      setText("agentMeta", (b.agentMode || "ScoringOnly") + (b.agentReason ? " · " + b.agentReason : ""));
      setText("planMeta", (b.planAction || "—") + (b.intervention ? " · level " + b.intervention : "") + (b.planReason ? " · " + b.planReason : ""));
      var modeBadge = $("modeBadge"); modeBadge.textContent = (b.mode || "observe").toUpperCase() + (b.armed ? " · ARMED" : "");
      modeBadge.setAttribute("data-tone", b.mode === "adaptive" && b.armed ? "warn" : "info");
      var safetyBadge = $("safetyBadge"); safetyBadge.textContent = b.holdActive ? "SAFETY HOLD" : "NO HOLD";
      safetyBadge.setAttribute("data-tone", b.holdActive ? "warn" : "ok");
    } else {
      ["baselineMeta","observationMeta","stateMeta","workloadMeta","cooldownMeta","skipMeta","holdMeta","agentMeta","planMeta"].forEach(function (id) { setText(id, "Waiting for status"); });
      progress("baselineProgress", 0, 1); progress("observationProgress", 0, 1);
      setText("modeBadge", "UNKNOWN"); setText("safetyBadge", "UNKNOWN");
    }
    renderConsultation(d);
    renderCapabilities(d);

    var blocked = state.bridge === false;
    $("bridgeNotice").hidden = !blocked;
    var idle = !state.busy && !blocked;
    $("btnStart").disabled = !idle || online;
    $("btnStop").disabled = !idle || !online;
    $("btnRestart").disabled = !idle;
    $("btnRefresh").disabled = !idle || !online;
    setText("engineFooter", ok && p.stale
      ? "The daemon has not written status recently. It may be stalled; check the Journal before enabling Adaptive."
      : "Refreshes every 3 seconds while visible. This page reads status.json and sends fixed daemon-control requests; it does not write kernel tunables.");
  }

  // ---- engine actions ----

  function runAction(cmd, opts) {
    if (state.busy || state.bridge === false) return;
    var go = function () {
      state.busy = true;
      renderEngine();
      return execShell(cmd, 70000).then(function (r) {
        var ok = r.errno === 0;
        var text = friendly(r.stdout, r.stderr);
        toast(ok ? (opts.ok || text || "Done.") : (text || "Action failed."), !ok);
      }).finally(function () {
        state.busy = false;
        pollEngine();
      });
    };
    if (opts.confirm) {
      return confirmDialog(opts.confirm).then(function (yes) { if (yes) return go(); });
    }
    return go();
  }

  // ---- policy ----

  function buildPolicyFields() {
    [["runtimeFields", "runtime"], ["interactivityFields", "interactivity"]].forEach(function (pair) {
      var host = $(pair[0]);
      clear(host);
      C.NUMERIC.filter(function (f) { return f.group === pair[1]; }).forEach(function (f) {
        var wrap = el("div", "field");
        var input = document.createElement("input");
        input.setAttribute("id", "field_" + f.key);
        input.setAttribute("name", f.key);
        input.setAttribute("type", "number");
        input.setAttribute("inputmode", f.kind === "int" ? "numeric" : "decimal");
        input.setAttribute("step", f.kind === "int" ? "1" : "any");
        input.setAttribute("min", String(f.min));
        if (f.max != null) input.setAttribute("max", String(f.max));
        var label = el("label", "", f.label + (f.unit ? " (" + f.unit + ")" : ""));
        label.setAttribute("for", "field_" + f.key);
        var help = el("p", "field-help", f.help + " Range: " + rangeText(f) + ".");
        var err = el("p", "field-error", "");
        wrap.append(label, input, help, err);
        host.appendChild(wrap);
        state.numInputs[f.key] = { input: input, err: err, wrap: wrap };
      });
    });
  }

  function markGroup(selector, attr, value) {
    document.querySelectorAll(selector).forEach(function (b) {
      b.toggleAttribute("selected", b.getAttribute(attr) === value);
    });
  }

  function setSwitch(id, on) {
    var sw = $(id);
    sw.checked = !!on;
    sw.toggleAttribute("checked", !!on);
  }

  function applyFormToUi() {
    var f = state.form;
    if (!f) return;
    markGroup("[data-mode]", "data-mode", f.mutation_mode);
    markGroup("[data-interactivity]", "data-interactivity", f.interactivity_mode);
    setSwitch("fRuntimeRefresh", f.runtime_refresh);
    C.NUMERIC.forEach(function (n) {
      var slot = state.numInputs[n.key];
      if (slot) {
        slot.input.value = String(f[n.key]);
        slot.err.textContent = "";
      }
    });
    renderAuthority();
  }

  function readFormFromUi() {
    var f = {
      mutation_mode: state.form.mutation_mode,
      interactivity_mode: state.form.interactivity_mode,
      runtime_refresh: !!$("fRuntimeRefresh").checked
    };
    Object.keys(state.numInputs).forEach(function (k) {
      f[k] = state.numInputs[k].input.value;
    });
    return f;
  }

  function showFieldErrors(errors) {
    Object.keys(state.numInputs).forEach(function (k) {
      var slot = state.numInputs[k];
      slot.err.textContent = errors[k] || "";
    });
  }

  function renderAuthority() {
    var mode = state.form ? state.form.mutation_mode : "observe";
    var probe = state.probe || {};
    var adaptive = mode === "adaptive";
    setText("authorityTitle", adaptive ? "Adaptive" : "Observe");
    setText("authorityNote", adaptive
      ? "The engine may apply verified changes. Each change is journaled, read back, and rolled back on regression."
      : "Observe is read-only. The engine watches and reasons but changes nothing.");
    setText("authorityGates", "SAFE_MODE: " + (probe.safe ? "present" : "absent")
      + " · DISABLE: " + (probe.disable ? "present" : "absent")
      + " · mutation_armed follows the mode");
  }

  function renderPolicyBanners() {
    var probe = state.probe || {};
    $("pendingNotice").hidden = !probe.pending;
  }

  function loadPolicy() {
    return execShell(C.readConfigCmd(), 8000).then(function (r) {
      state.configLoaded = r.errno === 0 && r.stdout.trim() !== "";
      state.configRaw = r.errno === 0 ? r.stdout : "";
      state.config = C.parseIni(state.configRaw);
      state.form = C.formFromConfig(state.config);
      applyFormToUi();
      setText("policyPath", C.PATHS.config + (state.configLoaded ? "" : " · not readable"));
      if (!state.configLoaded) {
        toast("config.ini is missing or unreadable. Start the module once so the default is created.", true);
      }
    });
  }

  function savePolicy() {
    if (state.busy) return;
    if (!state.configLoaded || !state.form) {
      toast("The configuration is not loaded yet. Reload first.", true);
      return;
    }
    var v = C.validate(readFormFromUi());
    showFieldErrors(v.errors);
    if (!v.ok) {
      toast("Fix the highlighted fields before saving.", true);
      return;
    }
    var probe = state.probe || {};
    var g = C.gate(v.values, {
      currentMode: state.config.mutation_mode === "adaptive" ? "adaptive" : "observe",
      safe: !!probe.safe,
      disable: !!probe.disable
    });
    if (!g.ok) {
      toast(g.reason, true);
      return;
    }
    var go = function () {
      var newRaw = C.applyValues(state.configRaw, v.values);
      state.busy = true;
      return execShell(C.writeConfigCmd(newRaw, v.values, { clearDisable: g.clearDisable }), 15000).then(function (r) {
        if (r.errno === 0 && r.stdout.indexOf("CONFIG_COMMIT_OK") >= 0) {
          state.configRaw = newRaw;
          state.config = C.parseIni(newRaw);
          state.form = C.formFromConfig(state.config);
          applyFormToUi();
          toast("Saved. Restart the engine to apply it.");
        } else {
          toast("Save rejected: " + (r.stderr || r.stdout || "unknown error").trim(), true);
        }
      }).finally(function () {
        state.busy = false;
        pollEngine();
      });
    };
    if (g.confirm) {
      return confirmDialog(g.confirm).then(function (yes) { if (yes) return go(); });
    }
    return go();
  }

  // ---- journal ----

  function loadJournal() {
    return Promise.all([execShell(C.readTraceCmd(), 10000), execShell(C.readLogCmd(), 10000)]).then(function (res) {
      state.trace = res[0].errno === 0 ? C.parseTrace(res[0].stdout, 400) : [];
      state.logText = res[1].errno === 0 && res[1].stdout
        ? res[1].stdout
        : "(No daemon log yet at " + C.PATHS.log + ")";
      renderJournal();
    });
  }

  function renderJournal() {
    var isDecisions = state.journalView === "decisions";
    $("jDecisions").hidden = !isDecisions;
    $("jLog").hidden = isDecisions;
    document.querySelectorAll("[data-jview]").forEach(function (b) {
      b.toggleAttribute("selected", b.getAttribute("data-jview") === state.journalView);
    });
    document.querySelectorAll("[data-jfilter]").forEach(function (b) {
      b.toggleAttribute("selected", b.getAttribute("data-jfilter") === state.traceFilter);
    });

    var rows = C.filterTrace(state.trace, state.traceFilter);
    var host = $("traceList");
    clear(host);
    setText("traceSummary", rows.length + " of " + state.trace.length + " recent decision records, newest first.");
    if (!rows.length) {
      host.appendChild(el("div", "empty", "No matching decisions. The journal lists only records the daemon wrote."));
    }
    rows.slice(0, 200).forEach(function (r) {
      var item = el("article", "trace-item");
      item.setAttribute("data-kind", r.kind);
      var title = (r.event || "event") + (r.result ? " · " + r.result : "");
      var meta = [r.time, r.scope, r.state, r.decision, r.reason].filter(Boolean).join(" · ");
      item.append(el("span", "trace-kind", r.kind.toUpperCase()), el("b", "", title), el("small", "", meta));
      host.appendChild(item);
    });

    var q = String($("logSearch").value || "").toLowerCase();
    var lines = state.logText.split(/\r?\n/).filter(Boolean);
    if (q) lines = lines.filter(function (l) { return l.toLowerCase().indexOf(q) >= 0; });
    $("logBox").textContent = lines.slice(-400).join("\n") || "(no matching lines)";
  }

  function copyText(text) {
    var done = function () { toast("Copied."); };
    var fallback = function () {
      var a = document.createElement("textarea");
      a.value = text;
      a.style.position = "fixed";
      a.style.opacity = "0";
      document.body.appendChild(a);
      a.select();
      try { document.execCommand("copy"); done(); } catch (e) { toast("Copy is unavailable in this WebView.", true); }
      a.remove();
    };
    if (navigator.clipboard && navigator.clipboard.writeText) {
      navigator.clipboard.writeText(text).then(done).catch(fallback);
    } else {
      fallback();
    }
  }

  // ---- startup ----


  function init() {
    document.querySelectorAll("[data-page-link]").forEach(function (b) {
      b.addEventListener("click", function () { showPage(b.getAttribute("data-page-link")); });
    });

    $("btnStart").addEventListener("click", function () {
      runAction(C.startCmd(), { ok: "Start requested." });
    });
    $("btnStop").addEventListener("click", function () {
      runAction(C.stopCmd(), { confirm: "stop" });
    });
    $("btnRestart").addEventListener("click", function () {
      runAction(C.restartCmd(), { confirm: "restart" });
    });
    $("btnRestartPending").addEventListener("click", function () {
      runAction(C.restartCmd(), { confirm: "restart" });
    });
    $("btnRefresh").addEventListener("click", function () {
      runAction(C.refreshCmd(), { ok: "Re-discovery requested." });
    });

    buildPolicyFields();
    document.querySelectorAll("[data-mode]").forEach(function (b) {
      b.addEventListener("click", function () {
        if (!state.form) return;
        state.form.mutation_mode = b.getAttribute("data-mode");
        applyFormToUi();
      });
    });
    document.querySelectorAll("[data-interactivity]").forEach(function (b) {
      b.addEventListener("click", function () {
        if (!state.form) return;
        state.form.interactivity_mode = b.getAttribute("data-interactivity");
        applyFormToUi();
      });
    });
    $("btnSavePolicy").addEventListener("click", savePolicy);
    $("btnReloadPolicy").addEventListener("click", loadPolicy);

    document.querySelectorAll("[data-jview]").forEach(function (b) {
      b.addEventListener("click", function () {
        state.journalView = b.getAttribute("data-jview");
        renderJournal();
      });
    });
    document.querySelectorAll("[data-jfilter]").forEach(function (b) {
      b.addEventListener("click", function () {
        state.traceFilter = b.getAttribute("data-jfilter");
        renderJournal();
      });
    });
    $("logSearch").addEventListener("input", renderJournal);
    $("btnRefreshJournal").addEventListener("click", loadJournal);
    $("btnCopyJournal").addEventListener("click", function () {
      copyText(state.journalView === "log" ? state.logText : $("traceList").textContent);
    });

    $("dialogCancel").addEventListener("click", function () { $("confirmDialog").close("cancel"); });
    $("dialogConfirm").addEventListener("click", function () { $("confirmDialog").close("confirm"); });

    document.addEventListener("visibilitychange", function () {
      if (document.visibilityState === "visible" && state.page === "engine") pollEngine();
      if (document.visibilityState === "hidden") {
        clearTimeout(state.pollTimer);
        state.pollTimer = null;
      }
    });

    showPage("engine");
  }

  if (document.readyState === "loading") document.addEventListener("DOMContentLoaded", init);
  else init();
})();
