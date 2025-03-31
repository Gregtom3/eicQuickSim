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
    analysis.setAnalysisType("DISIDIS");
    analysis.setEnergyConfig(argv[1]);
    analysis.setCSVSource(argv[2]); // number of files or CSV path.
    analysis.setMaxEvents(std::stoi(argv[3]));
    analysis.setCollisionType(argv[4]);
    analysis.setBinningSchemePath(argv[5]);
    if(argc >= 7)
        analysis.setOutputCSV(argv[6]);
    
    // Set the two DISIDIS particle ids.
    analysis.setDISIDISPids(211, -211);
    
    // Set a DISIDIS (dihadron) value function
    analysis.setDihadValueFunction([](const dihadronKinematics& dih) -> std::vector<double> {
        return std::vector<double>{ dih.Q2, dih.x };
    });

    
    analysis.run();
    analysis.end();
    
    return 0;
}
