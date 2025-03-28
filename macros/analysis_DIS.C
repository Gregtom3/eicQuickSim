#include "FileManager.h"
#include "Kinematics.h"
#include "Weights.h"
#include "BinningScheme.h"
#include "CombinedRowsProcessor.h"

// ROOT & HepMC3 includes:
#include "HepMC3/GenEvent.h"
#include "HepMC3/ReaderRootTree.h"
#include "HepMC3/GenParticle.h"
#include "TLorentzVector.h"
#include "TCanvas.h"
#include "TH1D.h"
#include "TPad.h"

#include <iostream>
#include <vector>
#include <cmath>


using namespace HepMC3;
using std::cout;
using std::endl;

//------------------------------------------------------
// Main analysis code
int main(int argc, char* argv[]) {
    // Check that the correct number of arguments is provided.
    if (argc < 6) {
        std::cerr << "Usage: " << argv[0] 
                  << " <energy configuration (e.g., 5x41)> <number of files> <max events> <collision type (ep or en)> <path to bin scheme>"
                  << std::endl;
        return 1;
    }

    std::string energyConfig = argv[1];
    int numFiles = std::stoi(argv[2]);
    int maxEvents = std::stoi(argv[3]);
    std::string collisionType = argv[4];
    std::string pathToBinScheme = argv[5];
    auto combinedRows = CombinedRowsProcessor::getCombinedRows(energyConfig, numFiles, maxEvents, collisionType);
    std::cout << "Combined " << combinedRows.size() << " CSV rows." << std::endl;

    // Get Q2 weights
    Weights q2Weights(combinedRows);
    // Load in experimental luminosity to scale weights
    q2Weights.loadExperimentalLuminosity("src/eicQuickSim/ep_lumi.csv");

    // Get binning scheme
    BinningScheme binScheme(pathToBinScheme);
    cout << "Loaded binning scheme for energy config: " << binScheme.getEnergyConfig() << "\n";

    // Step 4: Process each file
    for (size_t i = 0; i < combinedRows.size(); ++i) {
        CSVRow row = combinedRows[i];
        std::string fullPath = row.filename;
        cout << "Processing file: " << fullPath << endl;
        
        ReaderRootTree root_input(fullPath);
        if (root_input.failed()) {
            std::cerr << "Failed to open file: " << fullPath << endl;
            continue;
        }
        
        int eventsParsed = 0;
        while (!root_input.failed() && eventsParsed < maxEvents) {
            GenEvent evt;
            root_input.read_event(evt);
            if (root_input.failed()) break;
            eventsParsed++;
        
            eicQuickSim::Kinematics kin;
            kin.computeDIS(evt);
            eicQuickSim::disKinematics dis = kin.getDISKinematics();
            // Compute the weight for this event based on its Q2
            double eventWeight = q2Weights.getWeight(dis.Q2);

            // Prepare a vector of values for each binning dimension (assumed order: Q2, X).
            std::vector<double> values = { dis.Q2, dis.x };
            
            // Add this event to the binning scheme.
            binScheme.addEvent(values, eventWeight);
            
        }
        root_input.close();
    }
    
    std::string binName = binScheme.getSchemeName();

    // Format the output CSV file name to include energy configuration, collision type, bin scheme name, numFiles, and maxEvents.
    std::string outputCSV = "artifacts/analysis_DIS_energy=" + energyConfig + "_type=" + collisionType + "_yamlName=" 
                              + binName + "_numFiles=" + std::to_string(numFiles) + "_maxEvents=" + std::to_string(maxEvents) + ".csv";
    
    try {
        binScheme.saveCSV(outputCSV);
        cout << "Saved binned scaled event counts to " << outputCSV << endl;
    } catch (const std::exception &ex) {
        std::cerr << "Error saving CSV: " << ex.what() << endl;
        return 1;
    }

    return 0;
}
