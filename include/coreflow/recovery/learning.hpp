#pragma once
#include "coreflow/model/types.hpp"
#include <string>
namespace coreflow {
class LearningJournal {
public:
    void record(const ChangeProposal&, const Outcome&, Disposition,
                const std::string& detail) const;
};
}
