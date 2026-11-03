#include "coreflow/baseline_store.hpp"
#include <cassert>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <sys/wait.h>
#include <unistd.h>

using namespace coreflow;

static std::string read_value(const std::string& path) {
    std::ifstream in(path);
    std::string value;
    in >> value;
    return value;
}
int main(int argc, char* argv[]) {
    if (argc > 1 && std::string(argv[1]) == "--recover") {
        return BaselineStore::instance().recover_pending() ? 0 : 2;
    }
    char state_dir[] = "/tmp/coreflow-journal-XXXXXX";
    assert(::mkdtemp(state_dir) != nullptr);
    assert(::setenv("COREFLOW_TEST_STATE_DIR", state_dir, 1) == 0);

    const std::string a = std::string(state_dir) + "/mock_a";
    const std::string b = std::string(state_dir) + "/mock_b";
    { std::ofstream out(a); out << "0"; }
    { std::ofstream out(b); out << "100"; }
    auto& store = BaselineStore::instance();
    assert(store.recover_pending());
    assert(store.capture(a));
    assert(store.apply_mutation(a, "512"));
    assert(read_value(a) == "512");
    assert(store.has_active_mutations());
    assert(store.rollback_all() == 1);
    assert(read_value(a) == "0");
    assert(!store.has_active_mutations());

    // Capture failure must rollback existing mutations, too.
    assert(store.apply_mutation(a, "700"));
    assert(!store.apply_mutation(std::string(state_dir) + "/missing", "999"));
    assert(read_value(a) == "0");
    assert(!store.has_active_mutations());

    // Simulate fatal process death after a verified mutation, without shutdown.
    pid_t writer = ::fork();
    assert(writer >= 0);
    if (writer == 0) {
        if (!store.apply_mutation(a, "888") || !store.apply_mutation(b, "222"))
            ::_exit(2);
        ::_exit(0); // no cleanup, journal must retain the OEM values
    }
    int status = 0;
    assert(::waitpid(writer, &status, 0) == writer);
    assert(WIFEXITED(status) && WEXITSTATUS(status) == 0);
    assert(read_value(a) == "888");
    assert(read_value(b) == "222");

    // A new process detects same-boot journal and restores verified OEM values.
    pid_t restorer = ::fork();
    assert(restorer >= 0);
    if (restorer == 0) {
        ::execl(argv[0], argv[0], "--recover", nullptr);
        ::_exit(127);
    }
    assert(::waitpid(restorer, &status, 0) == restorer);
    assert(WIFEXITED(status) && WEXITSTATUS(status) == 0);
    assert(read_value(a) == "0");
    assert(read_value(b) == "100");

    std::filesystem::remove_all(state_dir);
    std::cout << "[CoreFlow] Baseline + crash-recovery invariants PASS\n";
    return 0;
}
