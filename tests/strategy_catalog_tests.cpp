#include "coreflow/discovery/strategy_catalog.hpp"

#include <cassert>
#include <cstdio>
#include <fstream>
#include <string>
#include <unistd.h>

int main() {
    using namespace coreflow;
    const std::string path = "strategy_catalog_test_" +
        std::to_string(static_cast<long long>(::getpid())) + ".tsv";
    std::string error;
    {
        std::ofstream out(path);
        out << "schema_version\t2\n";
        // Prove that catalog size is driven by file contents and has no small fixed cap.
        for (int i = 0; i < 64; ++i)
            out << "strategy\ttest.rule" << i << "\tproc_sys.test\tattr" << i
                << "\t0\t100\n";
    }
    StrategyCatalog catalog;
    assert(catalog.load(path, error));
    assert(catalog.size() == 64);
    assert(catalog.find("proc_sys.test", "attr63") != nullptr);
    assert(catalog.find("proc_sys.test", "unknown") == nullptr);
    std::remove(path.c_str());

    {
        std::ofstream out(path);
        out << "schema_version\t2\n";
        out << "strategy\tbad.path\tproc_sys.vm\t/proc/sys/vm/swappiness\t0\t200\n";
    }
    assert(!catalog.load(path, error));
    assert(error.find("INVALID_STRATEGY_RULE") == 0);
    std::remove(path.c_str());

    {
        std::ofstream out(path);
        out << "schema_version\t2\n";
        out << "strategy\tfirst\tproc_sys.vm\tswappiness\t0\t200\n";
        out << "strategy\tsecond\tproc_sys.vm\tswappiness\t0\t300\n";
    }
    assert(!catalog.load(path, error));
    assert(error.find("DUPLICATE_STRATEGY_SELECTOR") == 0);
    std::remove(path.c_str());
    return 0;
}
