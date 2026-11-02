/**
 * @file device_profiler.hpp
 * @brief Dynamic SoC Topology and Capability Discovery
 * @author mystivara-creator
 * @license Apache-2.0
 */

#pragma once

#include "types.hpp"
#include <string>

namespace coreflow {

class DeviceProfiler {
public:
    static DeviceProfiler& instance();

    /// Detect active device topology and cluster layout
    DeviceTopology probe();

    /// Get current thermal zone temperature in Celsius
    float get_soc_temperature() const;

    /// Read active CPU frequencies for all clusters
    std::vector<uint32_t> get_cluster_frequencies() const;

private:
    DeviceProfiler() = default;
    DeviceTopology cached_topology_;
    bool probed_{false};

    void discover_clusters(DeviceTopology& topo);
    void detect_soc_name(DeviceTopology& topo);
};

} // namespace coreflow
