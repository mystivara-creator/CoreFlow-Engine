CoreFlow Pulse WebUI
====================

Placement: module/webroot/ (packaged as webroot/ in the installable module ZIP)
Host:      KernelSU WebUI or a compatible host with the ksu.exec bridge
Design:    native HTML, CSS and JavaScript; no web component runtime or CDN

Intent
------
This is a compact, information-dense control and diagnostics surface for coreflowd. It uses
system fonts and native browser controls; it performs no font, image, package or script downloads
when opened. All dashboard rendering is text-only and does not create markup from daemon data.

Pages
-----
- Engine: daemon state, PID/version/status age, device telemetry, decision pipeline, baseline and
  outcome progress, policy/safety state, consultation reason counts, last namespace reasons,
  capability evidence and detailed Eyes/Brain/Hands/device/safety panels.
- Policy: Observe/Adaptive authority, runtime refresh, numeric runtime parameters and interactivity
  settings. Save edits config.ini only and announces when a daemon restart is required.
- Journal: structured decision trace with event filters plus searchable daemon log.
- Project & AI: Mystivara profile links, project principles and development-tool acknowledgements,
  including ChatGPT, Gemini, Perplexity, Grok, Claude and GitHub Copilot.

Control safety
--------------
Every daemon action uses a fixed command builder in control_logic.js. No free-form shell input is
accepted. The UI writes config.ini and the documented restart/kill-switch markers and sends only
fixed lifecycle signals; it does not write kernel tunables. coreflowd remains the authority for
capability checks, policy, bounded writes, verification and rollback.

- Start: launches coreflowd through the module service supervisor.
- Stop: requests supervisor and daemon termination; the daemon restores changes before exit.
- Restart: asks the supervisor to relaunch with saved configuration.
- Re-discover: signals SIGUSR1; it requests another capability scan without writing tunables.
- Observe: sets mutation_mode=observe, mutation_armed=false and creates DISABLE.
- Adaptive: requires explicit confirmation; SAFE_MODE always blocks Adaptive. If DISABLE exists,
  resuming requires separate explicit confirmation.

Performance and packaging
-------------------------
The WebUI has no third-party browser dependencies, external font requests, CDN or build-time
vendor step. Only the following static files are packaged:
  index.html, style.css, app.js, control_logic.js, engine_status.js

Tests
-----
- tools/test_webui_status.js validates engine_status.js against real status output.
- tools/test_webui_control.js checks validation, safety gates, config editing and command builders.
- tools/test_webui_render.js exercises the actual page and app against a simulated bridge/device.
- tools/test_webui_status.sh runs the WebUI contract suite using the host compiler and Node.js.

Project acknowledgements are not claims of endorsement, partnership or identical participation in
every release. The maintainer owns the final implementation and release decisions.
