#include "FileManager.h"
#include "Weights.h"
#include <iostream>
#include <string>

using std::cout;
using std::endl;

int main(int argc, char* argv[]) {
    // Check that the correct number of arguments is provided.
    if (argc < 6) {
        std::cerr << "Usage: " << argv[0]
                  << " <energy configuration (e.g., 10x100)> <number of files> <max events> <collision type (ep or en)> <output CSV path>"
                  << std::endl;
        return 1;
    }
    
    // Parse command-line arguments.
    std::string energyConfig = argv[1];
    int numFiles = std::stoi(argv[2]);
    int maxEvents = std::stoi(argv[3]);
    std::string collisionType = argv[4];
    std::string outputPath = argv[5];
    
    // Parse energy configuration (e.g., "10x100").
    size_t pos = energyConfig.find("x");
    if (pos == std::string::npos) {
        std::cerr << "Invalid energy configuration format. Expected format: NxM (e.g., 10x100)" << std::endl;
        return 1;
    }
    int beamEnergy1 = std::stoi(energyConfig.substr(0, pos));
    int beamEnergy2 = std::stoi(energyConfig.substr(pos + 1));
    
    // Determine input CSV file based on collision type.
    std::string inputCSV;
    if (collisionType == "ep") {
        inputCSV = "src/eicQuickSim/ep_files.csv";
    } else if (collisionType == "en") {
        inputCSV = "src/eicQuickSim/en_files.csv";
    } else {
        std::cerr << "Invalid collision type. Expected 'ep' or 'en'." << std::endl;
        return 1;
    }
    
    // Step 1: Construct the FileManager.
    FileManager fm(inputCSV);
    
    // Step 2: Get CSV data. Here, we assume a Q² range from 10 to 100.
    cout << "Getting CSV data for energy configuration " << energyConfig 
         << " with Q² range 10 to 100." << endl;
    auto rows = fm.getCSVData(beamEnergy1, beamEnergy2, 10, 100, numFiles, maxEvents);
    
    // Step 3: Get Q² weights.
    Weights q2Weights(rows);
    
    // Load experimental luminosity based on collision type.
    if (collisionType == "ep") {
        q2Weights.loadExperimentalLuminosity("src/eicQuickSim/ep_lumi.csv");
    } else {
        q2Weights.loadExperimentalLuminosity("src/eicQuickSim/en_lumi.csv");
    }
    
    // Write the CSV with weights to the user-specified output path.
    if (!q2Weights.exportCSVWithWeights(rows, outputPath)) {
        std::cerr << "Failed to export CSV with weights." << std::endl;
        return 1;
    }
    
    cout << "Successfully exported CSV with weights to " << outputPath << endl;
    return 0;
}
