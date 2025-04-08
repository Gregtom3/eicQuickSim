R__LOAD_LIBRARY(build/lib/libeicQuickSim.so)
R__ADD_INCLUDE_PATH(src/eicQuickSim)

#include "Analysis.h"
#include <iostream>

//   - yamlConfig: path to the YAML file with configuration (excluding the particle ids)
//   - pid1: first particle ID
//   - pid2: second particle ID
void analysis_DISIDIS(const char* yamlConfig, int pid1, int pid2) {
    using namespace eicQuickSim;
    Analysis a;
    a.initFromYaml(yamlConfig);
    a.setDISIDISPids(pid1, pid2);
    
    // Run the analysis.
    a.run();
    a.end();
}
