# CoreFlow v3.1.0 — native maintenance audit

This maintenance pass follows the recovery heartbeat fix. It reviews native
reasoning, resource state, vendor discovery/parsing, telemetry, persistence and
bounded subprocess execution. It also checks the existing actuator/journal gates
and module/WebUI contracts. Findings below are concrete source defects or avoidable
work; this report is not a claim that every possible runtime failure is eliminated.

## Findings and changes

| Area | Finding | Change |
| --- | --- | --- |
| Resource state | String/vector allocation inside `noexcept` methods could terminate the daemon. Adding a blocking constraint could leave an already-granted advisory permission set. | Catch failures, mark the record recovery-required and revoke permission. Failed reset leaves an empty model. Blocking constraints immediately revoke the flag. |
| Reasoning | Ranking, proposal formatting, agent construction and frame-interval collection allocated outside an exception boundary despite `noexcept`. | Return an invalid proposal, empty ranking or unavailable trace on allocation failure. No failure creates a mutation permit. |
| Capability classification | Name/path lowercase copies were rebuilt for each of 15 domain rules, including thousands of discovered resources. Strategy matching also copied strings per candidate. | Shared allocation-free ASCII matching on `string_view`. An allocation-count regression verifies one output-vector allocation for capability assessment. |
| Ranking | Every result was sorted even when only a small prefix was requested; a zero-result request still traversed the matrix. | Select/sort the requested prefix with `partial_sort`; return immediately for zero. Verify top-result equivalence against complete ranking. |
| Strategy evidence | Frame-jank numbers could influence gaming classification while their availability flag was false. CPU-list counting could overflow or accept broken/overlapping ranges. | Require available frame evidence; parse complete unsigned ranges and reject overflow, overlap or malformed tails. Parse scalar priors strictly without wrapping. |
| Numeric parsing | Effect-model and fatal-value integer parsing copied strings and used throwing conversion on candidate paths. Sensor integer parsing accepted a valid prefix followed by garbage. | Share trimmed views and `from_chars` in both paths; require the full sensor integer token. |
| Vendor JSON | A valid object followed by garbage was accepted; unknown escapes and Unicode escapes could silently change text; malformed scalar tokens/duplicate keys were accepted. | Require complete input, validate JSON scalar grammar and escapes, decode Unicode/surrogates, reject ambiguous keys and control bytes in tunable paths/values. |
| Power-hint semantics | Actions could attribute undeclared values to a kernel node; default-index parsing accepted partial integers. | Match string-valued node actions against declared `Nodes.Values`, recognize only node action types, and parse the full numeric default index. |
| Vendor I/O | A pre-read `stat` size did not bound actual bytes read if a file grew. Directory scanning had a successful-file limit but no entry budget. | Reuse the bounded regular-file reader on the resolved path; cap parser input at 1 MiB and directory enumeration at 16,384 entries. |
| Learned memory | The new bandit variance was not serialized; default stream precision also changed learned values across restart. | Write experience V3 with round-trip double precision and variance. Read V2 using conservative variance 0.02. Invalid variance fails the load while preserving current memory. |
| Subprocesses | Output growth could allocate after acquiring a child and pipe; embedded NUL arguments were silently truncated by `execv`; redirection errors were ignored. | Reserve capped output before creating resources, reject NUL arguments, check descriptor setup and clear stdout's close-on-exec flag even when descriptor numbers coincide. |
| GPU discovery | Recursive directory handles could leak if constructing a path threw. | Own directory handles with RAII during the walk. |

Power-hint value membership follows the node-action parsing in Android's
[libperfmgr HintManager](https://android.googlesource.com/platform/hardware/google/pixel/+/refs/heads/main/power-libperfmgr/libperfmgr/HintManager.cc).
Vendor profiles remain priors, not independent proof of safety or write authority.

## Verification

- All 47 host-compatible core sources compile with strict C++20 warnings as errors;
  all 27 host test executables run, including allocation-failure injection.
- Added regressions cover malformed/escaped JSON, undeclared hint values, NUL
  arguments, closed-stdout timeouts, invalid CPU lists, stale frame evidence,
  top-ranking equivalence, blocking constraints and V2/V3 memory reload.
- Focused ASan + UBSan runs cover allocation failure, bounded processes, vendor
  parsing, experience persistence, strategies and resource ranking. Local leak
  detection is disabled because of the traced execution environment; CI's leak
  detection remains enabled.
- The offline WebUI build and 384 WebUI checks remain part of validation. Source
  release, safety and installer checks verify packaging and existing contracts.
- README local links and the authored SVG banner's XML are checked. The document
  uses CoreFlow's own wording, version, capability scope and MIT licensing.

## Boundaries

No kernel control value, grant or thermal threshold is widened by this pass.
Existing descriptor checks, journals, ownership handoffs, single-epoch attribution
and rollback remain the write/recovery gates. The source version remains v3.1.0.

The entry budget can stop vendor harvesting before every optional file is reached;
missing priors do not grant permission. V3 experience is readable by this version;
older binaries that only understand V2 may decline the advisory database after a
downgrade. Recovery journals and lifetime sample records use their existing formats.

These are local source and host checks, not Android hardware performance numbers,
a device soak test or an Android NDK build in this environment. Consult Actions for
this commit's Android result and validate on-device recovery and workload behaviour
before claiming a binary release is field-validated.
