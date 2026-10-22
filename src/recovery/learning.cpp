#include "coreflow/recovery/learning.hpp"
#ifdef __ANDROID__
#include <android/log.h>
#endif
#include <iostream>
namespace coreflow {
void LearningJournal::record(const ChangeProposal& p, const Outcome& o,
                             Disposition d, const std::string& detail) const {
    const char* state=d==Disposition::Retain ? "retain" :
                      d==Disposition::Rollback ? "rollback" : "unresolved";
    std::string line="decision="+std::string(state)+" resource="+p.capability_id+
        " before_score="+std::to_string(o.before_score)+
        " after_score="+std::to_string(o.after_score)+" detail="+detail;
#ifdef __ANDROID__
    __android_log_print(ANDROID_LOG_INFO,"CoreFlowEngine","%s",line.c_str());
#else
    std::clog << "[CoreFlowEngine] " << line << '\n';
#endif
}
}
