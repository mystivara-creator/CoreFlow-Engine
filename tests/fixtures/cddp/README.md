# CDDP Synthetic Fixtures A-J

Fixtures are materialized into isolated temporary sysfs/procfs trees by `tests/cddp_fixture_factory.hpp`; they require neither root nor a real Android device.

| Fixture | Surface | Expected outcome |
|---|---|---|
| A | Valid CPUFreq, memory, PSI, cgroup v2, thermal | Eligibility and apply/readback/restore path |
| B | Non-contiguous CPU membership | Exact CPU IDs retained |
| C | Malformed online CPU set | Topology invalid; mutation rejected |
| D | Writable CPUFreq node but no advertised table | Semantics known; constraints unverified |
| E | Read-only CPUFreq max node | Remains READABLE even under uid 0 |
| F | Contradictory CPUFreq bounds | Constraints verification rejected |
| G | Generic devfreq bus | Not presumed to be GPU |
| H | Recognized Mali devfreq | GPU max capability verified in Hz |
| I | Qualcomm KGSL-like nodes | Observation-only |
| J | Sparse system tree | Missing capabilities remain unavailable |

The E2E test claims verified actuation and restoration only. It does not claim performance improvement without comparable runtime telemetry.