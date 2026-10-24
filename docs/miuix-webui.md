# CoreFlow Console (WebUI) — v3.1.0

The console is `module/webroot/index.html`. It is self-contained, mobile-first, and offers English (default) and Indonesian.

## Data path

The page does not open a network port and does not run a server. It uses the KernelSU WebUI bridge (`ksu.exec(command, optionsJson, callbackName)`), which runs commands as root and returns `(errno, stdout, stderr)` through the callback.

Reads (fixed commands, fixed paths under `/data/adb/coreflow`):

- `cat status.json`
- `cat capabilities.json`
- `tail -n 200 journal.jsonl`

Writes (fixed tokens only):

- `printf '%s\n' '<token>' > '/data/adb/coreflow/command'`, where `<token>` is one of `pause`, `resume`, `rescan`, `reset_learning`.

The daemon accepts only those tokens and deletes the command file after reading it.

## Status freshness

`status.json` carries `generated_unix` and `stale_after_s`. If the status is older than `stale_after_s`, the console reports the daemon as not reporting and disables the control buttons. A status without `stale_after_s` cannot be judged and is reported as an incompatible schema. A missing file is shown as "no state file yet".

## status.json

```json
{
  "schema": 1, "generated_unix": 1700000000,
  "engine_state": "running|paused|starting", "stage": "outcome",
  "uptime_s": 3725, "stale_after_s": 60, "cycles": 412, "workload": "cpu+io",
  "device": {"model": "...", "soc": "...", "android": "16", "kernel": "...", "storage": "UFS"},
  "inventory": {"discovered": 6, "eligible": 4, "catalog": "configured"},
  "counters": {"retained": 7, "rolled_back": 3, "deferred": 12, "unresolved": 1},
  "system": {
    "cpu_busy_percent": 63.4, "memory_available_ratio": 0.3721,
    "max_temperature_c": 58.2, "thermal_headroom_c": 27.8,
    "cpu_psi_some_avg10": 4.2, "memory_psi_some_avg10": 0.6, "io_psi_some_avg10": 2.1,
    "frame_average_fps": 59.6, "frame_p95_ms": 18.1, "frame_p99_ms": 21.4,
    "slow_frame_ratio": 0.012, "frame_package": "com.example.game"
  },
  "last_error": ""
}
```

Any measurement the daemon could not observe is `null`, never a fabricated value.

## Pipeline stages

The console highlights the stage named in `stage`. Stage names match the C++ functions: `command`, `discovery`, `observation`, `optimization`, `justification`, `validation`, `mutation`, `settling`, `outcome`, `rollback`, `learning`, `idle`, `paused`.

## Security posture

- No network listener; the console is reachable only through the root manager.
- The page renders device-derived strings with `textContent`, never `innerHTML`.
- Command tokens are whitelisted in the page and validated again by the daemon.
- The state directory is created with mode `0700`.
