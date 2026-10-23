#include "coreflow/runtime/rollback_guard.hpp"

#include <cassert>
#include <stdexcept>
#include <type_traits>
#include <utility>

namespace {
using coreflow::AppliedChange;
using coreflow::runtime::RollbackGuard;

struct FakeRollback {
    int* calls;
    bool result{true};
    bool throws{false};

    bool operator()(const AppliedChange&, std::string& reason) const {
        ++*calls;
        if (throws) {
            throw std::runtime_error("simulated rollback exception");
        }
        reason = result ? "restored" : "simulated failure";
        return result;
    }
};

using Guard = RollbackGuard<FakeRollback>;
static_assert(!std::is_copy_constructible_v<Guard>);
static_assert(!std::is_copy_assignable_v<Guard>);
static_assert(!std::is_move_constructible_v<Guard>);
static_assert(!std::is_move_assignable_v<Guard>);
} // namespace

int main() {
    AppliedChange applied{};

    // Active guard performs exactly one best-effort rollback.
    {
        int calls = 0;
        { Guard guard(FakeRollback{&calls}, applied); }
        assert(calls == 1);
    }

    // A transaction guard can be created before mutation and armed only when
    // the first write is attempted. Leaving it disarmed must not roll back.
    {
        int calls = 0;
        { Guard guard(FakeRollback{&calls}, applied, false); }
        assert(calls == 0);
    }

    // Arming after the write boundary restores during unwinding and preserves
    // the original exception.
    {
        int calls = 0;
        bool caught_original = false;
        try {
            Guard guard(FakeRollback{&calls}, applied, false);
            guard.arm();
            throw std::runtime_error("simulated post-write exception");
        } catch (const std::runtime_error& e) {
            caught_original = std::string(e.what()) == "simulated post-write exception";
        }
        assert(caught_original);
        assert(calls == 1);
    }

    // Normal completion disarms the guard.
    {
        int calls = 0;
        {
            Guard guard(FakeRollback{&calls}, applied);
            guard.dismiss();
        }
        assert(calls == 0);
    }

    // A failed rollback is attempted once, with no hidden second attempt.
    {
        int calls = 0;
        { Guard guard(FakeRollback{&calls, false, false}, applied); }
        assert(calls == 1);
    }

    // A throwing rollback callback cannot escape the destructor.
    {
        int calls = 0;
        { Guard guard(FakeRollback{&calls, true, true}, applied); }
        assert(calls == 1);
    }

    // During stack unwinding, rollback is still attempted and the original
    // exception remains observable to the caller.
    {
        int calls = 0;
        bool caught_original = false;
        try {
            Guard guard(FakeRollback{&calls}, applied);
            throw std::runtime_error("original failure");
        } catch (const std::runtime_error& e) {
            caught_original = std::string(e.what()) == "original failure";
        }
        assert(caught_original);
        assert(calls == 1);
    }

    return 0;
}
