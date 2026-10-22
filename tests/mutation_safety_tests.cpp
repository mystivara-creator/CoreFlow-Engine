#include "coreflow/mutation/mutation.hpp"
#include "coreflow/validation/validation.hpp"
#include <cassert>
#include <filesystem>
#include <fstream>
#include <string>
#include <unistd.h>
int main() {
    using namespace coreflow;
    const auto temp_root = std::filesystem::canonical(std::filesystem::temp_directory_path());
    const auto path = temp_root /
        ("coreflow-mutation-test-" + std::to_string(static_cast<long long>(::getpid())));
    { std::ofstream f(path); f << "10"; }
    ResourceMutation mutation;
    ChangeProposal p{"test-capability", path.string(), "10", "11", 0.0, 20.0, "unit test", {}};
    CapabilitySafetyValidator validator;
    std::string validation_reason;
    assert(validator.validate(p, validation_reason));
    auto applied = mutation.apply(p);
    assert(applied.write_attempted && applied.write_succeeded);
    assert(applied.readback_value == "11");
    std::string reason;
    assert(mutation.rollback(applied, reason));
    { std::ifstream f(path); std::string value; f >> value; assert(value == "10"); }

    // Discrete kernel controls must be restricted to values exposed by the kernel,
    // then read back and rollback using their normalized active selection.
    const auto discrete_path = temp_root /
        ("coreflow-discrete-test-" + std::to_string(static_cast<long long>(::getpid())));
    { std::ofstream f(discrete_path); f << "old"; }
    ChangeProposal discrete{"test-selector", discrete_path.string(), "old", "new", 0.0, 0.0,
                            "discrete test", {"old", "new", "aux"}};
    assert(validator.validate(discrete, validation_reason));
    auto discrete_applied = mutation.apply(discrete);
    assert(discrete_applied.write_attempted && discrete_applied.write_succeeded);
    assert(mutation.rollback(discrete_applied, reason));
    { std::ifstream f(discrete_path); std::string value; f >> value; assert(value == "old"); }
    discrete.target_value = "not-advertised";
    assert(!validator.validate(discrete, validation_reason));
    assert(!mutation.apply(discrete).write_attempted);
    std::filesystem::remove(discrete_path);

    // A concurrent change between discovery and mutation must not be overwritten.
    { std::ofstream f(path); f << "12"; }
    auto stale = mutation.apply(p);
    assert(!stale.write_attempted && !stale.write_succeeded);
    reason.clear();
    assert(mutation.rollback(stale, reason));
    { std::ifstream f(path); std::string value; f >> value; assert(value == "12"); }
    const auto target = path.string() + "-target";
    const auto link = path.string() + "-link";
    { std::ofstream f(target); f << "21"; }
    std::filesystem::create_symlink(target, link);
    ChangeProposal symlink_proposal{"symlink-capability", link, "21", "22", 0.0, 30.0, "symlink test", {}};
    const auto symlink_change = mutation.apply(symlink_proposal);
    assert(!symlink_change.write_attempted && !symlink_change.write_succeeded);
    assert(!validator.validate(symlink_proposal, validation_reason));
    { std::ifstream f(target); std::string value; f >> value; assert(value == "21"); }
    std::filesystem::remove(link);
    std::filesystem::remove(target);

    const auto parent_target = path.string() + "-parent-target";
    const auto parent_link = path.string() + "-parent-link";
    { std::ofstream f(parent_target); f << "31"; }
    std::filesystem::create_directory_symlink(std::filesystem::path(parent_target).parent_path(), parent_link);
    const auto redirected = parent_link / std::filesystem::path(parent_target).filename();
    ChangeProposal parent_symlink_proposal{"parent-symlink-capability", redirected.string(), "31", "32", 0.0, 40.0, "parent symlink test", {}};
    const auto parent_symlink_change = mutation.apply(parent_symlink_proposal);
    assert(!parent_symlink_change.write_attempted && !parent_symlink_change.write_succeeded);
    assert(!validator.validate(parent_symlink_proposal, validation_reason));
    { std::ifstream f(parent_target); std::string value; f >> value; assert(value == "31"); }
    std::filesystem::remove(parent_link);
    std::filesystem::remove(parent_target);
    std::filesystem::remove(path);
    return 0;
}
