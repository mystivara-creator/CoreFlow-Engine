#pragma once
#include "coreflow/model/types.hpp"
namespace coreflow {
class ChangeJustifier {
public:
    bool justified(const ChangeProposal&, const ObservationFrame&,
                   std::string& reason) const;
};
}
