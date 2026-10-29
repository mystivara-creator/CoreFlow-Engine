#include "../integration/android/performance_session.hpp"
#include <array>
#include <cstdio>
#include <thread>
struct APerformanceHintManager {};
struct APerformanceHintSession { int calls{0}; };
namespace { APerformanceHintManager manager; bool fail_create = false; int closed = 0; int reported = 0; }
extern "C" {
APerformanceHintManager* APerformanceHint_getManager() { return &manager; }
APerformanceHintSession* APerformanceHint_createSession(APerformanceHintManager*, const pid_t*, std::size_t, std::int64_t) {
    return fail_create ? nullptr : new APerformanceHintSession{};
}
void APerformanceHint_closeSession(APerformanceHintSession* s) { ++closed; delete s; }
int APerformanceHint_reportActualWorkDuration(APerformanceHintSession* s, std::int64_t) { ++s->calls; ++reported; return 0; }
int APerformanceHint_updateTargetWorkDuration(APerformanceHintSession*, std::int64_t) { return EPIPE; }
}
#define CHECK(x) do { if (!(x)) { std::fprintf(stderr,"FAIL line %d: %s\n",__LINE__,#x); return 1; } } while(false)
int main() {
    std::array<pid_t, 1> tids{42};
    {
        coreflow::android::PerformanceSession session;
        CHECK(session.report(1) == ENOTSUP);
        CHECK(!session.start({}, 1));
        CHECK(!session.start(tids, 0));
        CHECK(session.start(tids, 100));
        fail_create = true;
        CHECK(!session.start(tids, 200));
        CHECK(closed == 0); // failed replacement preserves previous live session
        CHECK(session.report(0) == EINVAL);
        CHECK(session.setTarget(1) == EPIPE); // preserve native failure, do not fake success
        std::thread a([&] { for (int i=0;i<1000;++i) (void)session.report(100); });
        std::thread b([&] { for (int i=0;i<1000;++i) (void)session.report(100); });
        a.join(); b.join();
        CHECK(reported == 2000);
        session.close(); session.close();
        CHECK(closed == 1);
        CHECK(session.report(1) == ENOTSUP);
        fail_create = false;
        CHECK(session.start(tids, 100));
    }
    CHECK(closed == 2);
    std::puts("CoreFlow application hint lifecycle: PASS");
}
