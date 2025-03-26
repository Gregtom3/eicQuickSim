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
    const std::string outputPath = "artifacts/test06_10x100_Q2_10_100.csv";
    if (!summarizer.exportCSVWithWeights(rows, weights, outputPath)) {
        std::cerr << "Failed to export CSV with weights." << std::endl;
        return 1;
    }

    // Step 4: Re-load the CSV with weights and print out the weights
    std::ifstream infile(outputPath);
    if (!infile.is_open()) {
        std::cerr << "Error opening file: " << outputPath << std::endl;
        return 1;
    }
    
    std::string line;
    bool isHeader = true;
    std::cout << "\nLoaded Weights from CSV:" << std::endl;
    while (std::getline(infile, line)) {
        // Skip header line.
        if (isHeader) {
            isHeader = false;
            continue;
        }
        
        // Use the FileManager's parseLine to parse the CSV line.
        CSVRow parsedRow = fm.parseLine(line);
        std::cout << "File: " << parsedRow.filename
                  << ", Weight: " << parsedRow.weight << std::endl;
    }
    
    infile.close();
    return 0;
}