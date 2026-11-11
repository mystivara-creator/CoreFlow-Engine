#!/usr/bin/env python3
"""Guard read-only CLI and deterministic composition-root lifecycle contracts."""
from pathlib import Path

source = (Path(__file__).resolve().parents[1] / "src/daemon/main.cpp").read_text()
config = (Path(__file__).resolve().parents[1] / "include/coreflow/config.hpp").read_text()
context = (Path(__file__).resolve().parents[1] / "src/kernel/coreflow_context.cpp").read_text()

assert "finish_cli_process" not in source
assert "::_exit(status);" not in source
assert "CoreFlowContext context;" in source
assert source.count("context.Start(CoreFlowContext::Mode::ReadOnly)") == 2

for command, printer in (
    ('if (command == "--status")', "print_status(context);"),
    ('if (command == "--capabilities")', "print_capabilities(context);"),
):
    start = source.index(command)
    # Bound the handler at the next sibling command. The live --status path
    # intentionally returns early after printing the daemon snapshot, so using
    # the first "return 0;" would truncate the read-only fallback under test.
    next_command = source.find('\n        if (command ==', start + len(command))
    stop = next_command if next_command >= 0 else len(source)
    block = source[start:stop]
    assert "CoreFlowContext::Mode::ReadOnly" in block
    assert printer in block

status_start = source.index("std::string build_status_json(CoreFlowContext& context,")
status_end = source.index("void print_capabilities(CoreFlowContext& context)", status_start)
status = source[status_start:status_end]
assert "infer_and_learn(" not in status
assert "UclampGovernor " not in status
assert "VulkanController::instance" not in status
assert "AutonomousTuner::instance" not in status
assert '.last_verdict = "NOT_STARTED"' in status
assert '.backend = "not_started"' in status
assert "tuner.last_verdict" in status
assert "tunerRuntimeStarted" in status
assert "tunerRuntimeStage" in status
assert "context.autonomous_tuner().snapshot()" in status
assert "context.neural_predictor().last_prediction()" in status
assert "context.vulkan_controller().snapshot()" in status
assert "cached->profile" in status
assert "cached->thermal" in status
assert "get_top_app_min()" in status
assert "get_top_app_max()" in status
assert "write_status_snapshot" in source
assert "O_EXCL" in source and "renameat" in source
assert "print_live_status_snapshot(live_pid)" in source
assert "std::chrono::milliseconds(500)" in source
assert "kStartupGrace = std::chrono::seconds(12)" in source
assert "STAGE_OBSERVING -> STAGE_ACTIVE_TUNING" in source
assert "set_operational(false)" in source
assert "thermal.pressure <= 65.0" in source
admission_start = source.index("if (!lifecycle_promoted)")
admission_end = source.index("// Real-time Hybrid ONNX", admission_start)
assert "return;" not in source[admission_start:admission_end]
status_timer_start = source.index("const int status_timer_fd")
status_timer_end = source.index('emit_log(false, "[CoreFlow] C++23', status_timer_start)
assert "build_status_json" not in source[status_timer_start:status_timer_end]
assert "bool inspection_mode{false};" in config
assert "bool enable_tuner{true};" in config
assert "vulkanProbeSkipped" in status
assert "platformVendor" in status
assert "executionTier" in status
assert "tier1Eligible" in status

assert "CoreFlowContext::~CoreFlowContext()" in context
assert "coordinated_rollback(" in context
assert "destroy_runtime_services();" in context
assert "coordinator_" in context

# Explicit workload probe is read-only and bounded to two one-second feature windows.
assert 'if (command == "--workload-probe")' in source
assert "return run_workload_probe();" in source
probe_start = source.index("int run_workload_probe()")
probe_end = source.index("bool ensure_secure_state_dir()", probe_start)
probe = source[probe_start:probe_end]
assert "WorkloadTelemetryPipeline pipeline;" in probe
assert "std::chrono::milliseconds(10)" in probe
assert "aggregate.event_drain_saturated" in probe
assert "eventCount" in probe
assert "eventDrainSaturated" in probe
assert "begin_frame_timeline_capture(1000)" in probe
assert "finish_frame_timeline_capture(aggregate)" in probe
assert "gpu_busy_sum" in probe
assert "frameDeadlineMissCount" in probe
assert "pipeline.stop();" in probe
assert "MutationCoordinator" not in probe
