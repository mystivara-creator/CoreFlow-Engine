#include "coreflow/runtime/state_store.hpp"

#include <cassert>
#include <cstdio>
#include <atomic>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <thread>
#include <vector>
#include <unistd.h>

using namespace coreflow;
namespace fs = std::filesystem;

namespace {

// Minimal structural check: balanced braces/brackets outside strings and no raw control
// characters. Enough to catch broken escaping or a missing comma-free separator.
bool structurallyValidJson(const std::string& text) {
    int depth = 0;
    bool in_string = false;
    bool escaped = false;
    for (const unsigned char c : text) {
        if (in_string) {
            if (escaped) { escaped = false; continue; }
            if (c == '\\') { escaped = true; continue; }
            if (c == '"') in_string = false;
            if (c < 0x20) return false;
            continue;
        }
        if (c == '"') in_string = true;
        else if (c == '{' || c == '[') ++depth;
        else if (c == '}' || c == ']') { if (--depth < 0) return false; }
    }
    return depth == 0 && !in_string;
}

} // namespace

int main() {
    // Exact-token commands only; nothing is interpreted as a shell fragment.
    assert(parseCommand("pause\n") == Command::Pause);
    assert(parseCommand("  resume \r\n") == Command::Resume);
    assert(parseCommand("rescan") == Command::Rescan);
    assert(parseCommand("reset_learning") == Command::ResetLearning);
    assert(parseCommand("PAUSE") == Command::None);
    assert(parseCommand("pause; rm -rf /") == Command::None);
    assert(parseCommand("") == Command::None);
    assert(commandToken(Command::ResetLearning) == "reset_learning");
    assert(commandToken(Command::None).empty());

    assert(jsonEscape("a\"b\\c\n\x01") == "a\\\"b\\\\c\\n\\u0001");

    RuntimeStatus status;
    status.engine_state = "running";
    status.stage = "mutation";
    status.generated_unix = 1700000000;
    status.model = "Model \"X\"";
    status.system.thermal_headroom_valid = true;
    status.system.thermal_headroom_c = 4.5;
    // Invalid measurements must be null, never fabricated zeros.
    const auto json = formatStatusJson(status);
    assert(structurallyValidJson(json));
    assert(json.find("\"engine_state\":\"running\"") != std::string::npos);
    assert(json.find("\"thermal_headroom_c\":4.5") != std::string::npos);
    assert(json.find("\"cpu_busy_percent\":null") != std::string::npos);
    assert(json.find("\"model\":\"Model \\\"X\\\"\"") != std::string::npos);

    std::vector<CapabilityRecord> records;
    records.push_back({"/a", "sysfs.block.queue", "nr_requests", "", "128", true});
    records.push_back({"/b", "sysfs.generic", "x", "", "1", false});
    records.push_back({"/c", "sysfs.cpu.governor", "rate_limit_us", "s", "500", true});
    const auto caps = formatCapabilitiesJson(records, 2);
    assert(structurallyValidJson(caps));
    assert(caps.find("\"total\":3") != std::string::npos);
    assert(caps.find("\"/c\"") == std::string::npos);  // limit applied
    assert(caps.find("\"eligible\":true") != std::string::npos);

    JournalRecord journal{"mutation", 1700000001, "/sys/x", "1", "2", "retain", "line\nbreak"};
    const auto line = formatJournalLine(journal);
    assert(structurallyValidJson(line));
    assert(line.find('\n') == std::string::npos);

    // Store round-trip on a private temporary directory.
    const fs::path dir = fs::temp_directory_path() /
        ("coreflow_state_test_" + std::to_string(static_cast<long long>(::getpid())));
    fs::remove_all(dir);
    StateStore store(dir.string());
    assert(store.ready());
    assert(store.writeStatus(status));
    assert(fs::exists(dir / "status.json"));
    assert(!fs::exists(dir / "status.json.tmp"));  // atomic replace leaves no temporary file
    assert(store.writeCapabilities(records));
    assert(store.appendJournal(journal));
    assert(store.appendJournal(journal));
    {
        std::ifstream in(dir / "journal.jsonl");
        std::string first, second, third;
        assert(std::getline(in, first) && std::getline(in, second));
        assert(!std::getline(in, third));
    }

    // Command file: consumed exactly once, invalid tokens are dropped too.
    { std::ofstream(dir / "command") << "resume\n"; }
    assert(store.takeCommand() == Command::Resume);
    assert(!fs::exists(dir / "command"));
    assert(store.takeCommand() == Command::None);
    { std::ofstream(dir / "command") << "reboot-device\n"; }
    assert(store.takeCommand() == Command::None);
    assert(!fs::exists(dir / "command"));
    { std::ofstream(dir / "command") << std::string(4096, 'x'); }
    assert(store.takeCommand() == Command::None);
    assert(!fs::exists(dir / "command"));

    // Journal rotation keeps a single previous generation.
    {
        std::ofstream big(dir / "journal.jsonl", std::ios::app);
        big << std::string(300U * 1024U, 'j') << '\n';
    }
    assert(store.appendJournal(journal));
    assert(fs::exists(dir / "journal.jsonl.1"));
    assert(fs::file_size(dir / "journal.jsonl") < 1024);

    // Concurrent callers share one store: status replacement remains valid and
    // journal writes remain complete single JSON lines.
    std::atomic<bool> concurrent_ok{true};
    std::vector<std::thread> writers;
    for (int worker = 0; worker < 4; ++worker) {
        writers.emplace_back([&, worker] {
            for (int i = 0; i < 25; ++i) {
                RuntimeStatus local;
                local.engine_state = "running";
                local.stage = "concurrent-test";
                local.generated_unix = 1700000100 + worker * 25 + i;
                local.stale_after_s = 60;
                JournalRecord entry{"mutation", local.generated_unix, "/sys/test",
                                    "1", "2", "retain", "concurrent write"};
                if (!store.writeStatus(local) || !store.appendJournal(entry)) {
                    concurrent_ok.store(false, std::memory_order_relaxed);
                }
            }
        });
    }
    for (auto& writer : writers) writer.join();
    assert(concurrent_ok.load(std::memory_order_relaxed));
    {
        std::ifstream in(dir / "status.json");
        const std::string snapshot((std::istreambuf_iterator<char>(in)), {});
        assert(structurallyValidJson(snapshot));
        assert(snapshot.find("\"stage\":\"concurrent-test\"") != std::string::npos);
    }
    {
        std::ifstream in(dir / "journal.jsonl");
        std::string entry;
        std::size_t line_count = 0;
        while (std::getline(in, entry)) {
            assert(structurallyValidJson(entry));
            ++line_count;
        }
        assert(line_count == 101); // one post-rotation entry plus 100 concurrent writes
    }
    // Separate StateStore instances must not consume the same mailbox command.
    { std::ofstream(dir / "command") << "pause\n"; }
    StateStore second_consumer(dir.string());
    std::atomic<int> consumed{0};
    std::thread consumer_a([&] {
        if (store.takeCommand() != Command::None) consumed.fetch_add(1, std::memory_order_relaxed);
    });
    std::thread consumer_b([&] {
        if (second_consumer.takeCommand() != Command::None)
            consumed.fetch_add(1, std::memory_order_relaxed);
    });
    consumer_a.join();
    consumer_b.join();
    assert(consumed.load(std::memory_order_relaxed) == 1);
    assert(!fs::exists(dir / "command"));

    for (const auto& item : fs::directory_iterator(dir)) {
        const auto name = item.path().filename().string();
        assert(name.find(".tmp.") == std::string::npos);
        assert(name.find("command.claimed.") == std::string::npos);
    }

    fs::remove_all(dir);
    return 0;
}
