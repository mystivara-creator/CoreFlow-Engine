#pragma once
#include "coreflow/model/types.hpp"
namespace coreflow {
class CapabilitySafetyValidator {
public:
    bool validate(const ChangeProposal&, std::string& reason) const;
};
}
