CoreFlow Autonomous Command Center WebUI
========================================

Placement: module/webroot/
Host: KernelSU WebUI / compatible KSU WebUI provider with the ksu.exec bridge.

Pages
-----
- Live engine (default): what the C++ engine sees (eyes), decides (brain) and changed (hands),
  a four-stage decision pipeline (Context, Policy, Authority, Actuators) and a one-line reason
  when the engine is not acting. Reads /data/adb/coreflow/status.json, which the daemon replaces
  atomically every cycle. Read-only: this page cannot change anything. It polls every 3 s only
  while visible, slows down after repeated failures, and reports a stalled or stopped daemon.
- Overview: daemon/config summary, latest log-derived runtime signals and recent events.
- Mode & policy: explicit Observe / Adaptive selection, separate CPU governor opt-in,
  safe config writes and confirmation dialog.
- Diagnostics: searchable log tail, install report, copy/export actions and capability clues.
- Shell console: curated read-only diagnostic command palette (no free-form root input).
- Project & AI: Mystivara profile, source links, project philosophy and AI acknowledgements.

Safety / runtime behavior
-------------------------
- WebUI never writes kernel tunables directly.
- Observe keeps diagnostics and refresh-discovery interaction available without arming mutation.
- Adaptive writes mutation_mode=adaptive and mutation_armed=true only after explicit confirmation.
  CPU governor changes require separate opt-in (allow_cpu_governor=yes).
- The engine still owns capability discovery, policy authorization, mutation journaling,
  verification, rollback, thermal guards and safety holds.
- A present SAFE_MODE or DISABLE marker blocks enabling Adaptive from the UI.
- The runtime config is /data/adb/coreflow/config.ini; the daemon loads it at initialization,
  so restart/reboot may be required for a new policy to take effect.
- Runtime discovery refresh sends SIGUSR1 to coreflowd; it does not directly change any tunable.
- Shell console commands are fixed/allowlisted and read-only. Do not add arbitrary root command input.
- Missing telemetry stays marked unavailable instead of being replaced with invented values.
- status.json content comes from kernel-provided strings, so the Live page renders it with
  textContent only (never innerHTML), caps its size, and ignores unknown fields.
- engine_status.js (pure logic) and the C++ writer are checked against each other by
  tools/test_webui_status.sh; adding a status field or blocker code on one side fails that test
  until the other side handles it.

AI-assisted development acknowledgement
---------------------------------------
CoreFlow is authored and directed by Mystivara. Gemini, ChatGPT, Grok AI, Claude and GitHub
Copilot are acknowledged as development assistants used for selected tasks such as exploring
alternatives, planning, reasoning, debugging discussion, code completion and documentation.
Their participation varies by task and release; this is not an endorsement or official partnership.
The maintainer reviews and validates changes before release.

Installation / build
--------------------
This folder is copied into the root of the flashable module by the GitHub Actions build workflow.
For a local package, ensure the final module ZIP contains webroot/index.html, webroot/style.css
and webroot/app.js at the module root, beside module.prop and customize.sh.


MODE SWITCHING (OPTION A)
- WebUI mode changes are saved as pending configuration; they do not change the running daemon immediately.
- Restart the module service or reboot to load the selected mode.
- Observe writes the DISABLE safety marker immediately.
- Adaptive keeps DISABLE until startup validates mutation_mode=adaptive, mutation_armed=true, and absence of SAFE_MODE.
- SAFE_MODE is never cleared or bypassed by WebUI.
- While restart is pending, the UI distinguishes requested configuration from effective runtime state.
