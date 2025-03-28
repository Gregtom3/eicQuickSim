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
#include <cctype>

using namespace HepMC3;
using std::cout;
using std::endl;

int main(int argc, char* argv[]) {
    // Updated usage message:
    if (argc < 6) {
        std::cerr << "Usage: " << argv[0]
                  << " <energy configuration (e.g., 5x41)> <number of files OR CSV path> <max events> <collision type (ep or en)> <path to bin scheme>"
                  << std::endl;
        return 1;
    }

    std::string energyConfig = argv[1];
    std::string secondArg = argv[2];  // could be number of files or CSV path
    int maxEvents = std::stoi(argv[3]);
    std::string collisionType = argv[4];
    std::string pathToBinScheme = argv[5];

    std::vector<CSVRow> combinedRows;
    bool isNumeric = true;
    // Check if every character in secondArg is a digit.
    for (char c : secondArg) {
        if (!std::isdigit(c)) {
            isNumeric = false;
            break;
        }
    }

    if (isNumeric) {
        // Use CombinedRowsProcessor if secondArg is numeric.
        int numFiles = std::stoi(secondArg);
        combinedRows = CombinedRowsProcessor::getCombinedRows(energyConfig, numFiles, maxEvents, collisionType);
    } else {
        // Otherwise, treat secondArg as a CSV file path.
        std::string csvPath = secondArg;
        FileManager fm(csvPath);
        combinedRows = fm.getAllCSVData(-1, -1);
    }

    std::cout << "Combined " << combinedRows.size() << " CSV rows." << std::endl;

    // Get Q2 weights.
    Weights q2Weights(combinedRows);
    // Load experimental luminosity based on collision type.
    if (collisionType == "ep") {
        q2Weights.loadExperimentalLuminosity("src/eicQuickSim/ep_lumi.csv");
    } else {
        q2Weights.loadExperimentalLuminosity("src/eicQuickSim/en_lumi.csv");
    }

    // Get binning scheme.
    BinningScheme binScheme(pathToBinScheme);
    cout << "Loaded binning scheme for energy config: " << binScheme.getEnergyConfig() << "\n";

    // Process each file.
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
            // Compute the weight for this event based on its Q2.
            double eventWeight = q2Weights.getWeight(dis.Q2);

            // Prepare a vector of values for each binning dimension (assumed order: Q2, x).
            std::vector<double> values = { dis.Q2, dis.x };
            binScheme.addEvent(values, eventWeight);
        }
        root_input.close();
    }

    // Get the bin scheme name.
    std::string binName = binScheme.getSchemeName();

    // Format the output CSV file name to include energy config, collision type, bin scheme name, max events,
    // and (if applicable) number of files.
    std::string outputCSV = "artifacts/analysis_DIS_energy=" + energyConfig +
                            "_type=" + collisionType +
                            "_yamlName=" + binName +
                            "_maxEvents=" + std::to_string(maxEvents);
    if (isNumeric) {
        outputCSV += "_numFiles=" + secondArg;
    }
    outputCSV += ".csv";

    try {
        binScheme.saveCSV(outputCSV);
        cout << "Saved binned scaled event counts to " << outputCSV << endl;
    } catch (const std::exception &ex) {
        std::cerr << "Error saving CSV: " << ex.what() << endl;
        return 1;
    }

    return 0;
}
