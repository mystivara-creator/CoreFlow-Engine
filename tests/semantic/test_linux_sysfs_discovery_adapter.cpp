#include "coreflow/semantic/linux_sysfs_discovery_adapter.hpp"
#include "coreflow/semantic/capability_resolver.hpp"

#include <array>
#include <cassert>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

namespace fs = std::filesystem;
using namespace coreflow::semantic;

namespace {
void write(const fs::path& root, const std::string& relative, const std::string& value) {
    const auto path = root / relative;
    fs::create_directories(path.parent_path());
    std::ofstream stream(path);
    stream << value;
    assert(stream.good());
}
Identifier128 id(std::uint8_t seed) {
    Identifier128 value{};
    value.bytes[0] = seed;
    return value;
}
struct Fixture {
    fs::path root;
    fs::path sys;
    fs::path proc;
    fs::path cgroup;
    Fixture() {
        root = fs::temp_directory_path() / ("coreflow-linux-discovery-" + std::to_string(
            static_cast<unsigned long long>(reinterpret_cast<std::uintptr_t>(this))));
        sys = root / "sys";
        proc = root / "proc";
        cgroup = root / "cg" / "workload";
        fs::create_directories(sys);
        fs::create_directories(proc / "self");
        write(sys, "devices/system/cpu/cpufreq/policy0/scaling_driver", "schedutil\n");
        write(sys, "devices/system/cpu/cpufreq/policy0/scaling_cur_freq", "600000\n");
        write(sys, "devices/system/cpu/cpufreq/policy0/scaling_available_frequencies",
              "300000 600000 1200000\n");
        write(sys, "devices/system/cpu/cpufreq/policy0/cpuinfo_min_freq", "300000\n");
        write(sys, "devices/system/cpu/cpufreq/policy0/cpuinfo_max_freq", "1200000\n");
        write(proc, "self/cgroup", "0::/workload\n");
        write(proc, "self/mountinfo",
              "29 23 0:26 / " + (root / "cg").string() +
              " rw,nosuid,nodev,noexec,relatime - cgroup2 cgroup rw\n");
        write(cgroup, "cpu.uclamp.min", "0\n");
        write(cgroup, "cpu.uclamp.max", "1024\n");
        write(cgroup, "cpu.stat", "usage_usec 12345\nuser_usec 10000\nsystem_usec 2345\n");
        write(cgroup, "cpu.pressure",
              "some avg10=0.00 avg60=0.00 avg300=0.00 total=123\n"
              "full avg10=0.00 avg60=0.00 avg300=0.00 total=12\n");
    }
    ~Fixture() { std::error_code ec; fs::remove_all(root, ec); }
    std::size_t collect(std::array<DomainRelationshipEvidence, 3>& evidence) {
        coreflow::platform::LinuxSysfsDiscoveryAdapter adapter(sys.string(), proc.string());
        coreflow::platform::RawDiscoveredEndpoint endpoint{
            id(7), "sysfs:/devices/system/cpu/cpufreq/policy0/scaling_cur_freq", AccessCapability::Read, 44};
        return adapter.collectDomainRelationshipEvidence(endpoint, evidence);
    }
};
}

int main() {
    std::array<DomainRelationshipEvidence, 3> evidence{};
    {
        Fixture fixture;
        coreflow::platform::LinuxSysfsDiscoveryAdapter adapter(fixture.sys.string(), fixture.proc.string());
        const auto endpoints = adapter.discover();
        assert(endpoints.size() == 1);
        assert(endpoints[0].observed_generation != 0);
        const auto first_read = adapter.readPassive(endpoints[0]);
        const auto second_read = adapter.readPassive(endpoints[0]);
        assert(first_read.has_value() && second_read.has_value() && *first_read == *second_read);
        assert(*first_read == "600000");
        const auto numeric = adapter.readNumericFormat(endpoints[0]);
        assert(numeric.has_minimum && numeric.has_maximum);
        assert(numeric.minimum == 300000 && numeric.maximum == 1200000);
        assert(fixture.collect(evidence) == 3);
        assert(evidence[0].kind == DomainEvidenceKind::SubsystemAssociation);
        assert(evidence[1].kind == DomainEvidenceKind::DriverAbi);
        assert(evidence[2].kind == DomainEvidenceKind::PsiResponseCorrelation);
        assert(evidence[0].capability == evidence[1].capability);
        assert(evidence[1].capability == evidence[2].capability);
        assert(evidence[0].relationship == evidence[2].relationship);
        assert(evidence[0].observed_at_ns == evidence[2].observed_at_ns);
        DomainEvidenceAggregator aggregator{};
        const auto assessment = aggregator.aggregate(evidence, 44,
            evidence[0].observed_at_ns, 5'000'000'000ULL);
        assert(assessment.mutationEligible());

        // Production discovery -> resolver -> aggregator path, using the exact endpoint generation.
        CapabilityResolver resolver{};
        const auto resolved = resolver.resolveOne(endpoints[0], adapter,
            endpoints[0].observed_generation, endpoints[0].observed_generation + 1'000'000'000ULL);
        assert(resolved.has_value());
        assert(resolved->state() == DescriptorState::Validated);
        assert(resolved->domainAssessment().domain == FunctionalDomain::Cpu);
        assert(resolved->domainAssessment().corroborating_relations == 3U);
        assert(resolved->domainAssessment().generation == endpoints[0].observed_generation);
        assert(resolved->domainAssessment().assessed_at_ns != 0U);
        // Discovery is read-only; semantic validation does not grant write authority.
        assert(resolved->mutation().mode == MutationMode::None);
        assert(!resolved->isMutationCapable());
    }
    {
        Fixture fixture;
        fs::remove(fixture.cgroup / "cpu.uclamp.max");
        assert(fixture.collect(evidence) == 0); // partial subsystem evidence fails closed
    }
    {
        Fixture fixture;
        write(fixture.sys, "devices/system/cpu/cpufreq/policy0/scaling_available_frequencies",
              "300000 broken 1200000\n");
        assert(fixture.collect(evidence) == 0); // malformed frequency ABI fails closed
    }
    {
        Fixture fixture;
        write(fixture.proc, "self/mountinfo", "this is not valid mountinfo\n");
        assert(fixture.collect(evidence) == 0);
    }
    {
        Fixture fixture;
        fs::remove(fixture.cgroup / "cpu.pressure");
        assert(fixture.collect(evidence) == 0);
    }
    {
        Fixture fixture;
        write(fixture.sys, "devices/system/cpu/cpufreq/policy0/cpuinfo_max_freq", "300000\n");
        assert(fixture.collect(evidence) == 0);
    }
    std::cout << "Linux sysfs discovery adapter mock filesystem: PASS\n";
    return 0;
}
