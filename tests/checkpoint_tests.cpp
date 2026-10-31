#include "coreflow/sample_sequence.hpp"
#include "coreflow/durable_file.hpp"
#include "coreflow/experience.hpp"
#include <filesystem>
#include <fstream>
#include <cstdio>
#include <cstdlib>
#include <limits>
using namespace coreflow;
#define CHECK(x) do { if (!(x)) { std::fprintf(stderr,"FAIL line %d: %s\n",__LINE__,#x); return 1; } } while(false)
int main() {
    char pattern[] = "/tmp/coreflow-checkpoint-XXXXXX";
    const auto* directory = ::mkdtemp(pattern); CHECK(directory != nullptr);
    const std::filesystem::path root(directory);
    const auto path = (root/"counter").string();
    SampleSequence first(path); CHECK(first.initialize());
    std::uint64_t id = 99;
    CHECK(first.next(id) && id == 0U); CHECK(first.next(id) && id == 1U);
    CHECK(first.checkpoint());
    SampleSequence clean(path); CHECK(clean.initialize()); CHECK(clean.next(id) && id == 2U);
    // Crash simulation: do not checkpoint clean's unused lease.
    SampleSequence crashed(path); CHECK(crashed.initialize()); CHECK(crashed.next(id) && id == 258U);
    for (unsigned i=0; i<SampleSequence::kBatch+5U; ++i) CHECK(crashed.next(id));
    const auto last = id; CHECK(crashed.checkpoint());
    SampleSequence resumed(path); CHECK(resumed.initialize()); CHECK(resumed.next(id) && id == last+1U);
    const auto broken = (root/"broken").string();
    std::ofstream(broken) << "corrupt";
    SampleSequence corrupt(broken); CHECK(!corrupt.initialize()); CHECK(!corrupt.next(id));
    std::string content; CHECK(readBoundedPrivateFile(broken,content) && content == "corrupt");
    SampleSequence missing((root/"missing"/"counter").string()); CHECK(missing.initialize()); CHECK(!missing.next(id)); CHECK(!missing.healthy());
    const auto exhaustion = (root/"exhaustion").string();
    FileMutationJournal exhaustedStore(exhaustion);
    CHECK(exhaustedStore.commit({{"next_sample",std::to_string(SampleSequence::kMaximum)}}));
    SampleSequence exhausted(exhaustion); CHECK(exhausted.initialize()); CHECK(!exhausted.next(id));
    SampleSequence syncFailure((root/"sync-failure").string()); CHECK(syncFailure.initialize());
    testing::failNextJournalDirectorySync(true); CHECK(!syncFailure.next(id));
    const auto file = (root/"experience").string();
    std::ofstream(root/"victim") << "untouched";
    std::filesystem::create_hard_link(root/"victim",file+".tmp");
    CHECK(writeDurablePrivateFile(file,"payload"));
    CHECK(readBoundedPrivateFile((root/"victim").string(),content) && content == "untouched");
    std::filesystem::create_symlink(file,root/"link");
    CHECK(!readBoundedPrivateFile((root/"link").string(),content));
    CHECK(!writeDurablePrivateFile(file,std::string(1024U*1024U+1U,'x')));
    ExperienceMemory memory; memory.setScope("device:kernel");
    ExperienceMemory::Record r; r.candidate.key="candidate"; r.score=.8; r.confidence=.9; r.observations=4; r.variance=.123456789012345;
    r.outcome=BaselineIntelligence::Outcome::Beneficial;
    CHECK(memory.record(r)); CHECK(memory.flush(file)); CHECK(!memory.dirty());
    ExperienceMemory loaded; loaded.setScope("device:kernel"); CHECK(loaded.load(file)); CHECK(loaded.size() == 1U);
    const auto* restored = loaded.find(r.candidate,r.context);
    CHECK(restored && restored->variance == r.variance && restored->score == r.score);
    CHECK(readBoundedPrivateFile(file,content));
    CHECK(content.starts_with("COREFLOW_EXPERIENCE_V3\n"));
    auto bad_variance = content;
    const auto variance_start = bad_variance.find_last_of('\t'); CHECK(variance_start != std::string::npos);
    bad_variance.replace(variance_start + 1U, bad_variance.size() - variance_start - 2U, "0.9");
    CHECK(writeDurablePrivateFile((root/"bad-variance").string(),bad_variance));
    CHECK(!loaded.load((root/"bad-variance").string()));
    CHECK(loaded.find(r.candidate,r.context)->variance == r.variance);
    // Older V2 experience remains readable with the conservative default variance.
    auto legacy = content;
    legacy.replace(0, std::string("COREFLOW_EXPERIENCE_V3").size(), "COREFLOW_EXPERIENCE_V2");
    const auto variance_tab = legacy.find_last_of('\t'); CHECK(variance_tab != std::string::npos);
    legacy.erase(variance_tab, legacy.size() - variance_tab - 1U);
    CHECK(writeDurablePrivateFile((root/"legacy").string(),legacy));
    ExperienceMemory old; old.setScope("device:kernel"); CHECK(old.load((root/"legacy").string()));
    CHECK(old.find(r.candidate,r.context) && old.find(r.candidate,r.context)->variance == .02);
    const auto score = content.find("0.8"); CHECK(score != std::string::npos);
    content.replace(score,3U,"0.8junk"); CHECK(writeDurablePrivateFile(file,content));
    CHECK(!loaded.load(file)); CHECK(loaded.size() == 1U); // Failed reads preserve the current advisory state.
    r.observations = std::numeric_limits<std::uint64_t>::max();
    CHECK(memory.record(r));
    const auto* saturated = memory.find(r.candidate,r.context);
    CHECK(saturated != nullptr && saturated->observations == std::numeric_limits<std::uint64_t>::max());
    CHECK(memory.record(r));
    saturated = memory.find(r.candidate,r.context);
    CHECK(saturated != nullptr && saturated->observations == std::numeric_limits<std::uint64_t>::max());
    std::filesystem::remove_all(root);
    std::puts("CoreFlow checkpoint persistence: PASS");
}
