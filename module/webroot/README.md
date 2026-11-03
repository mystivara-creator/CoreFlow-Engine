# CoreFlow Modern v2 offline WebUI

`npm run build` compiles the React/TypeScript dashboard into this directory using Vite. The production module bundles `index.html`, `app.js`, `style.css` and theme assets.

The native bridge runs fixed `coreflowd --status`, `--stop` and `--rollback-baseline` commands and can restart the module service. It does not use the retired v3 `CoreFlowControl`, `CoreFlowStatus` or `status.json` APIs.

Without the bridge this is an explicitly labelled interactive preview. Native mode displays only supported data: measured cgroup clamp values, typed CPU/SoC thermal sensor, status and daemon logs. FPS, jank and top-app metrics are unavailable until a validated platform telemetry interface is integrated.
