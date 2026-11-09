#!/usr/bin/env python3
"""Guard read-only CLI and deterministic composition-root lifecycle contracts."""
from pathlib import Path

source = (Path(__file__).resolve().parents[1] / "src/daemon/main.cpp").read_text()
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
    stop = source.index("return 0;", start)
    block = source[start:stop]
    assert "CoreFlowContext::Mode::ReadOnly" in block
    assert printer in block

status_start = source.index("void print_status(CoreFlowContext& context)")
status_end = source.index("void print_capabilities(CoreFlowContext& context)", status_start)
status = source[status_start:status_end]
assert "infer_and_learn(" not in status
assert "UclampGovernor " not in status
assert "VulkanController::instance" not in status
assert "AutonomousTuner::instance" not in status

assert "CoreFlowContext::~CoreFlowContext()" in context
assert "coordinated_rollback(" in context
assert "destroy_runtime_services();" in context
assert "coordinator_" in context
