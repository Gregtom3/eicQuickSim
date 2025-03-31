#include "Analysis.h"
#include <iostream>

using namespace eicQuickSim;

int main(int argc, char* argv[]) {
    if(argc < 6) {
        std::cerr << "Usage: " << argv[0]
                  << " <energyConfig> <CSV source> <maxEvents> <collisionType> <binSchemePath> [outputCSV]"
                  << std::endl;
        return 1;
    }
    
    Analysis analysis;
    analysis.setAnalysisType("SIDIS");
    analysis.setEnergyConfig(argv[1]);
    analysis.setCSVSource(argv[2]); // number of files or CSV path.
    analysis.setMaxEvents(std::stoi(argv[3]));
    analysis.setCollisionType(argv[4]);
    analysis.setBinningSchemePath(argv[5]);
    if(argc >= 7)
        analysis.setOutputCSV(argv[6]);
    
    // Set SIDIS particle id to 211.
    analysis.setSIDISPid(211);
    
    // Set a SIDIS value function
    analysis.setSIDISValueFunction([](const sidisKinematics& sid) -> std::vector<double> {
        return std::vector<double>{ sid.Q2, sid.x };
    });
    
    analysis.run();
    analysis.end();
    
    return 0;
}
