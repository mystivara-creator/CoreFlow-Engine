CoreFlow Pulse WebUI
====================

Placement: module/webroot/ (packaged as webroot/ in the installable module ZIP)
Host:      KernelSU WebUI or a compatible host with the ksu.exec bridge
Design:    Material 3 tokens in style.css; optional Material Web modules from a CDN

Intent
------
This is a compact, information-dense control and diagnostics surface for coreflowd. Styling is
local (Material 3 colour roles, shape and state layers). When the CDN is reachable, three optional
Material Web modules are loaded; each loads independently, and when none loads the UI keeps working
with local styles only (footer shows "Local fallback active"). All dashboard rendering is
text-only and does not create markup from daemon data.

Pages
-----
- Home: engine state, mutation authority, live signals (thermal, CPU, memory, battery) and a
  short decision pipeline.
- Resources: aggregate capability counts. The status contract does not expose per-resource rows,
  so none are shown or invented.
- Decisions: latest decision snapshot, consultation counts and the event timeline of the last cycle.
- Policy: Observe or Adaptive posture (saved through config.ini with the normal gates) and the
  safety gates the daemon reported.
- Journal: runtime trace, newest first. Each cycle reads bottom to top: policy_evaluated,
  scope_gate_checked, target_permit_checked, prewrite_read_completed, mutation_journal_commit,
  actuator_apply_started, actuator_apply_completed, mutation_controller_returned,
  observation_window_started or mutation_held, cycle_completed. Each row shows the target path,
  the value written ("Set to"), the value before the write ("Before") and the baseline.
- System: device profile, kernel, SoC and interface availability.
- About: project information and what the actuator is allowed to change.

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
The WebUI has no build-time vendor step. Optional runtime dependency: three Material Web modules
from a CDN (see the Intent section). Everything else is local. Packaged static files:
  index.html, style.css, material-fallback.css, app.js, control_logic.js, engine_status.js

Tests
-----
- tools/test_webui_status.js validates engine_status.js against real status output.
- tools/test_webui_control.js checks validation, safety gates, config editing and command builders.
- tools/test_webui_render.js exercises the actual page and app against a simulated bridge/device.
- tools/test_webui_status.sh runs the WebUI contract suite using the host compiler and Node.js.

Project acknowledgements are not claims of endorsement, partnership or identical participation in
every release. The maintainer owns the final implementation and release decisions.
