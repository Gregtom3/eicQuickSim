#include "BinningScheme.h"
#include <cassert>
#include <iostream>

int main() {
    // Sample 2d YAML file
    // ------------------------------------------------
    // energy_config: "example_energy"
    // dimensions:
    //   - name: "energy"
    //     branch_true: "true_energy"
    //     branch_reco: "reco_energy"
    //     edges: [0, 10, 20]
    //   - name: "angle"
    //     branch_true: "true_angle"
    //     branch_reco: "reco_angle"
    //     edges: [0, 50, 100]
    // ------------------------------------------------
    BinningScheme schemeYAML("src/tests/bins/test09_sample_2d.yaml", BinningScheme::BinningType::RECTANGULAR_YAML);
    
    // Sample 2d CSV file
    // ------------------------------------------------
    // bin1min,bin1max,bin1_branch_true,bin1_branch_reco,bin2min,bin2max,bin2_branch_true,bin2_branch_reco
    // 0,10,true_energy,reco_energy,0,50,true_angle,reco_angle
    // 0,10,true_energy,reco_energy,50,100,true_angle,reco_angle
    // 10,20,true_energy,reco_energy,0,50,true_angle,reco_angle
    // 10,20,true_energy,reco_energy,50,100,true_angle,reco_angle
    // ------------------------------------------------
    BinningScheme schemeCSV("src/tests/bins/test09_sample_2d.csv", BinningScheme::BinningType::ND_CSV);

    // Define some test events.
    std::vector<std::vector<double>> testEvents = {
        {5, 30},    // Should be in bin: energy [0,10), angle [0,50)
        {5, 75},    // energy [0,10), angle [50,100)
        {15, 30},   // energy [10,20), angle [0,50)
        {15, 75},   // energy [10,20), angle [50,100)
        {25, 30}    // Out-of-range in energy => no bin (all -1)
    };

    for (const auto& ev : testEvents) {
        std::vector<int> binsYAML = schemeYAML.findBins(ev);
        std::vector<int> binsCSV = schemeCSV.findBins(ev);
        std::string keyYAML = schemeYAML.makeBinKey(binsYAML);
        std::string keyCSV = schemeCSV.makeBinKey(binsCSV);
        std::cout << "Event: (" << ev[0] << ", " << ev[1] << ") --> YAML bin: " << keyYAML 
                  << ", CSV bin: " << keyCSV << "\n";
        // Check that the keys match.
        assert(keyYAML == keyCSV);
    }
    std::cout << "Closure test passed: YAML and CSV binning yield the same results for these events.\n";
    return 0;
}