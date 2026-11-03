# CoreFlow Modern v2 offline WebUI

This directory is the built offline React dashboard packaged with the module.

The native bridge uses only coreflowd --status, --stop, --rollback-baseline, the persistent CoreFlow config, and the bounded daemon log. Native controls represent the same four-state stability model as the C++ daemon.

The browser-only preview is explicitly simulated. Modern v2 does not claim native FPS/frame-pacing telemetry, package-specific game detection, vendor unlocks, or arbitrary sysfs mutation.
