#include "FileManager.h"
#include <fstream>
#include <iostream>
#include <sstream> // for std::stringstream

FileManager::FileManager(const std::string &csvPath)
{
    std::ifstream infile(csvPath);
    if (!infile.is_open()) {
        std::cerr << "[FileManager] Error opening CSV: " << csvPath << std::endl;
        return;
    }

    std::string line;
    bool isHeader = true;
    while (std::getline(infile, line)) {
        // If there's a header row, skip it
        if (isHeader) {
            isHeader = false;
            continue;
        }

        // Empty line check
        if (line.empty()) {
            continue;
        }

        // Parse the CSV row
        CSVRow row = parseLine(line);
        // If the parse was invalid, skip
        if (row.filename.empty()) {
            // parseLine might return an empty filename on error
            continue;
        }

        // Build the map key
        EnergyQ2Key key {
            row.eEnergy,
            row.hEnergy,
            row.q2Min,
            row.q2Max
        };

        // Add to the vector for that key
        csvMap_[key].push_back(row);
    }
}

/**
 * Return up to nFiles from the (e, h, q2Min, q2Max) group.
 * If that group doesn't exist, or is empty, we return empty.
 */
std::vector<std::string> FileManager::getFiles(int eEnergy, int hEnergy,
                                               int q2Min, int q2Max,
                                               int nFilesRequested) const
{
    EnergyQ2Key key { eEnergy, hEnergy, q2Min, q2Max };

    auto it = csvMap_.find(key);
    if (it == csvMap_.end()) {
        std::cerr << "[FileManager] No CSV entries for e=" << eEnergy
                  << ", h=" << hEnergy
                  << ", Q2=" << q2Min << ".." << q2Max << std::endl;
        return {};
    }

    const auto &rows = it->second;
    if (rows.empty()) {
        std::cerr << "[FileManager] Found empty group for e=" << eEnergy
                  << ", h=" << hEnergy
                  << ", Q2=" << q2Min << ".." << q2Max << std::endl;
        return {};
    }

    // Decide how many to return
    int total = static_cast<int>(rows.size());
    if (nFilesRequested <= 0 || nFilesRequested > total) {
        nFilesRequested = total;
    }

    // Build the result vector
    std::vector<std::string> results;
    results.reserve(nFilesRequested);
    for (int i = 0; i < nFilesRequested; ++i) {
        results.push_back(rows[i].filename);
    }
    return results;
}

/**
 * Parse a CSV line:
 *   filename, Q2_min, Q2_max, electron_energy, hadron_energy, n_events, cross_section_pb
 */
CSVRow FileManager::parseLine(const std::string &line) const
{
    CSVRow row;
    row.filename.clear(); // make sure default is empty

    std::stringstream ss(line);
    std::string token;

    // We'll read 7 columns
    // If the CSV has more/fewer, handle carefully
    std::vector<std::string> fields;
    while (std::getline(ss, token, ',')) {
        fields.push_back(token);
    }

    if (fields.size() < 7) {
        std::cerr << "[FileManager] Malformed CSV line (need 7 cols): " << line << std::endl;
        return row; // row.filename stays empty => invalid
    }

    try {
        // 0) filename
        row.filename = fields[0];

        // 1) Q2_min
        row.q2Min = std::stoi(fields[1]);

        // 2) Q2_max
        row.q2Max = std::stoi(fields[2]);

        // 3) electron_energy
        row.eEnergy = std::stoi(fields[3]);

        // 4) hadron_energy
        row.hEnergy = std::stoi(fields[4]);

        // 5) n_events
        row.nEvents = std::stoll(fields[5]);

        // 6) cross_section_pb
        row.crossSectionPb = std::stod(fields[6]);
    } catch (const std::exception &e) {
        std::cerr << "[FileManager] CSV parse error: " << e.what()
                  << " on line: " << line << std::endl;
        // Return row with empty filename => invalid
        row.filename.clear();
    }

    return row;
}

/**
    * Return up to nRows of CSV data for the given (e,h,q2Min,q2Max).
    * This includes nEvents, crossSection, etc.
    */
std::vector<CSVRow> FileManager::getCSVData(int eEnergy, int hEnergy,
                                            int q2Min, int q2Max,
                                            int nRowsRequested) const
{
    EnergyQ2Key key { eEnergy, hEnergy, q2Min, q2Max };
    auto it = csvMap_.find(key);
    if (it == csvMap_.end() || it->second.empty()) {
        // Return empty if no match
        return {};
    }
    const auto &rows = it->second;
    int total = static_cast<int>(rows.size());
    if (nRowsRequested <= 0 || nRowsRequested > total) {
        nRowsRequested = total;
    }
    // Copy the first nRowsRequested rows
    return std::vector<CSVRow>(rows.begin(), rows.begin() + nRowsRequested);
}

/**
 * Combine an arbitrary number of CSVRow vectors into one big vector.
 *
 * Example usage:
 *
 * std::vector<CSVRow> v1 = ...;
 * std::vector<CSVRow> v2 = ...;
 * std::vector<CSVRow> v3 = ...;
 *
 * std::vector<std::vector<CSVRow>> multiple = { v1, v2, v3 };
 * auto combined = FileManager::combineCSV(multiple);
 */
static std::vector<CSVRow> FileManager::combineCSV(const std::vector<std::vector<CSVRow>> &dataSets)
 {
     std::vector<CSVRow> result;
     // Reserve space if you want some optimization
     // Calculate total size
     size_t totalSize = 0;
     for (auto &ds : dataSets) {
         totalSize += ds.size();
     }
     result.reserve(totalSize);
 
     // Append each vector in turn
     for (auto &ds : dataSets) {
         result.insert(result.end(), ds.begin(), ds.end());
     }
 
     return result;
 }