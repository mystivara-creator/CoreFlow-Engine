#pragma once
#include <cstddef>
#include <cstdint>
#include <sys/types.h>
struct APerformanceHintManager;
struct APerformanceHintSession;
extern "C" {
APerformanceHintManager* APerformanceHint_getManager();
APerformanceHintSession* APerformanceHint_createSession(APerformanceHintManager*, const pid_t*, std::size_t, std::int64_t);
void APerformanceHint_closeSession(APerformanceHintSession*);
int APerformanceHint_reportActualWorkDuration(APerformanceHintSession*, std::int64_t);
int APerformanceHint_updateTargetWorkDuration(APerformanceHintSession*, std::int64_t);
}
