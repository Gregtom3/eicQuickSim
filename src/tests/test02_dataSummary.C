#include "FileManager.h"
#include "FileDataSummary.h"
#include <iostream>

int main() {
    // Step 1: Construct the FileManager
    FileManager fm("src/eicQuickSim/en_files.csv");

    // Step 2: Ask for all CSV data for e=10, h=100, Q2=10..100
    auto rows = fm.getCSVData(10, 100, 10, 100, -1);

    // Step 3: Summarize with FileDataSummary
    FileDataSummary summarizer;
    long long totalEvents = summarizer.getTotalEvents(rows);

    std::cout << "Got " << rows.size() << " rows, total events = " << totalEvents << std::endl;

    return 0;
}
