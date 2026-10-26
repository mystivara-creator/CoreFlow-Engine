# CoreFlow Engine WebUI — Offline KernelSU / APatch Control Surface

This directory (`module/webroot/`) contains the compiled offline React + TypeScript control surface built from `/webui/` via `npm run build` (`vite build`).

## C++ v3.0.0 Contract Integration
- `control_logic.js` and `engine_status.js` are preserved unmodified from the CoreFlow v3.0.0 C++ contract.
- When opened inside KernelSU or APatch WebUI (`window.ksu.exec`), `app.js` executes only the fixed shell command builders exported by `CoreFlowControl`:
  - `snapshotCmd()`, `splitSnapshot()`, `parseProbe()`
  - `startCmd()`, `stopCmd()`, `restartCmd()`, `refreshCmd()` (`SIGUSR1` capability re-discovery)
  - `readConfigCmd()`, `validate()`, `gate()`, `applyValues()`, `writeConfigCmd()`
  - `readTraceCmd()`, `parseTrace()`, `filterTrace()`, `describeEvent()`, `readLogCmd()`
- Live `/data/adb/coreflow/status.json` snapshots written by `src/runtime/status_snapshot.cpp` are parsed via `CoreFlowStatus.parse()`, `summary()`, and `pipeline()`.
- When opened in a standard browser without `window.ksu` (such as development preview on port 3000), the UI runs an interactive C++ v3.0.0 Mirror responder routed through the exact same `CoreFlowControl` and `CoreFlowStatus` parsers.
- Zero external runtime CDNs are used; all assets (`index.html`, `app.js`, `style.css`, `control_logic.js`, `engine_status.js`) resolve locally from `./`.
