#include "Weights.h"
#include <algorithm>
#include <iostream>
#include <sstream>
#include <fstream>
#include <cmath>

using std::cout;
using std::cerr;
using std::endl;

// Constructor: process the CSV rows and compute Q2 ranges, entries, cross sections, and weights.
Weights::Weights(const std::vector<CSVRow>& combinedRows) {
    if (combinedRows.empty())
        throw std::runtime_error("Error: No CSV rows provided.");

    // Verify that all CSVRows have the same eEnergy and hEnergy.
    energy_e = combinedRows[0].eEnergy;
    energy_h = combinedRows[0].hEnergy;
    for (const auto& row : combinedRows) {
        if (row.eEnergy != energy_e || row.hEnergy != energy_h) {
            throw std::runtime_error("Error: All CSV rows must have the same electron and hadron energies.");
        }
    }

    // Build unique Q2 range vectors.
    calculateUniqueRanges(combinedRows);
    // Build Q2 entries, cross sections, and record any provided weights.
    calculateEntriesAndXsecs(combinedRows);
    // Determine total cross section.
    determineTotalCrossSection();
    // Calculate Q2 weights and compute simulated luminosity.
    calculateWeights();
}

// Extract unique Q2 ranges (unique (q2Min, q2Max) pairs) and sort them by q2Min.
void Weights::calculateUniqueRanges(const std::vector<CSVRow>& rows) {
    std::vector<std::pair<int, int>> uniqueRanges;
    for (const auto& row : rows) {
        std::pair<int, int> range(row.q2Min, row.q2Max);
        if (std::find(uniqueRanges.begin(), uniqueRanges.end(), range) == uniqueRanges.end()) {
            uniqueRanges.push_back(range);
        }
    }
    std::sort(uniqueRanges.begin(), uniqueRanges.end(), [](auto a, auto b) {
        return a.first < b.first;
    });
    Q2mins.clear();
    Q2maxs.clear();
    for (const auto& range : uniqueRanges) {
        Q2mins.push_back(static_cast<double>(range.first));
        Q2maxs.push_back(static_cast<double>(range.second));
    }
}

// For each CSVRow, add its nEvents to the appropriate unique Q2 range, record the cross section,
// and if a weight is provided (>= 0), store it in providedWeights.
void Weights::calculateEntriesAndXsecs(const std::vector<CSVRow>& rows) {
    Q2entries.resize(Q2mins.size(), 0);
    Q2xsecs.resize(Q2mins.size(), 0.0);
    providedWeights.resize(Q2mins.size(), -1.0);  // -1 indicates "not provided"
    for (const auto& row : rows) {
        for (size_t i = 0; i < Q2mins.size(); i++) {
            if (Q2mins[i] == row.q2Min && Q2maxs[i] == row.q2Max) {
                Q2entries[i] += row.nEvents;
                // Assume crossSectionPb is the same for all rows in a given range.
                Q2xsecs[i] = row.crossSectionPb;
                // If a user-specified weight is provided and not yet set, record it.
                if (row.weight >= 0 && providedWeights[i] < 0) {
                    providedWeights[i] = row.weight;
                }
                break;
            }
        }
    }
}

void Weights::clearUserProvidedWeights() {
    std::fill(providedWeights.begin(), providedWeights.end(), -1.0);
    weightsWereProvided = false;
    std::cout << "All user provided weights have been cleared." << std::endl;
}

void Weights::updateUserProvidedWeight(double userQ2min, double userQ2max, double userWeight) {
    bool found = false;
    for (size_t i = 0; i < Q2mins.size(); i++) {
        std::cout << Q2mins[i] << " , " << Q2maxs[i] << std::endl;
        if (Q2mins[i] == userQ2min && Q2maxs[i] == userQ2max) {
            providedWeights[i] = userWeight;
            found = true;
            std::cout << "User provided weight " << userWeight
                      << " for Q2 range > " << userQ2min
                      << " && < " << userQ2max << std::endl;
            break;
        }
    }
    if (!found) {
        throw std::runtime_error("Error: Specified Q2 range (" +
                                 std::to_string(userQ2min) + ", " +
                                 std::to_string(userQ2max) + ") not found.");
    }
    weightsWereProvided = true;
}

// Determine the total cross section based on two cases.
// Case A: If the first unique range fully contains all others, use its cross section.
// Case B: If the unique ranges "link" (adjacent ranges are continuous), sum all cross sections.
void Weights::determineTotalCrossSection() {
    bool caseA = true;
    for (size_t i = 1; i < Q2mins.size(); i++) {
        if (!(Q2mins[0] <= Q2mins[i] && Q2maxs[0] >= Q2maxs[i])) {
            caseA = false;
            break;
        }
    }
    if (caseA) {
        totalCrossSection = Q2xsecs[0];
        return;
    }
    bool caseB = true;
    for (size_t i = 0; i + 1 < Q2mins.size(); i++) {
        if (Q2maxs[i] != Q2mins[i+1]) {
            caseB = false;
            break;
        }
    }
    if (caseB) {
        totalCrossSection = 0.0;
        for (double xs : Q2xsecs) {
            totalCrossSection += xs;
        }
        return;
    }
    throw std::runtime_error("Error: Q2 ranges do not satisfy either Case A or Case B for determining total cross section.");
}

