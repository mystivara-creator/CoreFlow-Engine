#include "coreflow/frequency_controller.hpp"
#include "coreflow/baseline_store.hpp"
#include <cassert>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>
#include <unistd.h>
namespace fs = std::filesystem;
static void put(const fs::path& p, const std::string& value) { fs::create_directories(p.parent_path()); std::ofstream(p) << value; }
static std::string get(const fs::path& p) { std::string value; std::ifstream(p) >> value; return value; }
int main() {
    char pattern[] = "/tmp/coreflow-frequency-XXXXXX";
    const char* dir = ::mkdtemp(pattern); assert(dir);
    const fs::path root(dir);
    ::setenv("COREFLOW_TEST_STATE_DIR", dir, 1);
    auto& baseline = coreflow::BaselineStore::instance(); assert(baseline.recover_pending());
    const auto cpu = root / "devices/system/cpu/cpufreq/policy0";
    put(cpu / "scaling_max_freq", "2000000"); put(cpu / "scaling_min_freq", "500000");
    put(cpu / "scaling_available_frequencies", "500000 1000000 1500000 2000000");
    const auto gpu = root / "class/devfreq/gpu";
    put(gpu / "max_freq", "800000000"); put(gpu / "min_freq", "200000000");
    put(gpu / "available_frequencies", "200000000 400000000 600000000 800000000");
    coreflow::FrequencyController controller(root.string()); controller.discover(true, true);
    assert(controller.cpu_count() == 1 && controller.gpu_count() == 1);
    assert(controller.apply(0.75));
    assert(get(cpu / "scaling_max_freq") == "1500000");
    assert(get(gpu / "max_freq") == "600000000");
    put(cpu / "scaling_max_freq", "1000000"); // OEM changed its cap.
    assert(controller.apply(1.0));
    assert(get(cpu / "scaling_max_freq") == "1000000");
    assert(controller.cpu_count() == 0);
    assert(get(gpu / "max_freq") == "800000000");
    baseline.rollback_all(); assert(get(cpu / "scaling_max_freq") == "1000000");
    assert(!controller.apply(1.1));
    fs::remove_all(root);
}
