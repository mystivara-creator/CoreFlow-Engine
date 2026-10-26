#include "coreflow/config.hpp"
#include "coreflow/consultation.hpp"

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <string>

#define CHECK(condition) do { \
    if (!(condition)) { \
        std::fprintf(stderr, "CHECK failed at %s:%d: %s\n", \
                     __FILE__, __LINE__, #condition); \
        return __LINE__; \
    } \
} while (false)

namespace fs = std::filesystem;
using namespace coreflow;

namespace {

fs::path writeConfig(const std::string& name, const std::string& body) {
    const fs::path path = fs::temp_directory_path() / name;
    std::ofstream out(path);
    out << body;
    return path;
}

// Default-deny: a fresh config grants nothing, and every write gate is closed.
int testFreshConfigGrantsNothing() {
    EngineConfig config;
    CHECK(!config.allowCpuGovernor());
    CHECK(!config.grantMemory());
    CHECK(!config.grantBlockQueue());
    CHECK(!config.grantScheduler());
    CHECK(!config.grantGpu());
    CHECK(!config.grantCgroup());
    CHECK(!config.grantCpuFreq());
    CHECK(!config.grantKernel());
    CHECK(!config.mutationArmed());
    CHECK(config.mutationMode() == MutationMode::Disabled);
    return 0;
}

// The default UserGrants used by the consultation ledger permits nothing.
int testDefaultUserGrantsDenyEverything() {
    const UserGrants grants;
    CHECK(!grants.allows(ControlNamespace::VmSysctl));
    CHECK(!grants.allows(ControlNamespace::BlockQueue));
    CHECK(!grants.allows(ControlNamespace::Scheduler));
    CHECK(!grants.allows(ControlNamespace::Gpu));
    CHECK(!grants.allows(ControlNamespace::Cgroup));
    CHECK(!grants.allows(ControlNamespace::CpuFreqTunable));
    CHECK(!grants.allows(ControlNamespace::KernelTunable));
    return 0;
}

// A grant for one domain must not authorise another.
int testGrantIsDomainScoped() {
    UserGrants grants;
    grants.memory = true;
    CHECK(grants.allows(ControlNamespace::VmSysctl));
    CHECK(!grants.allows(ControlNamespace::Gpu));
    CHECK(!grants.allows(ControlNamespace::BlockQueue));
    CHECK(!grants.allows(ControlNamespace::Cgroup));
    return 0;
}

// The shipped default.conf states every grant explicitly, so the armed behaviour
// is traceable and matches what the shipped file says.
int testShippedConfigGrantsAreExplicit() {
    EngineConfig config;
    const fs::path shipped = fs::path(COREFLOW_SOURCE_DIR) / "module/system/etc/coreflow/default.conf";
    CHECK(config.load(shipped.string()));
    CHECK(config.allowCpuGovernor());
    CHECK(config.grantMemory());
    CHECK(config.grantBlockQueue());
    CHECK(config.grantScheduler());
    CHECK(config.grantGpu());
    CHECK(config.grantCgroup());
    CHECK(config.grantCpuFreq());
    CHECK(config.grantKernel());
    CHECK(!config.mutationArmed());  // arming is still an explicit operator act
    return 0;
}

// Each grant key is read on its own. A file that grants only memory does not
// grant GPU.
int testSingleGrantKeyIsReadAlone() {
    const fs::path path = writeConfig("coreflow_grant_single.ini", "grant_memory=yes\n");
    EngineConfig config;
    CHECK(config.load(path.string()));
    CHECK(config.grantMemory());
    CHECK(!config.grantGpu());
    CHECK(!config.allowCpuGovernor());
    fs::remove(path);
    return 0;
}

// An explicit no overrides nothing else: it denies only its own domain.
int testExplicitNoDeniesOnlyItsDomain() {
    const fs::path path = writeConfig("coreflow_grant_no.ini",
                                      "grant_memory=yes\ngrant_gpu=no\nallow_cpu_governor=yes\n");
    EngineConfig config;
    CHECK(config.load(path.string()));
    CHECK(config.grantMemory());
    CHECK(!config.grantGpu());
    CHECK(config.allowCpuGovernor());
    fs::remove(path);
    return 0;
}


// Re-loading a reduced config is a replacement, not a merge: omitted grants
// must be withdrawn instead of leaking from the previously loaded file.
int testReloadWithdrawsOmittedGrants() {
    const fs::path first = writeConfig(
        "coreflow_grant_reload_first.ini",
        "mutation_mode=adaptive\nmutation_armed=true\ngrant_memory=yes\ngrant_gpu=yes\n");
    const fs::path second = writeConfig(
        "coreflow_grant_reload_second.ini",
        "mutation_mode=adaptive\nmutation_armed=true\n");
    EngineConfig config;
    CHECK(config.load(first.string()));
    CHECK(config.grantMemory());
    CHECK(config.grantGpu());
    CHECK(config.load(second.string()));
    CHECK(!config.grantMemory());
    CHECK(!config.grantGpu());
    CHECK(!config.allowCpuGovernor());
    CHECK(config.mutationMode() == MutationMode::Adaptive);
    CHECK(config.mutationArmed());
    fs::remove(first);
    fs::remove(second);
    return 0;
}

// A rejected mode withdraws every grant as well as arming (fail safe).
int testRejectedModeWithdrawsGrants() {
    const fs::path path = writeConfig(
        "coreflow_grant_bad_mode.ini",
        "mutation_mode=turbo\nmutation_armed=true\nallow_cpu_governor=yes\ngrant_memory=yes\n");
    EngineConfig config;
    CHECK(config.load(path.string()));
    CHECK(config.mutationMode() == MutationMode::Disabled);
    CHECK(!config.mutationArmed());
    CHECK(!config.allowCpuGovernor());
    CHECK(!config.grantMemory());
    fs::remove(path);
    return 0;
}

} // namespace

int main() {
    int failures = 0;
    failures += testFreshConfigGrantsNothing() != 0;
    failures += testDefaultUserGrantsDenyEverything() != 0;
    failures += testGrantIsDomainScoped() != 0;
    failures += testShippedConfigGrantsAreExplicit() != 0;
    failures += testSingleGrantKeyIsReadAlone() != 0;
    failures += testExplicitNoDeniesOnlyItsDomain() != 0;
    failures += testReloadWithdrawsOmittedGrants() != 0;
    failures += testRejectedModeWithdrawsGrants() != 0;
    if (failures != 0) {
        std::fprintf(stderr, "CoreFlow authorization gate tests: FAIL (%d)\n", failures);
        return 1;
    }
    std::puts("CoreFlow authorization gate tests: PASS");
    return 0;
}
