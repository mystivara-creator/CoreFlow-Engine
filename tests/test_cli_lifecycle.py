#!/usr/bin/env python3
"""Guard one-shot diagnostic CLI paths against static-destruction regressions."""
from pathlib import Path

source = (Path(__file__).resolve().parents[1] / "src/daemon/main.cpp").read_text()
required = (
    "[[noreturn]] void finish_cli_process(int status) noexcept",
    "std::cout.flush();",
    "std::cerr.flush();",
)
assert required[0] in source
assert required[1] in source
assert required[2] in source
assert "::_exit(status);" in source

status = source.index('if (command == "--status")')
capabilities = source.index('if (command == "--capabilities")')
assert source.index("print_status();", status) < source.index("finish_cli_process(0);", status)
assert source.index("print_capabilities();", capabilities) < source.index("finish_cli_process(0);", capabilities)
