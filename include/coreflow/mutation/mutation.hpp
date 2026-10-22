#pragma once
#include "coreflow/model/types.hpp"
namespace coreflow {
class ResourceMutation {
public:
    AppliedChange apply(const ChangeProposal&) const;
    bool rollback(const AppliedChange&, std::string& reason) const;
};
}
