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
    assert(!store.identity_unavailable());
    assert(store.capture(a));
    assert(store.apply_mutation(a, "512"));
    assert(read_value(a) == "512");
    assert(store.has_active_mutations());
    assert(store.owns_expected_state(a, "512"));
    assert(store.rollback_all() == 1);
    assert(read_value(a) == "0");
    assert(!store.has_active_mutations());

    // A failed operation is path-scoped: it must not roll back another
    // controller's still-active mutation on an unrelated node.
    assert(store.apply_mutation(a, "700"));
    assert(!store.apply_mutation(std::string(state_dir) + "/missing", "999"));
    assert(read_value(a) == "700");
    assert(store.has_active_mutations());
    assert(store.rollback(a));
    assert(read_value(a) == "0");
    assert(!store.has_active_mutations());

    // A rejected operation on one path must not silently reset other
    // active paths. Their owners remain responsible for explicit rollback.
    assert(store.capture(a));
    assert(store.capture(b));
    assert(store.apply_mutation(a, "610"));
    assert(store.apply_mutation(b, "210"));
    assert(!store.apply_mutation(std::string(state_dir) + "/missing-again", "999"));
    assert(read_value(a) == "610");
    assert(read_value(b) == "210");
    assert(store.rollback(a));
    assert(store.rollback(b));
    assert(read_value(a) == "0");
    assert(read_value(b) == "100");
    assert(!store.has_active_mutations());

    // Simulate fatal process death after a verified mutation, without shutdown (writes CFJ4).
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

    // Verify CFJ4 journal written by child process is recovered on same boot
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

    // A crash after journaling the next write in CFJ3 format, but before the kernel accepts it,
    // must recognize the previous CoreFlow value as ours and restore baseline.
    std::string boot_id = store.boot_id();
    assert(!boot_id.empty());
    { std::ofstream out(a); out << "888"; }
    { std::ofstream out(b); out << "222"; }
    {
        std::ofstream journal(std::string(state_dir) + "/baseline.journal");
        journal << "CFJ3\t" << boot_id << "\n"
                << a << "\t30\t383838\t393939\n"
                << b << "\t313030\t313030\t323232\n";
    }

    restorer = ::fork();
    assert(restorer >= 0);
    if (restorer == 0) {
        ::execl(argv[0], argv[0], "--recover", nullptr);
        ::_exit(127);
    }
    assert(::waitpid(restorer, &status, 0) == restorer);
    assert(WIFEXITED(status) && WEXITSTATUS(status) == 0);
    assert(read_value(a) == "0");
    assert(read_value(b) == "100");

    // Existing installations can still have a CFJ1 journal at upgrade time.
    { std::ofstream out(a); out << "555"; }
    {
        std::ofstream journal(std::string(state_dir) + "/baseline.journal");
        journal << "CFJ1\t" << boot_id << "\n" << a << "\t30\n";
    }
    restorer = ::fork();
    assert(restorer >= 0);
    if (restorer == 0) {
        ::execl(argv[0], argv[0], "--recover", nullptr);
        ::_exit(127);
    }
    assert(::waitpid(restorer, &status, 0) == restorer);
    assert(WIFEXITED(status) && WEXITSTATUS(status) == 0);
    assert(read_value(a) == "0");

    // An OEM write made after our crash is authoritative on the next start.
    writer = ::fork();
    assert(writer >= 0);
    if (writer == 0) {
        if (!store.apply_mutation(a, "888") || !store.apply_mutation(b, "222"))
            ::_exit(2);
        ::_exit(0);
    }
    assert(::waitpid(writer, &status, 0) == writer);
    assert(WIFEXITED(status) && WEXITSTATUS(status) == 0);
    { std::ofstream out(a); out << "777"; }
    restorer = ::fork();
    assert(restorer >= 0);
    if (restorer == 0) {
        ::execl(argv[0], argv[0], "--recover", nullptr);
        ::_exit(127);
    }
    assert(::waitpid(restorer, &status, 0) == restorer);
    assert(WIFEXITED(status) && WEXITSTATUS(status) == 0);
    assert(read_value(a) == "777");
    assert(read_value(b) == "100");
    assert(!store.apply_mutation(a, "999"));
    assert(read_value(a) == "777");
    assert(store.apply_mutation(a, "888"));
    { std::ofstream out(a); out << "666"; }
    assert(store.rollback(a));
    assert(read_value(a) == "666");

    // BASELINE-02: Inode replacement safety (unlink + recreate with new inode must not be overwritten)
    { std::ofstream out(a); out << "10"; }
    store.reinitialize();
    assert(store.recover_pending());
    assert(store.capture(a));
    assert(store.apply_mutation(a, "20"));
    assert(read_value(a) == "20");
    // Replace file at path `a` with a newly created file (different inode) holding value "20"
    // Even if its content happens to match active_value ("20"), its inode changed!
    const std::string replacement = std::string(state_dir) + "/replacement_a";
    { std::ofstream out(replacement); out << "20"; }
    assert(::unlink(a.c_str()) == 0);
    assert(::rename(replacement.c_str(), a.c_str()) == 0);
    assert(!store.owns_expected_state(a, "20"));
    assert(store.rollback(a));
    // Must NOT overwrite the new file with old factory_value "10"
    assert(read_value(a) == "20");

    // BASELINE-01: Missing kernel boot_id falls back to per-boot boot_epoch in state_dir
    const std::string missing_boot_id = std::string(state_dir) + "/nonexistent_boot_id";
    assert(::setenv("COREFLOW_TEST_BOOT_ID_PATH", missing_boot_id.c_str(), 1) == 0);
    const std::string boot_epoch_path = std::string(state_dir) + "/boot_epoch";
    { std::ofstream out(boot_epoch_path); out << "epoch-boot-A-10001\n"; }
    { std::ofstream out(a); out << "42"; }
    store.reinitialize();
    assert(store.recover_pending());
    assert(!store.identity_unavailable());
    assert(store.boot_id() == "epoch-boot-A-10001");
    assert(store.apply_mutation(a, "84"));
    assert(read_value(a) == "84");

    // Simulate Boot B: boot_epoch changes in post-fs-data.sh, kernel node resets to OEM "50"
    { std::ofstream out(boot_epoch_path); out << "epoch-boot-B-20002\n"; }
    { std::ofstream out(a); out << "50"; }
    store.reinitialize();
    assert(store.recover_pending());
    // Boot A's journal must NOT overwrite Boot B's OEM state ("50") with Boot A's ("42")
    assert(read_value(a) == "50");
    assert(!std::filesystem::exists(std::string(state_dir) + "/baseline.journal"));

    // BASELINE-01: Legacy "fallback-boot-id" journal is rejected and never replayed
    {
        std::ofstream journal(std::string(state_dir) + "/baseline.journal");
        journal << "CFJ3\tfallback-boot-id\n"
                << a << "\t30\t383838\t383838\n";
    }
    { std::ofstream out(a); out << "888"; }
    store.reinitialize();
    assert(!store.recover_pending());
    assert(read_value(a) == "888");
    (void)::unlink((std::string(state_dir) + "/baseline.journal").c_str());

    // BASELINE-01: Missing BOTH kernel boot_id and boot_epoch fails closed without deleting journal
    (void)::unlink(boot_epoch_path.c_str());
    {
        std::ofstream journal(std::string(state_dir) + "/baseline.journal");
        journal << "CFJ4\tepoch-boot-B-20002\n"
                << a << "\t3530\t3530\t3834\t1\t1\n";
    }
    store.reinitialize();
    assert(store.identity_unavailable());
    assert(!store.recover_pending());
    assert(std::filesystem::exists(std::string(state_dir) + "/baseline.journal"));
    assert(!store.capture(a));
    assert(!store.apply_mutation(a, "99"));
    assert(read_value(a) == "888");

    // A symlinked state directory must fail closed before creating or renaming
    // a journal in the symlink target.
    const std::string safe_state = std::string(state_dir) + "/safe-state";
    const std::string external_state = std::string(state_dir) + "/external-state";
    const std::string explicit_boot_id = std::string(state_dir) + "/explicit-boot-id";
    std::filesystem::create_directory(safe_state);
    std::filesystem::create_directory(external_state);
    { std::ofstream out(explicit_boot_id); out << "explicit-boot-identity-0001\n"; }
    assert(::setenv("COREFLOW_TEST_STATE_DIR", safe_state.c_str(), 1) == 0);
    assert(::setenv("COREFLOW_TEST_BOOT_ID_PATH", explicit_boot_id.c_str(), 1) == 0);
    store.reinitialize();
    assert(store.recover_pending());
    { std::ofstream out(a); out << "42"; }
    assert(store.capture(a));
    assert(std::filesystem::remove(safe_state));
    assert(::symlink(external_state.c_str(), safe_state.c_str()) == 0);
    assert(!store.apply_mutation(a, "84"));
    assert(read_value(a) == "42");
    assert(!std::filesystem::exists(external_state + "/baseline.journal"));
    assert(!std::filesystem::exists(external_state + "/baseline.journal.tmp"));

    // A clear in-memory state is not enough: the empty journal must also be
    // written durably before an orphaned coordinator lease can be retired.
    assert(!store.has_active_mutations());
    assert(!store.verify_and_persist_clear_state()); // Symlinked state path must fail closed.
    assert(!std::filesystem::exists(external_state + "/baseline.journal"));
    assert(std::filesystem::remove(safe_state)); // Remove the symlink, not its target.
    std::filesystem::create_directory(safe_state);
    store.reinitialize();
    assert(store.recover_pending());
    assert(!store.has_active_mutations());
    assert(store.verify_and_persist_clear_state());
    assert(std::filesystem::file_size(safe_state + "/baseline.journal") > 0);

    std::filesystem::remove_all(state_dir);
    std::cout << "[CoreFlow] Baseline + crash-recovery + boot_epoch + inode + state-path symlink invariants PASS\n";
    return 0;
}
