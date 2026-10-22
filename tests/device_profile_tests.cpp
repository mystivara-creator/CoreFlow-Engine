#include "coreflow/profile/device_profile.hpp"
#include <cassert>
#include <fstream>
#include <string>
int main() {
    const std::string path = "device_profile_test.tsv";
    {
        std::ofstream f(path);
        f << "schema_version\t1\n";
        f << "device\tmanufacturer\tXiaomi\n";
        f << "device\tmodel\t25098RA98G\n";
        f << "device\tcodename\tkunzite\n";
        f << "device\tsoc\tSM6475\n";
        f << "device\tandroid_api\t36\n";
        f << "device\tkernel\t6.6-test\n";
        f << "capability\tcpu.policy0.governor\tcpu\t/sys/example/governor\tenum\tread-only\tOBSERVE_ONLY\t\tinventory\n";
    }
    coreflow::DeviceProfile p;
    std::string error;
    assert(coreflow::DeviceProfileReader{}.load(path,p,error));
    assert(p.device_codename == "kunzite");
    assert(p.soc_model == "SM6475");
    assert(p.capabilities.size() == 1);
    assert(p.capabilities[0].semantic_status == "OBSERVE_ONLY");
    {
        std::ofstream f(path);
        f << "schema_version\t1\n";
        f << "device\tcodename\tkunzite\n";
        f << "device\tsoc\tSM6475\n";
        f << "capability\tdup\tcpu\t/a\tint\tread-only\tOBSERVE_ONLY\t\tinv\n";
        f << "capability\tdup\tcpu\t/b\tint\tread-only\tOBSERVE_ONLY\t\tinv\n";
    }
    assert(!coreflow::DeviceProfileReader{}.load(path,p,error));
    assert(error == "DUPLICATE_CAPABILITY_ID at line 5");
    std::remove(path.c_str());
}
