#include "coreflow/consultation.hpp"

#include <cstdlib>
#include <iostream>
#include <string>

namespace {
int failures = 0;
#define CHECK(x) do { if (!(x)) { ++failures; std::cerr << "FAIL: " #x "\n"; } } while (false)

using namespace coreflow;

// Grants are default-deny: a default UserGrants permits no namespace, so tests
// that need "no grant" build an explicitly opted-out set.
UserGrants noGrants() {
    UserGrants g;
    g.memory = g.block_queue = g.scheduler = false;
    g.gpu = g.cgroup = g.cpufreq = g.kernel = false;
    return g;
}

// A fact in which every check passes for the VM namespace. Each test flips
// exactly one field, so the reason it returns is the one under test.
ControlFact passingVmFact() {
    ControlFact f;
    f.ns = ControlNamespace::VmSysctl;
    f.exists = true;
    f.readable = true;
    f.writable = true;
    f.verified = true;
    f.baseline_captured = true;
    f.surface_admissible = true;
    f.semantic_known = true;
    f.stabilizing = true;
    f.benefit_justified = true;
    return f;
}

UserGrants grantAll() {
    UserGrants g;
    g.memory = true;
    g.block_queue = true;
    g.scheduler = true;
    return g;
}

void expect(const Consultation& c, ConsultVerdict v, ConsultReason r, const char* label) {
    if (c.verdict != v || c.reason != r) {
        ++failures;
        std::cerr << "FAIL [" << label << "]: got "
                  << consultVerdictName(c.verdict) << "/" << consultReasonName(c.reason)
                  << " expected " << consultVerdictName(v) << "/" << consultReasonName(r)
                  << "\n";
    }
}

void testAllowWhenEverythingPasses() {
    const auto c = consult(passingVmFact(), RuntimeState::Pressure, grantAll());
    expect(c, ConsultVerdict::Allow, ConsultReason::Allowed, "all pass");
    CHECK(c.need_present);
}

void testNoGrantDefersEvenWhenSafeAndNeeded() {
    // Everything is evidently safe and useful; only the user's permission is
    // missing. The device should ask, not act.
    const auto c = consult(passingVmFact(), RuntimeState::Pressure, noGrants());
    expect(c, ConsultVerdict::Defer, ConsultReason::AwaitingUserGrant, "no grant");
    CHECK(c.need_present);
}

void testNoNeedDefersEvenWithGrant() {
    // Permission alone is not a reason to write. Idle, Normal and Warming show
    // no demand, so nothing is changed.
    const RuntimeState calm[] = {RuntimeState::Idle, RuntimeState::Normal, RuntimeState::Warming};
    for (RuntimeState s : calm) {
        const auto c = consult(passingVmFact(), s, grantAll());
        expect(c, ConsultVerdict::Defer, ConsultReason::NoNeed, "no need");
        CHECK(!c.need_present);
    }
}

void testElevatedNeedsControl() {
    CHECK(runtimeNeedsControl(RuntimeState::Elevated));
    CHECK(runtimeNeedsControl(RuntimeState::Pressure));
    CHECK(runtimeNeedsControl(RuntimeState::ThermalGuard));
    CHECK(!runtimeNeedsControl(RuntimeState::Idle));
    CHECK(!runtimeNeedsControl(RuntimeState::Normal));
    CHECK(!runtimeNeedsControl(RuntimeState::Warming));
}

void testStressedAcceptsOnlyStabilizing() {
    ControlFact f = passingVmFact();
    f.stabilizing = false;
    expect(consult(f, RuntimeState::Pressure, grantAll()),
           ConsultVerdict::Defer, ConsultReason::StressedNonStabilizing, "stressed non-stab");
    expect(consult(f, RuntimeState::ThermalGuard, grantAll()),
           ConsultVerdict::Defer, ConsultReason::StressedNonStabilizing, "thermal non-stab");
    // Not stressed: a non-stabilizing change is not blocked by stress.
    expect(consult(f, RuntimeState::Elevated, grantAll()),
           ConsultVerdict::Allow, ConsultReason::Allowed, "elevated non-stab allowed");
}

void testNotObservableDefers() {
    const auto check = [](auto mutate, const char* label) {
        ControlFact f = passingVmFact();
        mutate(f);
        expect(consult(f, RuntimeState::Pressure, grantAll()),
               ConsultVerdict::Defer, ConsultReason::NotObservable, label);
    };
    check([](ControlFact& f) { f.exists = false; }, "missing");
    check([](ControlFact& f) { f.readable = false; }, "unreadable");
    check([](ControlFact& f) { f.writable = false; }, "unwritable");
    check([](ControlFact& f) { f.verified = false; }, "unverified");
}

void testMissingBaselineDefers() {
    ControlFact f = passingVmFact();
    f.baseline_captured = false;
    expect(consult(f, RuntimeState::Pressure, grantAll()),
           ConsultVerdict::Defer, ConsultReason::NoRollbackBaseline, "no baseline");
}

void testUnknownSemanticsDefers() {
    ControlFact f = passingVmFact();
    f.semantic_known = false;
    expect(consult(f, RuntimeState::Pressure, grantAll()),
           ConsultVerdict::Defer, ConsultReason::NoSemanticPrior, "no prior");
}

void testBenefitNotJustifiedDefers() {
    ControlFact f = passingVmFact();
    f.benefit_justified = false;
    expect(consult(f, RuntimeState::Pressure, grantAll()),
           ConsultVerdict::Defer, ConsultReason::NoBenefitEvidence, "no benefit");
}

void testSurfaceRefusedDenies() {
    ControlFact f = passingVmFact();
    f.surface_admissible = false;
    expect(consult(f, RuntimeState::Pressure, grantAll()),
           ConsultVerdict::Deny, ConsultReason::SurfaceDenied, "surface refused");
}

void testObserveOnlyNamespacesDenyAlways() {
    // Even with every other condition satisfied and permission granted, build
    // properties and settings are never written in this build.
    for (ControlNamespace ns : {ControlNamespace::BuildProp, ControlNamespace::SettingsDb}) {
        ControlFact f = passingVmFact();
        f.ns = ns;
        expect(consult(f, RuntimeState::Pressure, grantAll()),
               ConsultVerdict::Deny, ConsultReason::NoWriteActuator, controlNamespaceName(ns));
    }
    for (ControlNamespace ns : {ControlNamespace::CpuFreq, ControlNamespace::Other}) {
        ControlFact f = passingVmFact();
        f.ns = ns;
        expect(consult(f, RuntimeState::Pressure, grantAll()),
               ConsultVerdict::Deny, ConsultReason::OutOfScope, controlNamespaceName(ns));
    }
}

void testDenyOutranksDefer() {
    // A build prop that also lacks a baseline is a Deny, not a Defer: the
    // namespace is closed regardless of the other facts.
    ControlFact f = passingVmFact();
    f.ns = ControlNamespace::BuildProp;
    f.baseline_captured = false;
    expect(consult(f, RuntimeState::Pressure, noGrants()),
           ConsultVerdict::Deny, ConsultReason::NoWriteActuator, "deny outranks defer");
}

void testGrantsArePerNamespace() {
    UserGrants memoryOnly = noGrants();
    memoryOnly.memory = true;
    CHECK(memoryOnly.allows(ControlNamespace::VmSysctl));
    CHECK(!memoryOnly.allows(ControlNamespace::BlockQueue));
    CHECK(!memoryOnly.allows(ControlNamespace::Scheduler));
    CHECK(!memoryOnly.allows(ControlNamespace::BuildProp));
    CHECK(!noGrants().allows(ControlNamespace::VmSysctl));
    // Opt-out default: every tunable namespace is permitted.
    // Default-deny: no namespace is permitted without an explicit grant.
    CHECK(!UserGrants{}.allows(ControlNamespace::VmSysctl));
    CHECK(!UserGrants{}.allows(ControlNamespace::Gpu));
    CHECK(!UserGrants{}.allows(ControlNamespace::Cgroup));
    CHECK(!UserGrants{}.allows(ControlNamespace::CpuFreq));  // governor authority
    CHECK(!UserGrants{}.allows(ControlNamespace::BuildProp));

    ControlFact io = passingVmFact();
    io.ns = ControlNamespace::BlockQueue;
    expect(consult(io, RuntimeState::Pressure, memoryOnly),
           ConsultVerdict::Defer, ConsultReason::AwaitingUserGrant, "io not granted");
}

void testNamespaceMapping() {
    CHECK(namespaceOf(ResourceDomain::Memory, "vm.swappiness") == ControlNamespace::VmSysctl);
    CHECK(namespaceOf(ResourceDomain::Io, "sda:read_ahead_kb") == ControlNamespace::BlockQueue);
    CHECK(namespaceOf(ResourceDomain::Scheduler, "scheduler") == ControlNamespace::Scheduler);
    CHECK(namespaceOf(ResourceDomain::CpuFreq, "policies") == ControlNamespace::CpuFreq);
    CHECK(namespaceOf(ResourceDomain::Thermal, "thermal-zones") == ControlNamespace::Other);
}

void testSurfaceClassification() {
    const std::string vm = "/proc/sys/vm/swappiness";
    CHECK(classifySurface(ResourceDomain::Memory, "vm.swappiness", vm) == SurfaceClass::Admissible);
    CHECK(classifySurface(ResourceDomain::Memory, "vm.drop_caches", "/proc/sys/vm/drop_caches") ==
          SurfaceClass::Denied);
    CHECK(classifySurface(ResourceDomain::Memory, "sysrq", "/proc/sys/kernel/sysrq") ==
          SurfaceClass::Denied);
    CHECK(classifySurface(ResourceDomain::Io, "sda:read_ahead_kb",
                          "/sys/block/sda/queue/read_ahead_kb") == SurfaceClass::Admissible);
    // Capability-driven: every non-fatal kernel tunable is admissible.
    CHECK(classifySurface(ResourceDomain::Io, "sda:scheduler",
                          "/sys/block/sda/queue/scheduler") == SurfaceClass::Admissible);
    CHECK(classifySurface(ResourceDomain::Io, "zram0:read_ahead_kb",
                          "/sys/block/zram0/queue/read_ahead_kb") == SurfaceClass::Admissible);
    CHECK(classifySurface(ResourceDomain::Scheduler, "kernel.sched_latency_ns",
                          "/proc/sys/kernel/sched_latency_ns") == SurfaceClass::Admissible);
    CHECK(classifySurface(ResourceDomain::Gpu, "/sys/class/kgsl/kgsl-3d0/max_gpuclk",
                          "/sys/class/kgsl/kgsl-3d0/max_gpuclk") == SurfaceClass::Admissible);
    CHECK(classifySurface(ResourceDomain::UClamp, "/dev/cpuctl/top-app/cpu.uclamp.min",
                          "/dev/cpuctl/top-app/cpu.uclamp.min") == SurfaceClass::Admissible);
    // Fatal classes and the governor authority boundary.
    CHECK(classifySurface(ResourceDomain::Gpu, "x", "/sys/class/kgsl/kgsl-3d0/throttling") ==
          SurfaceClass::Denied);
    CHECK(classifySurface(ResourceDomain::Vendor, "x", "/sys/class/thermal/thermal_zone3/mode") ==
          SurfaceClass::Denied);
    CHECK(classifySurface(ResourceDomain::CpuFreq, "x",
                          "/sys/devices/system/cpu/cpufreq/policy0/scaling_governor") ==
          SurfaceClass::OutOfClass);
    CHECK(classifySurface(ResourceDomain::Thermal, "x", "/sys/class/foo/bar") ==
          SurfaceClass::OutOfClass);
    CHECK(classifySurface(ResourceDomain::Memory, "vm.x", "") == SurfaceClass::OutOfClass);
}

void testLedgerRecordsOutcomes() {
    ConsultationLedger ledger;
    ledger.record(ControlNamespace::VmSysctl,
                  consult(passingVmFact(), RuntimeState::Pressure, noGrants()));
    ledger.record(ControlNamespace::VmSysctl,
                  consult(passingVmFact(), RuntimeState::Pressure, grantAll()));
    ControlFact prop = passingVmFact();
    prop.ns = ControlNamespace::BuildProp;
    ledger.record(ControlNamespace::BuildProp,
                  consult(prop, RuntimeState::Pressure, grantAll()));

    CHECK(ledger.by_reason[static_cast<std::size_t>(ConsultReason::AwaitingUserGrant)] == 1U);
    CHECK(ledger.by_reason[static_cast<std::size_t>(ConsultReason::Allowed)] == 1U);
    CHECK(ledger.by_reason[static_cast<std::size_t>(ConsultReason::NoWriteActuator)] == 1U);
    CHECK(ledger.last_reason[static_cast<std::size_t>(ControlNamespace::VmSysctl)] ==
          ConsultReason::Allowed);
    CHECK(ledger.last_need[static_cast<std::size_t>(ControlNamespace::VmSysctl)]);
}

} // namespace

int main() {
    testAllowWhenEverythingPasses();
    testNoGrantDefersEvenWhenSafeAndNeeded();
    testNoNeedDefersEvenWithGrant();
    testElevatedNeedsControl();
    testStressedAcceptsOnlyStabilizing();
    testNotObservableDefers();
    testMissingBaselineDefers();
    testUnknownSemanticsDefers();
    testBenefitNotJustifiedDefers();
    testSurfaceRefusedDenies();
    testObserveOnlyNamespacesDenyAlways();
    testDenyOutranksDefer();
    testGrantsArePerNamespace();
    testNamespaceMapping();
    testSurfaceClassification();
    testLedgerRecordsOutcomes();

    if (failures != 0) return EXIT_FAILURE;
    std::cout << "CoreFlow consultation brain: PASS\n";
    return EXIT_SUCCESS;
}
