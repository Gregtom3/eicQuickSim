#include "FileManager.h"
#include "Weights.h"
#include <iostream>


int main() {
    // Step 1: Construct the FileManager
    FileManager fm("src/eicQuickSim/en_files.csv");
    
    // Step 2: Ask for all CSV data for e=10, h=100, Q2=10..100
    std::cout << "Ask for all CSV data for e=10, h=100, Q2=10..100" << std::endl;
    auto rows = fm.getCSVData(10, 100, 10, 100, -1);

    // Step 3: Get Q2 weights
    Weights q2Weights(rows);
    // Load in experimental luminosity to scale weights
    q2Weights.loadExperimentalLuminosity("src/eicQuickSim/en_lumi.csv");

    // Write the CSV with weights to a new file.
    const std::string outputPath = "artifacts/test06_10x100_Q2_10_100.csv";
    if (!q2Weights.exportCSVWithWeights(rows, outputPath)) {
        std::cerr << "Failed to export CSV with weights." << std::endl;
        return 1;
    }

    // Step 4: Re-load the CSV with weights and print out the weights
    FileManager fm_saved(outputPath);
    auto rows_saved = fm_saved.getAllCSVData(-1,-1);
    Weights q2Weights_saved(rows_saved);
    return 0;
}