// Calculate Q2 weights using a luminosity-based method and compute simulatedLuminosity.
// If a weight was provided by the CSV rows for a given Q2 bracket, use it directly.
void Weights::calculateWeights() {
    long long totalEntriesLocal = 0;
    for (auto entry : Q2entries) {
        totalEntriesLocal += entry;
    }
    totalEvents = totalEntriesLocal;
    double lumiTotal = static_cast<double>(totalEntriesLocal) / totalCrossSection;
    simulatedLumi = lumiTotal; // simulatedLuminosity = totalEvents/totalCrossSection
    Q2weights.resize(Q2xsecs.size(), 0.0);
    
    for (size_t i = 0; i < Q2xsecs.size(); i++) {
        // If a user-specified weight was provided for this Q2 bracket, use it directly.
        if (i < providedWeights.size() && providedWeights[i] >= 0) {
            Q2weights[i] = providedWeights[i];
            std::cout << "\tUsing provided weight for Q2 > " << Q2mins[i];
            if (Q2maxs[i] > 0)
                std::cout << " && Q2 < " << Q2maxs[i];
            std::cout << ": " << Q2weights[i] << std::endl;
            continue;
        }
        
        double lumiThis = 0.0;
        for (size_t j = 0; j < Q2xsecs.size(); j++) {
            if (inQ2Range(Q2mins[i], Q2mins[j], Q2maxs[j], false) &&
                inQ2Range(Q2maxs[i], Q2mins[j], Q2maxs[j], true))
            {
                lumiThis += static_cast<double>(Q2entries[j]) / Q2xsecs[j];
            }
        }
        if (lumiThis == 0.0) {
            throw std::runtime_error("Error: Computed luminosity for a Q2 range is zero.");
        }
        Q2weights[i] = lumiTotal / lumiThis;
        std::cout << "\tQ2 > " << Q2mins[i];
        if (Q2maxs[i] > 0)
            std::cout << " && Q2 < " << Q2maxs[i];
        std::cout << ":" << std::endl;
        std::cout << "\t\tcount    = " << Q2entries[i] << std::endl;
        std::cout << "\t\txsec     = " << Q2xsecs[i] << std::endl;
        std::cout << "\t\tweight   = " << Q2weights[i] << std::endl;
    }
}

// Returns true if value is within [minVal, maxVal) or [minVal, maxVal] if inclusiveUpper==true.
bool Weights::inQ2Range(double value, double minVal, double maxVal, bool inclusiveUpper) const {
    if (inclusiveUpper)
        return (value >= minVal && value <= maxVal);
    else
        return (value >= minVal && value < maxVal);
}

// Loads experimental luminosity from a CSV file.
// The file is expected to have a header and rows with:
// electron_energy,hadron_energy,expected_lumi
// This function finds the row that matches the stored eEnergy and hEnergy.
void Weights::loadExperimentalLuminosity(const std::string& lumiCSVFilename) {
    std::ifstream fin(lumiCSVFilename);
    if (!fin)
        throw std::runtime_error("Error: Unable to open luminosity CSV file: " + lumiCSVFilename);
    
    std::string line;
    // Read header line.
    if (!std::getline(fin, line))
        throw std::runtime_error("Error: Luminosity CSV file is empty.");
    
    bool found = false;
    while (std::getline(fin, line)) {
        if (line.empty())
            continue;
        std::stringstream ss(line);
        std::string token;
        int file_eEnergy, file_hEnergy;
        double expectedLumi;
        
        // Get electron_energy.
        if (!std::getline(ss, token, ','))
            continue;
        file_eEnergy = std::stoi(token);
        // Get hadron_energy.
        if (!std::getline(ss, token, ','))
            continue;
        file_hEnergy = std::stoi(token);
        // Get expected_lumi.
        if (!std::getline(ss, token, ','))
            continue;
        expectedLumi = std::stod(token);
        
        if (file_eEnergy == energy_e && file_hEnergy == energy_h) {
            experimentalLumi = expectedLumi;
            found = true;
            std::cout << "Loaded experimental luminosity: " << experimentalLumi << std::endl;
            break;
        }
    }
    if (!found)
        throw std::runtime_error("Error: No matching electron/hadron energy found in luminosity CSV file.");
}

// Returns the Q2 weight for a given Q2 value, multiplied by (experimentalLumi/simulatedLumi).
double Weights::getWeight(double Q2) const {
    int idx = -1;
    for (size_t i = 0; i < Q2mins.size(); i++) {
        if (inQ2Range(Q2, Q2mins[i], Q2maxs[i], false)) {
            idx = static_cast<int>(i); // if multiple match, the last one wins.
        }
    }
    if (idx < 0)
        idx = 0;
    double baseWeight = Q2weights[idx];
    if(weightsWereProvided==true){
        return baseWeight;
    }
    else{
        return baseWeight * (experimentalLumi / simulatedLumi);
    }
}

// Export CSV with weights.
// For each CSVRow, we compute a weight and write the row with an appended weight column.
bool Weights::exportCSVWithWeights(const std::vector<CSVRow>& rows, const std::string &outFilePath) const {
    std::ofstream ofs(outFilePath);
    if (!ofs.is_open()) {
         std::cerr << "Error: Unable to open output file: " << outFilePath << std::endl;
         return false;
    }
    // Write header.
    ofs << "filename,Q2_min,Q2_max,electron_energy,hadron_energy,n_events,cross_section_pb,weight\n";
    for (const auto &row : rows) {
         // Use just a smidgen infront of the Q2min 
         double theQ2 = row.q2Min + 0.0001;
         double weight = getWeight(theQ2);
         ofs << row.filename << ","
             << row.q2Min << ","
             << row.q2Max << ","
             << row.eEnergy << ","
             << row.hEnergy << ","
             << row.nEvents << ","
             << row.crossSectionPb << ","
             << weight << "\n";
    }
    ofs.close();
    return true;
}
