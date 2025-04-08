R__LOAD_LIBRARY(build/lib/libeicQuickSim.so)
R__ADD_INCLUDE_PATH(src/eicQuickSim)

#include "Analysis.h"
#include <iostream>

//   - yamlConfig: path to the YAML file containing the configuration
//   - pid: the particle ID to use for SIDIS
void analysis_SIDIS(const char* yamlConfig, int pid) {
    using namespace eicQuickSim;
    Analysis a;
    a.initFromYaml(yamlConfig);
    a.setSIDISPid(pid);
    // Run the analysis.
    a.run();
    a.end();
}
