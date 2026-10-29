# Workload, rendering and launch control

Baseline: published 3.0.2 source at
5a251588cd70d6ed1ec5309ef7333325c0241837.

## Implemented behavior

ForegroundTracker resolves a nonempty, terminated process identity before
committing a transition. Missing, empty and truncated cmdline data remains
retryable. EINTR and allocation failures cannot terminate optional telemetry.
A known member is retained across cgroup row reordering. Empty membership clears
identity; changed cmdline is detected even when a PID is reused. Membership is
bounded to 4096 bytes; larger inputs are unavailable rather than partially trusted.
The identity is a process heuristic, not an activity lifecycle or launch-time metric.

Foreground transitions cancel pending CPU/resource trials without learning a
benefit or regression, release temporary boosts, and restart measurement baselines.
Accepted settings stay owned. Incomplete restoration uses the existing recovery
retry path and prevents another mutation epoch.

FrameTelemetrySource selects a unique package-qualified layer from the bounded
SurfaceFlinger --list result. --latency receives that exact layer as one direct
exec argument. Missing or ambiguous surfaces produce an unavailable reason;
there is no fallback to unrelated global frames. Surface discovery and collection
share a one-child-per-poll budget and use time/output limits. Discovery retries
are rate limited. Changed foreground, refresh period, failed collection and
surface replacement start fresh history epochs. Initial history is warmup and
repeated timestamps contribute no new evidence. Empty collection invalidates
frame-analysis history, preventing stale jank from triggering a new boost.

The legacy latency parser now shares the actual-presentation interval parser.
AOSP columns are desired present, actual present and frame ready; subtracting
actual from ready was not a frame-duration measurement.

InteractivityPolicy keeps the short CPU launch hint, caps its duration to both
the configured boost window and remaining launch deadline, and issues no expired
launch intent. Launch GPU assistance requires measured high GPU utilization.
Launch read-ahead requires I/O pressure with low memory pressure. Frame-disturbance
CPU assistance requires CPU pressure; GPU assistance requires high GPU utilization.
Missing frame evidence cannot authorize a stutter boost. Domain permits, target
admission, journaling, verification, thermal holds and boost expiry still apply.
This evidence does not establish application-level causal benefit; read-ahead
in particular is not proof that the workload is sequential.

A busy GPU is no longer classified as a verified game. Generic GPU/render
strategies continue to use the pressure signals without claiming game identity.
Idle operation invalidates interaction history and skips expensive collectors.

## User-visible status

The status JSON and WebUI distinguish foreground availability, process identity,
selected surface, collector reason and interaction state. Rendering is explicitly
a presentation-interval versus display-period proxy. A deliberate 30 FPS workload
on a 60 Hz display can exceed that period without being defective. The app's own
target FPS, CPU/GPU render duration and input-to-display latency remain unknown.
The metric is not a universal app jank or launch-completion measurement.

## Framework and driver boundaries

| Area | Implemented decision |
|---|---|
| GPU/CPU/I/O | Existing discovered, bounded actuators with evidence-filtered assistance |
| Vulkan/renderer | Preserve the app and platform renderer choice; no forced global property override |
| Power HAL | Existing vendor awareness and ownership-continuity behavior; no new Binder backend |
| ART | Platform-managed compilation/background maintenance; no launch-time forced compilation |
| ADPF | Application integration required; no fabricated cross-app hint sessions |
| Swappy | Application integration required; no injected game rendering library |

These boundaries are exposed rather than represented as successful optimizations.
ART, ADPF, Swappy or a replacement GPU driver are not newly implemented backends.
They require separate integration contracts and device evidence. Root access does
not supply the application's work duration or desired frame rate.

## Validation and device acceptance

Host coverage includes process-read failures, empty/truncated identity, retry,
cgroup reordering, identity replacement, ambiguous/prefix-collision layer names,
collector budget and rate cap, history warmup, deduplication, failed collection,
strict refresh headers, pressure filtering and exclusive launch deadline.
The collector runner is injected in host tests; production uses the bounded
process implementation. Android orchestration is separately syntax checked,
which does not replace an Android NDK link/build.

Before publication, run Android CI and compare the same workloads on device:
cold/warm launch, scrolling, System UI transitions and a sustained game session.
Capture foreground/surface availability, presentation distributions, temperature,
energy and collector overhead. Verify app switching never mixes trial outcomes;
missing permission, layer replacement, hold and restore failure must be exercised.
No performance superiority, thermal reduction or driver optimization is claimed
from host tests alone. Main-loop polling bounds reaction speed; this is not a new
Android activity/display callback backend.

## Primary references

- https://android.googlesource.com/platform/frameworks/native/+/refs/heads/android15-release/services/surfaceflinger/SurfaceFlinger.cpp
- https://source.android.com/docs/core/runtime/configure/art-service
- https://developer.android.com/reference/android/os/PerformanceHintManager.Session
- https://developer.android.com/games/sdk/frame-pacing/

## Local validation result

- Host suite: 27/27 passed.
- ASan/UBSan configuration: 27/27 passed; leak detection disabled in this environment.
- WebUI contracts: 224 status, 70 control and 14 render checks passed.
- Installer: 11 scenarios passed, including daemon configuration-parser validation.
- Frontend production build, safety foundation, strict Android-orchestration host
  syntax check and diff whitespace checks passed.
- New Android NDK linking and device acceptance remain pending for this source.
