# Capability-Driven Discovery and Resource Eligibility

CoreFlow discovers interfaces from mounted filesystem roots and records evidence about their existence, readability, writability, and semantics. Discovery is bounded by traversal depth, visit budget, and filesystem boundaries; symlinks are not followed. A truncated inventory must be reported as incomplete rather than treated as exhaustive.

## Candidate admission

A writable interface is only a candidate when its subsystem is recognized by location evidence or explicitly described by vendor configuration. Unclaimed writable nodes should be recorded for diagnostics, not automatically mutated. Candidate value generation should rely on vendor configuration, kernel-advertised options, ordered scales, or a conservative bounded step appropriate to the interface semantics.

## Eligibility is not authorization

Before any write, the resource must pass all relevant checks:

- capability evidence is current and the interface's semantics are understood;
- the interface is not in a fatal or quarantined class;
- the candidate value is valid and bounded for the discovered interface;
- runtime context and safety holds permit the action;
- policy/consultation grants authority and a valid mutation permit is present;
- the pre-write state is journaled;
- read-back verification confirms the intended state, with rollback available on failure.

CPUFreq governor control follows the global Adaptive/armed authority; the legacy `allow_cpu_governor` key is ignored. Block-queue controls are not globally excluded merely because they are block-queue controls; each candidate still needs capability evidence, a suitable value, policy approval, and all safety gates. Unknown semantics remain observe-only.

## Important limitation

Kernel interfaces vary by device, vendor, Android version, and kernel configuration. A path's presence or writable bit does not prove that a particular value is beneficial or safe. Test changes on a specific device only after host validation and with recovery available.
