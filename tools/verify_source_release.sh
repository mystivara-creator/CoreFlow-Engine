#!/bin/sh
set -eu

ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"

[ -f "$ROOT/CMakeLists.txt" ]
[ -f "$ROOT/module/module.prop" ]
[ -f "$ROOT/module/service.sh" ]
[ -f "$ROOT/module/system/etc/coreflow/default.conf" ]
[ -f "$ROOT/src/main.cpp" ]
[ ! -f "$ROOT/module/system/bin/coreflowd" ]

grep -q '^version=v1.8.0$' "$ROOT/module/module.prop"
grep -q 'project(CoreFlowAutonomous VERSION 1.8.0 LANGUAGES CXX)' "$ROOT/CMakeLists.txt"
printf '%s\n' "CoreFlow source release checks: PASS"
