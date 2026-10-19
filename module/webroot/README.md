# CoreFlow Material 3 CDN + local fallback — integrated package

This package replaces the existing `module/webroot/` presentation with the Material-inspired six-page dashboard.

## Integration notes
- `control_logic.js` and `engine_status.js` are copied byte-for-byte from the supplied current CoreFlow source archive.
- `app.js` uses `CoreFlowControl.snapshotCmd()`, `splitSnapshot()`, `parseProbe()`, `refreshCmd()`, `readConfigCmd()`, `gate()`, `validate()`, `applyValues()`, and `writeConfigCmd()`; it does not use guessed bridge APIs.
- Start, stop, restart, and re-discovery actions use only the fixed command builders exported by `CoreFlowControl`.
- Policy mode changes read `config.ini`, validate the current form, pass through `gate()`, and use the atomic config writer. Observe remains the default fallback. Adaptive remains subject to SAFE_MODE/DISABLE and daemon-side gates.
- The status display uses `CoreFlowStatus.parse()` and its status helpers. Unknown or absent telemetry remains unknown.
- The current status schema exposes aggregate capability counts, not per-resource records. The resource page explicitly says so rather than fabricating per-resource inventory.
- Journal data comes from the existing trace and log readers. Device/runtime strings are escaped before display.

## CDN fallback
The HTML, CSS, and core behavior are local. CDN imports are optional; a failure sets the local fallback status and does not prevent the UI from operating. Internet access is only needed for the optional Material enhancement.

## Validation
The JavaScript files pass `node --check`. The existing control contract test passes 69/69 checks against the supplied source. The existing render/status tests target the original page structure and therefore are not evidence that this redesigned DOM has passed those tests; run them again after merging and update their fixture/DOM contract as needed.

## Apply safely
Back up `module/webroot/` first. Review the package diff and test in Observe mode. Do not set Adaptive or change mutation permissions as part of UI deployment.
