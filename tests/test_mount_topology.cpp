#include "coreflow/mount_topology.hpp"

#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <unistd.h>

namespace fs = std::filesystem;
using namespace coreflow;

static void put(const fs::path& path, const std::string& value) {
    fs::create_directories(path.parent_path());
    std::ofstream stream(path, std::ios::binary | std::ios::trunc);
    stream << value;
    assert(stream.good());
}

static std::string mount_escape(std::string value) {
    std::string out;
    for (const char ch : value) {
        if (ch == ' ') out += "\\040";
        else out.push_back(ch);
    }
    return out;
}

int main() {
    char pattern[] = "/tmp/coreflow-mount-topology-XXXXXX";
    const char* temp = ::mkdtemp(pattern);
    assert(temp != nullptr);
    const fs::path root(temp);
    const fs::path proc = root / "proc";
    const fs::path mount = root / "mount point with spaces";
    const fs::path active = mount / "workload-group-17";

    put(proc / "self/mountinfo", "41 30 0:42 / " + mount_escape(mount.string()) +
        " rw,nosuid,nodev - cgroup cgroup rw,cpu,cpuacct\n");
    put(proc / "441/cgroup", "7:cpu,cpuacct:/workload-group-17\n");
    put(active / "cpu.uclamp.min", "31.25\n");
    put(active / "cpu.uclamp.max", "100.00\n");
    put(active / "cpu.uclamp.latency_sensitive", "0\n");
    put(active / "tasks", "441\n");
    put(active / "cgroup.procs", "441\n");

    MountTopologyResolver resolver(proc.string());
    const auto mounts = resolver.discover_cgroup_mounts();
    assert(mounts.size() == 1U);
    assert(mounts.front().filesystem == CgroupFilesystem::V1);
    assert(mounts.front().mount_point == mount.string());
    assert(mounts.front().controllers.size() >= 2U);

    const auto groups = resolver.discover_cgroup_groups();
    assert(groups.size() == 1U);
    assert(groups.front().has_uclamp_min && groups.front().has_uclamp_max);
    assert(groups.front().has_uclamp_latency_sensitive);
    assert(groups.front().membership == CgroupMembershipInterface::Tasks);
    assert(groups.front().has_process_list);
    assert(groups.front().absolute_path == active.string());

    const auto memberships = resolver.process_memberships(441);
    assert(memberships.size() == 1U);
    assert(memberships.front().absolute_path == active.string());
    assert(memberships.front().relative_path == "workload-group-17");

    const auto decimal = MountTopologyResolver::parse_uclamp_value("31.25\n");
    assert(decimal && decimal->encoding == UclampEncoding::Percent && decimal->capacity == 320U);
    const auto hundred = MountTopologyResolver::parse_uclamp_value("100.00");
    assert(hundred && hundred->capacity == 1024U);
    const auto integer = MountTopologyResolver::parse_uclamp_value("320");
    assert(integer && integer->encoding == UclampEncoding::Capacity1024 && integer->capacity == 320U);
    const auto maximum = MountTopologyResolver::parse_uclamp_value("max");
    assert(maximum && maximum->is_max && maximum->capacity == 1024U);
    assert(!MountTopologyResolver::parse_uclamp_value("1025"));
    assert(!MountTopologyResolver::parse_uclamp_value("100.01"));
    assert(MountTopologyResolver::encode_uclamp_value(320U, UclampEncoding::Percent, false) == "31.25");
    assert(MountTopologyResolver::encode_uclamp_value(1024U, UclampEncoding::Percent, true) == "max");
    assert(MountTopologyResolver::encode_uclamp_value(320U, UclampEncoding::Capacity1024, false) == "320");

    // A second mount with a different mount-point layout must also resolve;
    // the algorithm is independent of endpoint and group names.
    const fs::path v2_mount = root / "unified-root";
    put(proc / "self/mountinfo", "41 30 0:42 / " + mount_escape(mount.string()) +
        " rw,nosuid,nodev - cgroup cgroup rw,cpu,cpuacct\n" +
        "42 30 0:43 / " + mount_escape(v2_mount.string()) +
        " rw,nosuid,nodev - cgroup2 cgroup rw\n");
    put(v2_mount / "nested/interactive-9/cpu.uclamp.min", "0.00\n");
    put(v2_mount / "nested/interactive-9/cpu.uclamp.max", "max\n");
    put(v2_mount / "nested/interactive-9/cgroup.threads", "441\n");
    const auto all_mounts = resolver.discover_cgroup_mounts();
    assert(all_mounts.size() == 2U);
    assert(all_mounts[1].filesystem == CgroupFilesystem::V2);
    const auto all_groups = resolver.discover_cgroup_groups();
    assert(all_groups.size() == 2U);
    assert(all_groups[1].membership == CgroupMembershipInterface::Threads);

    // Foreground resolution is based on process membership and the discovered
    // tasks ABI, not a particular cgroup directory name or UClamp availability.
    const fs::path foreground = mount / "foreground-set-29";
    put(foreground / "tasks", "441\n");
    put(proc / "441/status", "Name:\tgame\nUid:\t10123\t10123\t10123\t10123\nTgid:\t441\n");
    put(proc / "441/oom_score_adj", "0\n");
    put(proc / "441/cgroup", "7:cpu,cpuacct:/foreground-set-29\n");
    const auto foreground_group = resolver.resolve_foreground_group();
    assert(foreground_group.has_value());
    assert(foreground_group->absolute_path == foreground.string());
    assert(foreground_group->membership == CgroupMembershipInterface::Tasks);
    assert(!resolver.resolve_foreground_uclamp_group().has_value());

    fs::remove_all(root);
    std::cout << "Mount topology, cgroup ABI, process membership and UClamp encoding: PASS\n";
    return 0;
}
