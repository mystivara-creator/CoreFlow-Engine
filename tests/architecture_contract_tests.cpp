#include "coreflow/model/types.hpp"
#include <cassert>
int main() {
    coreflow::Capability c;
    c.id="runtime-discovered";
    c.path="/runtime/discovered/resource";
    c.numeric=true;
    c.writable=true;
    c.range={-2.0,8.0,true};
    assert(c.range.valid && c.range.minimum < c.range.maximum);
    coreflow::ChangeProposal p{c.id,c.path,"1","2",c.range.minimum,c.range.maximum,"test"};
    assert(p.capability_id==c.id);
    return 0;
}
