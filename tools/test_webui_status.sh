#!/bin/sh
# Builds real status snapshots from the C++ code and runs the WebUI contract test.
set -eu
ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
if ! command -v node >/dev/null 2>&1; then echo "SKIP: node not installed; WebUI status contract NOT verified"; exit 0; fi
if ! command -v g++ >/dev/null 2>&1; then echo "SKIP: g++ not installed; WebUI status contract NOT verified"; exit 0; fi
TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT
g++ -std=c++17 -Wall -Wextra -Werror -I"$ROOT/include" \
    "$ROOT/tests/status_dump.cpp" "$ROOT/src/runtime/status_snapshot.cpp" -o "$TMP/status_dump"
mkdir -p "$TMP/out"
"$TMP/status_dump" "$TMP/out"
cat > "$TMP/out/decision_trace.jsonl" <<'TRACE_EOF'
{"schema":"coreflow.event.v2","session_id":"host-test-1","event_seq":1,"timestamp":"2026-10-17T12:00:00Z","cycle":7,"event":"scope_gate_checked","scope":"cpu_freq_coarse","result":"DENY","reason":"cpu_governor_opt_in_disabled","decision_reason":"NO_VERIFIED_MUTATION","previous_state":"Normal","state":"Normal","decision":"NoAction","confidence":0.9,"workload":"interactive","context_confidence":0.9,"mutation_eligible":true,"stabilizing_only":false,"candidate_count":1,"plan_action":"Candidate","intervention":1,"plan_reason":"test plan","safety_hold":"NONE","mode":"observe","mutation_armed":false,"allow_cpu_governor":false,"permit_valid":false,"failure_stage":"","actuator_status":"","cpu_mutation":"SKIPPED","resource_mutation":"SKIPPED"}
{"schema":"coreflow.event.v2","session_id":"host-test-1","event_seq":2,"timestamp":"2026-10-17T12:00:01Z","cycle":7,"event":"actuator_apply_completed","scope":"resource","result":"OK","reason":"actuator_return_reports_write_and_readback_counts","decision_reason":"NO_VERIFIED_MUTATION","previous_state":"Normal","state":"Normal","decision":"NoAction","confidence":0.9,"workload":"interactive","context_confidence":0.9,"mutation_eligible":true,"stabilizing_only":false,"candidate_count":1,"plan_action":"Candidate","intervention":1,"plan_reason":"test plan","safety_hold":"NONE","mode":"adaptive","mutation_armed":true,"allow_cpu_governor":false,"permit_valid":true,"failure_stage":"","actuator_status":"OK","operation_id":"host-test-1:7:resource:/sys/test/tunable","target":"/sys/test/tunable","requested":"64","current_value":"32","baseline_value":"32","writes_attempted":1,"writes_verified":1,"cpu_mutation":"SKIPPED","resource_mutation":"VERIFIED"}
{"schema":"coreflow.decision.v1","timestamp":"2026-10-17T11:59:59Z","cycle":6,"previous_state":"Normal","state":"Normal","decision":"NoAction","confidence":0.9,"workload":"interactive","context_confidence":0.9,"mutation_eligible":false,"stabilizing_only":false,"candidate_count":0,"plan_action":"Observe","intervention":0,"plan_reason":"legacy fixture","safety_hold":"NONE","mode":"observe","mutation_armed":false,"allow_cpu_governor":false,"cpu_mutation":"SKIPPED","resource_mutation":"SKIPPED","reason":"POLICY_BLOCKED"}
TRACE_EOF
node "$ROOT/tools/test_webui_status.js" "$TMP/out"
node "$ROOT/tools/test_webui_render.js" "$TMP/out"
node "$ROOT/tools/test_webui_control.js"
