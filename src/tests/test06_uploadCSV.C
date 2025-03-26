#include "FileManager.h"
#include "FileDataSummary.h"
#include <iostream>

int main() {
    // Step 1: Construct the FileManager
    FileManager fm("src/eicQuickSim/en_files.csv");
    
    // Step 2: Ask for all CSV data for e=10, h=100, Q2=10..100
    std::cout << "Ask for all CSV data for e=10, h=100, Q2=10..100" << std::endl;
    auto rows = fm.getCSVData(10, 100, 10, 100, -1);

    // Step 3: Summarize with FileDataSummary
    FileDataSummary summarizer("src/eicQuickSim/en_lumi.csv");
    int totalEvents = summarizer.getTotalEvents(rows);
    double totalLumi = summarizer.getTotalLuminosity(rows);

    std::vector<double> weights = summarizer.getWeights(rows);

    // Write the CSV with weights to a new file.
    if (!fileDataSummary.exportCSVWithWeights(rows, weights, "artifacts/test06_10x100_Q2_10_100.csv")) {
        std::cerr << "Failed to export CSV with weights." << std::endl;
    }

    return 0;
}