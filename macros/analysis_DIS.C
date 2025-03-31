#include "Analysis.h"
#include <iostream>

int main(int argc, char* argv[]) {
    if (argc < 6) {
        std::cerr << "Usage: " << argv[0]
                  << " <energy configuration> <CSV source> <max events> <collision type> <path to bin scheme> [output file path]"
                  << std::endl;
        return 1;
    }
    
    using namespace eicQuickSim;
    
    Analysis analysis;
    analysis.setAnalysisType("DIS");
    analysis.setEnergyConfig(argv[1]);
    analysis.setCSVSource(argv[2]); // either a number of files or a CSV path
    analysis.setMaxEvents(std::stoi(argv[3]));
    analysis.setCollisionType(argv[4]);
    analysis.setBinningSchemePath(argv[5]);
    if (argc >= 7)
        analysis.setOutputCSV(argv[6]);
    
    analysis.setDISValueFunction([](const HepMC3::GenEvent& evt, const Kinematics& kin) {
        return std::vector<double>{ kin.getDISKinematics().Q2, kin.getDISKinematics().x };
    });
    
    analysis.run();
    analysis.end();
    
    return 0;
}
