R__LOAD_LIBRARY(build/lib/libeicQuickSim.so)
R__ADD_INCLUDE_PATH(src/eicQuickSim)

#include "Analysis.h"
#include <iostream>

void analysis_DIS(const char* yamlConfig = "out/test_v0/config_en_5x41/config.yaml") {
    using namespace eicQuickSim;
    Analysis a;
    a.initFromYaml(yamlConfig);
    a.run();
    a.end();
}
