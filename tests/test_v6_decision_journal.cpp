#include "coreflow/v6/decision_journal.hpp"

#include <cassert>
#include <iostream>
#include <string>

int main() {
    coreflow::v6::DecisionJournal journal;
    assert(journal.size() == 0U);
    assert(journal.total_recorded() == 0U);

    // Deduplicate identical consecutive episode within 4 seconds
    journal.record(1000U, "context", "ModeShift", "EFFICIENCY -> GAMING", "verified", 42.5, 0.4, 1.0);
    journal.record(2500U, "context", "ModeShift", "EFFICIENCY -> GAMING", "verified", 42.6, 0.4, 1.0);
    assert(journal.size() == 1U);
    assert(journal.total_recorded() == 1U);

    // Same event after >= 4 seconds records a fresh episode
    journal.record(6000U, "context", "ModeShift", "EFFICIENCY -> GAMING", "verified", 43.0, 0.5, 0.95);
    assert(journal.size() == 2U);

    // Ring buffer wrap-around keeps newest kCapacity items in newest-first order
    for (uint64_t i = 0; i < 45U; ++i) {
        journal.record(10000U + i * 5000U, "power", "Tick", "Action " + std::to_string(i), "verified", 40.0 + i * 0.1, 0.2, 1.0);
    }
    assert(journal.size() == coreflow::v6::DecisionJournal::kCapacity);
    const auto recent = journal.recent(5U);
    assert(recent.size() == 5U);
    assert(recent[0].action == "Action 44");
    assert(recent[4].action == "Action 40");
    assert(recent[0].id > recent[1].id);

    journal.clear();
    assert(journal.size() == 0U);
    assert(journal.recent().empty());

    std::cout << "CoreFlow v6 DecisionJournal tests PASSED\n";
    return 0;
}
