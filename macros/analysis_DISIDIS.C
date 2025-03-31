#include "Analysis.h"
#include <iostream>

using namespace eicQuickSim;

int main(int argc, char* argv[]) {
    if(argc < 6) {
        std::cerr << "Usage: " << argv[0]
                  << " <energy configuration> <CSV source> <max events> <collision type> <path to bin scheme> [output file path]"
                  << std::endl;
        return 1;
    }
    
    Analysis analysis;
    analysis.setAnalysisType("DISIDIS");
    analysis.setEnergyConfig(argv[1]);
    analysis.setCSVSource(argv[2]);
    analysis.setMaxEvents(std::stoi(argv[3]));
    analysis.setCollisionType(argv[4]);
    analysis.setBinningSchemePath(argv[5]);
    if(argc >= 7)
        analysis.setOutputCSV(argv[6]);
    
    analysis.setDISIDISPids(211, -211);
    
    analysis.run();
    analysis.end();
    
    return 0;
}
