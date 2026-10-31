#include "coreflow/decision_agent.hpp"
#include "coreflow/frame_latency.hpp"
#include "coreflow/strategy_catalog.hpp"
#include "coreflow/bounded_process.hpp"
#include "coreflow/high_capability.hpp"
#include "coreflow/fatal_surface.hpp"
#include <cstdio>
#include <cstdlib>
#include <new>

namespace {
bool fail_allocation = false;
bool count_allocations = false;
std::size_t allocation_count = 0;
}
void* operator new(std::size_t size) {
    if (count_allocations) ++allocation_count;
    if (fail_allocation) throw std::bad_alloc{};
    if (void* p = std::malloc(size == 0U ? 1U : size)) return p;
    throw std::bad_alloc{};
}
void* operator new[](std::size_t size) { return ::operator new(size); }
void operator delete(void* p) noexcept { std::free(p); }
void operator delete[](void* p) noexcept { ::operator delete(p); }
void operator delete(void* p, std::size_t) noexcept { ::operator delete(p); }
void operator delete[](void* p, std::size_t) noexcept { ::operator delete(p); }

#define CHECK(x) do { if (!(x)) { std::fprintf(stderr, "FAIL line %d: %s\n", __LINE__, #x); return 1; } } while (false)
int main() {
    using namespace coreflow;
    ScoringDecisionAgent agent;
    EffectModel model;
    EnvironmentCapabilityMatrix matrix;
    matrix.resources.emplace_back(); // forces rank() to reserve
    std::unordered_map<std::string, std::string> baselines;
    EffectContext context;
    const std::string latency = "16666666\n1 100000000 1\n2 116666666 2\n";
    const std::vector<std::string> args = {"/bin/echo", "test"};
    ResourceCapability gpu;
    gpu.domain = ResourceDomain::Gpu;
    gpu.path = "/sys/devfreq/governor";
    CouplingSnapshot snapshot;
    snapshot.profile = StrategyProfile::SustainedGaming;
    context.gaming_active = true;

    ResourceStateModel resources;
    resources.reset(matrix);
    auto larger_matrix = matrix;
    larger_matrix.resources.resize(32U);
    const std::string long_value(128U, '7');
    ResourceConstraint constraint;
    constraint.blocking = true;
    fail_allocation = true;
    const bool fatal = isFatalValue(long_value, "1");
    const auto proposal = agent.reason({});
    const auto ranked = model.rank(matrix, baselines, context, 8U);
    const auto frame = graphics::parseSurfaceFlingerLatency(latency);
    const auto factory = createDefaultDecisionAgent();
    const auto bounded = process::runBounded(args, 100U, 1024U);
    const auto adjustment = StrategyCatalog::evaluateCandidate(gpu, "ondemand", "PERFORMANCE",
        EffectDirection::Select, snapshot, context);
    const bool observed = resources.observe(matrix.resources[0].domain, matrix.resources[0].name, long_value);
    const bool baseline = resources.setBaseline(matrix.resources[0].domain, matrix.resources[0].name, long_value);
    const bool desired = resources.setDesired(matrix.resources[0].domain, matrix.resources[0].name, long_value);
    const bool verified = resources.setVerified(matrix.resources[0].domain, matrix.resources[0].name, long_value);
    const bool constrained = resources.addConstraint(matrix.resources[0].domain, matrix.resources[0].name, std::move(constraint));
    const auto state = resources.all()[0].lifecycle;
    resources.reset(larger_matrix);
    fail_allocation = false;
    CHECK(!proposal.valid && ranked.empty() && !frame.valid && factory == nullptr);
    CHECK(!bounded.started && !fatal);
    CHECK(!observed && !baseline && !desired && !verified && !constrained);
    CHECK(state == ResourceLifecycle::RecoveryRequired && resources.size() == 0U);
    matrix.resources[0].path = "/sys/devices/platform/qualcomm/gpu/devfreq/available_frequencies";
    count_allocations = true;
    const auto expanded = HighCapabilityExpansion::assess(matrix);
    count_allocations = false;
    CHECK(expanded.size() == 15U && allocation_count == 1U);
    CHECK(adjustment.benefit_delta > 0.0); // hot classification uses no allocation
    std::puts("CoreFlow allocation failure boundaries: PASS");
    return 0;
}
