R__LOAD_LIBRARY(build/lib/libeicQuickSim.so)
R__ADD_INCLUDE_PATH(src/eicQuickSim)

#include "FileManager.h"
#include "CombinedRowsProcessor.h"
#include "Weights.h"

#include <iostream>
#include <string>

using std::cout;
using std::cerr;
using std::endl;

void preprocess_HPC(const char* energyConfig, int numFiles, int maxEvents,
                    const char* collisionType, const char* outputPath) {

    // Convert C-string parameters to std::string for ease of use.
    std::string energyConfigStr = energyConfig;
    std::string collisionTypeStr = collisionType;
    std::string outputPathStr = outputPath;
    
    // Parse the energy configuration (expected in NxM format, e.g., "10x100").
    size_t pos = energyConfigStr.find("x");
    if (pos == std::string::npos) {
        cerr << "Invalid energy configuration format. Expected format: NxM (e.g., 10x100)" << endl;
        return;
    }
    int beamEnergy1 = std::stoi(energyConfigStr.substr(0, pos));
    int beamEnergy2 = std::stoi(energyConfigStr.substr(pos + 1));
    // (Optionally, you can print or use the beam energies if needed)
    cout << "Parsed beam energies: " << beamEnergy1 << " and " << beamEnergy2 << endl;

    // Determine the input CSV file based on the collision type.
    std::string inputCSV;
    if (collisionTypeStr == "ep") {
        inputCSV = "src/eicQuickSim/ep_files.csv";
    } else if (collisionTypeStr == "en") {
        inputCSV = "src/eicQuickSim/en_files.csv";
    } else {
        cerr << "Invalid collision type. Expected 'ep' or 'en'." << endl;
        return;
    }
    
    // Construct a FileManager to handle the CSV.
    FileManager fm(inputCSV);
    
    // Get the combined rows based on energy configuration, number of files, and max events.
    auto rows = CombinedRowsProcessor::getCombinedRows(energyConfigStr, numFiles, maxEvents, collisionTypeStr);
    cout << "Combined " << rows.size() << " CSV rows." << endl;
    
    // Determine the luminosity file based on collision type.
    std::string lumiFile;
    if (collisionTypeStr == "ep") {
        lumiFile = "src/eicQuickSim/ep_lumi.csv";
    } else {
        lumiFile = "src/eicQuickSim/en_lumi.csv";
    }
    
    // Create a Weights object using the combined rows and the luminosity file.
    Weights q2Weights(rows, WeightInitMethod::LUMI_CSV, lumiFile);
    
    // Export the CSV with computed weights.
    if (!q2Weights.exportCSVWithWeights(rows, outputPathStr)) {
        cerr << "Failed to export CSV with weights." << endl;
        return;
    }
    
    cout << "Successfully exported CSV with weights to " << outputPathStr << endl;
}
