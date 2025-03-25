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
    FileDataSummary summarizer;
    int totalEvents = summarizer.getTotalEvents(rows);

    std::cout << "Got " << rows.size() << " rows, total events = " << totalEvents << std::endl;

    // Step 4: Test total cross section
    // Obtain Q2 1..10 and Q2 10..100 files

    std::cout << "Ask for all CSV data for e=10, h=100, Q2=1..10" << std::endl;
    auto rows_1_10 = fm.getCSVData(10,100,1,10,-1);
    int totalCrossSection_1_10 = summarizer.getTotalCrossSection(rows_1_10);
    std::cout << "Got " << rows_1_10.size() << " rows, total cross section = " << totalCrossSection_1_10 << std::endl;

    std::cout << "Ask for all CSV data for e=10, h=100, Q2=10..100" << std::endl;
    auto rows_10_100 = fm.getCSVData(10,100,10,100,-1);
    int totalCrossSection_10_100 = summarizer.getTotalCrossSection(rows_10_100);
    std::cout << "Got " << rows_10_100.size() << " rows, total cross section = " << totalCrossSection_10_100 << std::endl;

    // Step 5: Combine rows_1_10 and rows_10_100
    std::vector<std::vector<CSVRow>> all { rows_1_10, rows_10_100};
    auto rows_1_100 = FileManager::combineCSV(all);
    int totalCrossSection_1_100 = summarizer.getTotalCrossSection(rows_1_100);
    std::cout << "Got " << rows_1_100.size() << " rows, total cross section = " << totalCrossSection_1_100 << std::endl;

    if (totalCrossSection_1_10 + totalCrossSection_10_100 != totalCrossSection_1_100)
    {
            std::cout << "ERROR: totalCrossSection_1_10 + totalCrossSection_10_100 != totalCrossSection_1_100" << std::endl;
    }

    return 0;
}
