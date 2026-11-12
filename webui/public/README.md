# CFAE System Framework v3.1.0 WebUI Public Assets

Vite copies these static assets into `module/webroot` during build so both standard web preview deployments (`dist/`) and KernelSU / APatch / Magisk offline packaging remain 100% identical and synchronized.

Native mode is capability-driven and read-only for system policy: the dashboard explains `coreflowd --status`, thermal and PSI telemetry, semantic evidence, ownership journal state, and daemon audit decisions. The daemon owns CPU, memory, storage, thermal, and longevity adaptation. The sole manual runtime opt-in is Vulkan diagnostic/pass-through layer mode. Browser mode is an explicitly labelled simulation of the Coordinated Mutation Safety architecture.

Architecture Studio remains available as an in-app learning environment for the engine model, mutation safety, risk provenance, thermal simulation, and source-level architecture. It does not apply kernel policy.

Unverified metrics such as synthetic FPS, frame-time, or jank are never claimed without a validated platform source.

CF-DVS is exposed in the WebUI as a read-only session-state and validation-coverage view. Session creation/finalization remains owned by the native runtime lifecycle; the WebUI does not expose start/stop/profile controls. The public `CoreFlow` JavaScript bridge intentionally exposes status and capability queries only, not arbitrary shell execution or policy/lifecycle mutators.
