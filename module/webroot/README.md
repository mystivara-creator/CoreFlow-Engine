# CoreFlow Material 3 — CI-vendored upstream + local fallback

This package replaces the existing `module/webroot/` presentation with the Material-inspired six-page dashboard.

## Integration notes
- `control_logic.js` and `engine_status.js` are copied byte-for-byte from the supplied current CoreFlow source archive.
- `app.js` uses `CoreFlowControl.snapshotCmd()`, `splitSnapshot()`, `parseProbe()`, `refreshCmd()`, `readConfigCmd()`, `gate()`, `validate()`, `applyValues()`, and `writeConfigCmd()`; it does not use guessed bridge APIs.
- Start, stop, restart, and re-discovery actions use only the fixed command builders exported by `CoreFlowControl`.
- Policy mode changes read `config.ini`, validate the current form, pass through `gate()`, and use the atomic config writer. Observe remains the default fallback. Adaptive remains subject to SAFE_MODE/DISABLE and daemon-side gates.
- The status display uses `CoreFlowStatus.parse()` and its status helpers. Unknown or absent telemetry remains unknown.
- The current status schema exposes aggregate capability counts, not per-resource records. The resource page explicitly says so rather than fabricating per-resource inventory.
- Journal data comes from the existing trace and log readers. Device/runtime strings are escaped before display.

## Upstream tracking and offline runtime
The Android CI workflow runs `tools/vendor_webui_material.sh` before packaging. It resolves the selected upstream ref (default `main`) from `https://github.com/material-esm/material`, copies the Material Web modules and Lit runtime dependencies into `module/webroot/vendor/`, and records the exact resolved commit and upstream package version in `vendor/material-build-info.json`.

This means the upstream ref is dynamic between CI builds, while each built artifact records the immutable commit it actually contains. The installed WebUI loads Material and Lit from local files and does not require a CDN at runtime. If the vendor tree is missing or incomplete during development, the built-in CSS fallback remains available.

The script accepts `COREFLOW_MATERIAL_REF` for controlled builds; its default is `main`. Review upstream changes and run the UI contract tests before distributing a production build; a moving upstream branch can introduce breaking changes.

## Validation
## Contract validation
Run `sh tools/test_webui_status.sh`, `node --check module/webroot/app.js`, and `git diff --check` before committing. The CI workflow runs the WebUI contract tests and verifies that the pinned Material Web components and Lit runtime are present in the final packaged artifact. These checks validate the repository's declared contracts and file presence; they do not replace an actual browser test on the target Android WebView.

## Apply safely
Back up `module/webroot/` first. Review the package diff and test in Observe mode. Do not set Adaptive or change mutation permissions as part of UI deployment.
