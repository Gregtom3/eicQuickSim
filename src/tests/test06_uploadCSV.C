#include "FileManager.h"
#include "Weights.h"
#include <iostream>


int main() {
    // Step 1: Construct the FileManager
    FileManager fm("src/eicQuickSim/en_files.csv");
    
    // Step 2: Ask for all CSV data for e+n 5x41
    std::cout << "Ask for all CSV data for e=5, h=41" << std::endl;
    auto rows_1 = fm.getCSVData(5, 41, 1, 10, -1);
    auto rows_10 = fm.getCSVData(5, 41, 10, 100, -1);
    auto rows_100 = fm.getCSVData(5, 41, 100, 1000, -1);
    std::vector<std::vector<CSVRow>> groups = { rows_1, rows_10, rows_100};
    std::vector<CSVRow> rows = FileManager::combineCSV(groups);

    // Step 3: Get Q2 weights
    Weights q2Weights(rows, WeightInitMethod::LUMI_CSV, "src/eicQuickSim/en_lumi.csv");

    
    std::cout << "Q2=1.01 --> " << q2Weights.getWeight(1.01) << std::endl;
    std::cout << "Q2=10.01 --> " << q2Weights.getWeight(10.01) << std::endl;
    std::cout << "Q2=100.01 --> " << q2Weights.getWeight(100.01) << std::endl;

    // Write the CSV with weights to a new file.
    const std::string outputPath = "artifacts/test06_5x41.csv";
    if (!q2Weights.exportCSVWithWeights(rows, outputPath)) {
        std::cerr << "Failed to export CSV with weights." << std::endl;
        return 1;
    }
    return 0;
}