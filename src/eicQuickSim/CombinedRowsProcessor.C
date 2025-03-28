#include "CombinedRowsProcessor.h"
#include <iostream>
#include <cmath>
#include <string>
#include <vector>

std::vector<CSVRow> CombinedRowsProcessor::getCombinedRows(const std::string& energyConfig,
                                                            int numFiles,
                                                            int maxEvents,
                                                            const std::string& collisionType) {
    // Parse the energy configuration (e.g., "5x41").
    size_t pos = energyConfig.find("x");
    if (pos == std::string::npos) {
        std::cerr << "Invalid energy configuration format. Expected format: NxM (e.g., 5x41)" << std::endl;
        return std::vector<CSVRow>();
    }
    int beamEnergy1 = std::stoi(energyConfig.substr(0, pos));
    int beamEnergy2 = std::stoi(energyConfig.substr(pos + 1));

    // Choose the proper CSV file for the file list based on collision type.
    std::string fileListCSV;
    if (collisionType == "ep") {
        fileListCSV = "src/eicQuickSim/ep_files.csv";
    } else if (collisionType == "en") {
        fileListCSV = "src/eicQuickSim/en_files.csv";
    } else {
        std::cerr << "Invalid collision type. Expected 'ep' or 'en'." << std::endl;
        return std::vector<CSVRow>();
    }

    // Instantiate the FileManager.
    FileManager fm(fileListCSV);
    std::vector<std::vector<CSVRow>> groups;

    // Build the groups based on the collision type.
    if (collisionType == "ep") {
        auto rows_1 = fm.getCSVData(beamEnergy1, beamEnergy2, 1, 100000, numFiles, maxEvents);
        auto rows_10 = fm.getCSVData(beamEnergy1, beamEnergy2, 10, 100000, numFiles, maxEvents);
        auto rows_100 = fm.getCSVData(beamEnergy1, beamEnergy2, 100, 100000, numFiles, maxEvents);
        if (beamEnergy1 != 5) {
            auto rows_1000 = fm.getCSVData(beamEnergy1, beamEnergy2, 1000, 100000, numFiles, maxEvents);
            groups = { rows_1, rows_10, rows_100, rows_1000 };
        } else {
            groups = { rows_1, rows_10, rows_100 };
        }
    } else if (collisionType == "en") {
        auto rows_1 = fm.getCSVData(beamEnergy1, beamEnergy2, 1, 10, numFiles, maxEvents);
        auto rows_10 = fm.getCSVData(beamEnergy1, beamEnergy2, 10, 100, numFiles, maxEvents);
        auto rows_100 = fm.getCSVData(beamEnergy1, beamEnergy2, 100, 1000, numFiles, maxEvents);
        if (beamEnergy1 != 5) {
            auto rows_1000 = fm.getCSVData(beamEnergy1, beamEnergy2, 1000, 100000, numFiles, maxEvents);
            groups = { rows_1, rows_10, rows_100, rows_1000 };
        } else {
            groups = { rows_1, rows_10, rows_100 };
        }
    }

    // Combine the groups into a single vector and return.
    return FileManager::combineCSV(groups);
}
