/* CoreFlow Control: a control surface for the coreflowd C++ daemon.
 *
 * - Every action runs one fixed command from control_logic.js through the KernelSU exec bridge.
 * - The daemon is the only authority over kernel writes. The UI edits config.ini, touches the
 *   documented marker files, sends SIGTERM/SIGUSR1, and asks service.sh to (re)start coreflowd.
 * - Rendering uses textContent only. status.json and the journal come from the kernel and the
 *   daemon, so they are treated as data, never as markup.
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

  // ---- Live engine view: renders only what the daemon wrote and the probe observed ----

  function renderEngine() {
    var p = state.status;
    var probe = state.probe;
    var online = !!(probe && probe.running);
    var sum = p ? S.summary(p) : { tone: "info", title: "Reading daemon state", detail: "Waiting for the first probe.", code: "" };

    var hero = $("heroCard");
    hero.setAttribute("data-tone", sum.tone);
    setText("heroCode", sum.code || "—");
    setText("heroTitle", sum.title);
    setText("heroDetail", sum.detail);

    var chip = $("headerChip");
    if (state.bridge === false) {
      chip.setAttribute("data-tone", "bad");
      chip.textContent = "NO BRIDGE";
    } else if (!probe) {
      chip.setAttribute("data-tone", "info");
      chip.textContent = "CHECKING";
    } else {
      var tone = !online ? "bad" : sum.tone === "bad" ? "bad" : sum.tone === "warn" ? "warn" : "ok";
      chip.setAttribute("data-tone", tone);
      chip.textContent = online ? "ONLINE" : "OFFLINE";
    }
    setText("headerSub", "coreflowd" + (online ? " · pid " + probe.pids.join(" ") : probe ? " · not running" : " · connecting"));

    var ok = !!(p && p.ok);
    setText("metaPid", online ? probe.pids.join(", ") : "—");
    setText("metaVersion", ok && p.data.version ? p.data.version : "—");
    setText("metaUpdated", ok ? (p.age === 0 ? "just now" : p.age + " s ago") : "—");

    var pipe = $("pipeline");
    clear(pipe);
    if (ok) {
      S.pipeline(p.data).forEach(function (stage, i) {
        var row = el("div", "stage");
        row.setAttribute("data-tone", stage.tone);
        var body = el("div");
        body.appendChild(el("b", "", stage.label));
        body.appendChild(el("p", "", stage.note));
        row.append(el("span", "stage-index", String(i + 1)), body, el("span", "state", stage.state));
        pipe.appendChild(row);
      });
    } else {
      pipe.appendChild(el("p", "empty", "The pipeline appears once the daemon writes its first status."));
    }

    fillRows("eyesRows", ok ? S.eyesRows(p.data) : []);
    fillRows("brainRows", ok ? S.brainRows(p.data) : []);
    fillRows("handsRows", ok ? S.handsRows(p.data) : []);
    fillRows("deviceRows", ok ? S.deviceRows(p.data) : []);
    fillRows("safetyRows", probe ? [
      ["SAFE_MODE", probe.safe ? "present" : "absent", probe.safe ? "Adaptive is blocked" : ""],
      ["DISABLE kill switch", probe.disable ? "present" : "absent", probe.disable ? "mutation held" : ""],
      ["Config waiting for restart", probe.pending ? "yes" : "no", ""],
      ["Boot attempts", probe.bootAttempts || "0", "unconfirmed boots"]
    ] : []);

    var blocked = state.bridge === false;
    $("bridgeNotice").hidden = !blocked;
    var idle = !state.busy && !blocked;
    $("btnStart").disabled = !idle || online;
    $("btnStop").disabled = !idle || !online;
    $("btnRestart").disabled = !idle;
    $("btnRefresh").disabled = !idle || !online;

    setText("engineFooter", ok && p.stale
      ? "The daemon has not written status recently. It may be stalled."
      : "Polls every 3 s while this page is visible. The daemon writes status.json; the UI only reads it.");
  }

  function fillRows(id, rows) {
    var host = $(id);
    clear(host);
    if (!rows || !rows.length) {
      host.appendChild(el("div", "empty", "No data yet."));
      return;
    }
    rows.forEach(function (r) {
      var row = el("div");
      row.append(el("dt", "", r[0]), el("dd", "", r[1]));
      if (r[2]) row.appendChild(el("small", "", r[2]));
      host.appendChild(row);
    });
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
        var input = document.createElement("md-text-field");
        input.setAttribute("label", f.label + (f.unit ? " (" + f.unit + ")" : ""));
        input.setAttribute("color", "outlined");
        input.setAttribute("inputmode", f.kind === "int" ? "numeric" : "decimal");
        input.setAttribute("aria-label", f.label);
        var help = el("p", "field-help", f.help + " Range: " + rangeText(f) + ".");
        var err = el("p", "field-error", "");
        wrap.append(input, help, err);
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
    sw.selected = !!on;
    sw.toggleAttribute("selected", !!on);
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
      runtime_refresh: !!$("fRuntimeRefresh").selected
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

  function checkVendor() {
    var ready = !!(window.customElements && customElements.get("md-button") && customElements.get("md-switch"));
    $("vendorNotice").hidden = ready;
  }

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

    setTimeout(checkVendor, 2500);
    showPage("engine");
  }

  if (document.readyState === "loading") document.addEventListener("DOMContentLoaded", init);
  else init();
})();
