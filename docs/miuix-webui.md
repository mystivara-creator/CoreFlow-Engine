# MIUIX WebUI — v3.0.0

The WebUI is packaged at `module/webroot/index.html` and is mobile-first, self-contained, and has English as its default language with an Indonesian language toggle. The About tab identifies Mystivara and links to:

- Developer profile: https://github.com/mystivara-creator
- Development repository: https://github.com/mystivara-creator/CoreFlow-Engine

## Current integration boundary

This patch adds the UI assets and includes them in the module package. It does **not** implement an HTTP server in `coreflow_daemon`. The page therefore explicitly displays a disconnected state until a same-origin local API exists. It does not invent telemetry or display sample metrics as real measurements, and it contains no kernel mutation controls.

The UI currently expects:
- `GET /api/status`
- `GET /api/capabilities`
- `GET /api/decisions`
- `GET /api/mutations`

Expected JSON shape:
- status: `{ "engine_state": "...", "mode": "...", "cpu_load": "...", "memory_available": "...", "hottest_sensor": "...", "capability_count": 0, "profile_status": "...", "uptime": "...", "subsystems": [], "events": [] }`
- capability list: `{ "items": [{ "path": "...", "domain": "...", "access": "...", "semantic_status": "...", "strategy_id": "...", "eligibility": "..." }] }`
- decision list: `{ "items": [{ "time": "...", "candidate": "...", "decision": "...", "reason": "..." }] }`
- mutation list: `{ "items": [{ "time": "...", "resource": "...", "before": "...", "after": "...", "verification": "...", "outcome": "...", "recovery": "..." }] }`

The local API must be bound to loopback, validate requests, expose read-only data by default, and avoid exposing a privileged unauthenticated mutation endpoint. Do not expose the UI/API to a LAN interface without a separately reviewed security design.
