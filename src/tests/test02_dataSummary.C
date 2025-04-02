#include "FileManager.h"
#include "Weights.h"
#include <iostream>

int main() {
    // --- Step 1: Load CSV data for the 5x41 configuration ---
    FileManager fm("src/eicQuickSim/en_files.csv");
    std::cout << "Loading CSV data for SIDIS overlay analysis (5x41 configuration).\n";
    const int MAX_EVENTS = 10000;
    auto rows_1_10    = fm.getCSVData(5, 41, 1, 10, 3, MAX_EVENTS);
    auto rows_10_100  = fm.getCSVData(5, 41, 10, 100, 3, MAX_EVENTS);
    auto rows_100_1000 = fm.getCSVData(5, 41, 100, 1000, 3, MAX_EVENTS);
    std::vector<std::vector<CSVRow>> groups = { rows_1_10, rows_10_100, rows_100_1000 };
    std::vector<CSVRow> combinedRows = FileManager::combineCSV(groups);
    std::cout << "Combined " << combinedRows.size() << " CSV rows.\n";
    
    // Step 2: Get Q2 weights
    Weights q2Weights(combinedRows, WeightInitMethod::LUMI_CSV, "src/eicQuickSim/en_lumi.csv");

    
    return 0;
}
