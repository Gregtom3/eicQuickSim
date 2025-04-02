#include "Weights.h"
#include <algorithm>
#include <iostream>
#include <sstream>
#include <fstream>
#include <stdexcept>
#include <cmath>

using std::cout;
using std::cerr;
using std::endl;

///////////////////////////////////////////////////////////
// Constructor
///////////////////////////////////////////////////////////
Weights::Weights(const std::vector<CSVRow>& combinedRows, WeightInitMethod initMethod, const std::string &csvFilename)
    : initMethod_(initMethod)
{
    // Mode 3: PRECALCULATED weights.
    if (initMethod_ == WeightInitMethod::PRECALCULATED) {
        loadPrecalculatedWeights(csvFilename);
        weightsWereProvided = true;
        return;
    }
    
    // For modes LUMI_CSV and DEFAULT, ensure rows are provided.
    if (combinedRows.empty())
        throw std::runtime_error("Error: No CSV rows provided.");
    
    // Verify that all CSVRows have the same electron and hadron energies.
    energy_e = combinedRows[0].eEnergy;
    energy_h = combinedRows[0].hEnergy;
    for (const auto& row : combinedRows) {
        if (row.eEnergy != energy_e || row.hEnergy != energy_h) {
            throw std::runtime_error("Error: All CSV rows must have the same electron and hadron energies.");
        }
    }
    
    // Process the CSV rows.
    calculateUniqueRanges(combinedRows);
    calculateEntriesAndXsecs(combinedRows);
    determineTotalCrossSection();
    calculateWeights();
    
    // Mode LUMI_CSV: load experimental luminosity from file.
    if (initMethod_ == WeightInitMethod::LUMI_CSV) {
        loadExperimentalLuminosity(csvFilename);
    }
    // Mode DEFAULT: assume experimental luminosity is 1.
    else if (initMethod_ == WeightInitMethod::DEFAULT) {
        experimentalLumi = 1.0;
    }
}

///////////////////////////////////////////////////////////
// Private Helper Functions
///////////////////////////////////////////////////////////
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

void Weights::determineTotalCrossSection() {
    // Case A: If the first unique range fully contains all others.
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
    
    // Case B: If the unique ranges "link" (adjacent ranges are continuous).
    bool caseB = true;
    for (size_t i = 0; i + 1 < Q2mins.size(); i++) {
        if (Q2maxs[i] != Q2mins[i+1]) {
            caseB = false;
            break;
        }
    }
    if (caseB) {
        totalCrossSection = 0.0;
        for (double xs : Q2xsecs)
            totalCrossSection += xs;
        return;
    }
    
    throw std::runtime_error("Error: Q2 ranges do not satisfy either Case A or Case B for determining total cross section.");
}

void Weights::calculateWeights() {
    long long totalEntriesLocal = 0;
    for (auto entry : Q2entries)
        totalEntriesLocal += entry;
    
    totalEvents = totalEntriesLocal;
    double lumiTotal = static_cast<double>(totalEntriesLocal) / totalCrossSection;
    simulatedLumi = lumiTotal;
    
    Q2weights.resize(Q2xsecs.size(), 0.0);
    
    for (size_t i = 0; i < Q2mins.size(); i++) {
        // If a user-specified weight was provided, use it.
        if (i < providedWeights.size() && providedWeights[i] >= 0) {
            Q2weights[i] = providedWeights[i];
            cout << "\tUsing provided weight for Q2 > " << Q2mins[i];
            if (Q2maxs[i] > 0)
                cout << " && Q2 < " << Q2maxs[i];
            cout << ": " << Q2weights[i] << endl;
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
        cout << "\tQ2 > " << Q2mins[i];
        if (Q2maxs[i] > 0)
            cout << " && Q2 < " << Q2maxs[i];
        cout << ":\n\t\tcount    = " << Q2entries[i]
             << "\n\t\txsec     = " << Q2xsecs[i]
             << "\n\t\tweight   = " << Q2weights[i] << endl;
    }
}

bool Weights::inQ2Range(double value, double minVal, double maxVal, bool inclusiveUpper) const {
    if (inclusiveUpper)
        return (value >= minVal && value <= maxVal);
    else
        return (value >= minVal && value < maxVal);
}

void Weights::loadExperimentalLuminosity(const std::string &lumiCSVFilename) {
    std::ifstream fin(lumiCSVFilename);
    if (!fin)
        throw std::runtime_error("Error: Unable to open luminosity CSV file: " + lumiCSVFilename);
    
    std::string line;
    // Read header.
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
        
        if (!std::getline(ss, token, ','))
            continue;
        file_eEnergy = std::stoi(token);
        if (!std::getline(ss, token, ','))
            continue;
        file_hEnergy = std::stoi(token);
        if (!std::getline(ss, token, ','))
            continue;
        expectedLumi = std::stod(token);
        
        if (file_eEnergy == energy_e && file_hEnergy == energy_h) {
            experimentalLumi = expectedLumi;
            found = true;
            cout << "Loaded experimental luminosity: " << experimentalLumi << endl;
            break;
        }
    }
    if (!found)
        throw std::runtime_error("Error: No matching electron/hadron energy found in luminosity CSV file.");
}

void Weights::loadPrecalculatedWeights(const std::string &precalcCSVFilename) {
    std::ifstream fin(precalcCSVFilename);
    if (!fin)
        throw std::runtime_error("Error: Unable to open pre-calculated weights CSV file: " + precalcCSVFilename);
    
    std::string line;
    // Read and ignore header.
    if (!std::getline(fin, line))
        throw std::runtime_error("Error: Pre-calculated weights CSV file is empty.");
    
    // Clear current vectors.
    Q2mins.clear();
    Q2maxs.clear();
    Q2weights.clear();
    
    // Expected format per row:
    // Q2min, Q2max, collisionType, eEnergy, hEnergy, weight
    while (std::getline(fin, line)) {
        if (line.empty())
            continue;
        std::stringstream ss(line);
        std::string token;
        double q2min, q2max, weight;
        std::string collisionType;
        int file_eEnergy, file_hEnergy;
        
        if (!std::getline(ss, token, ','))
            continue;
        q2min = std::stod(token);
        
        if (!std::getline(ss, token, ','))
            continue;
        q2max = std::stod(token);
        
        if (!std::getline(ss, token, ','))
            continue;
        collisionType = token;
        
        if (!std::getline(ss, token, ','))
            continue;
        file_eEnergy = std::stoi(token);
        
        if (!std::getline(ss, token, ','))
            continue;
        file_hEnergy = std::stoi(token);
        
        if (!std::getline(ss, token, ','))
            continue;
        weight = std::stod(token);
        
        // Initialize energies if not already set.
        if (energy_e == 0 && energy_h == 0) {
            energy_e = file_eEnergy;
            energy_h = file_hEnergy;
        }
        
        if (file_eEnergy == energy_e && file_hEnergy == energy_h) {
            std::cout << line << std::endl;
            Q2mins.push_back(q2min);
            Q2maxs.push_back(q2max);
            Q2weights.push_back(weight);
            std::cout << weight << std::endl;
        }
    }
    if (Q2mins.empty() || Q2weights.empty())
        throw std::runtime_error("Error: No matching pre-calculated weights found for the given energy configuration.");
    weightsWereProvided = true;
}

///////////////////////////////////////////////////////////
// Public Member Functions
///////////////////////////////////////////////////////////
double Weights::getWeight(double Q2) const {
    int idx = -1;

    for (size_t i = 0; i < Q2mins.size(); i++) {
        std::cout << "A" << std::endl;
        if (inQ2Range(Q2, Q2mins[i], Q2maxs[i], false)) {
            std::cout << "HERE" << std::endl;
            idx = static_cast<int>(i);
        }
        std::cout << idx << std::endl;
    }
    if (idx < 0)
        idx = 0;
    std::cout << idx << std::endl;
    std::cout << Q2weights[idx] << std::endl;
    double baseWeight = Q2weights[idx];
    if (initMethod_ == WeightInitMethod::PRECALCULATED)
        return baseWeight;
    else
        return baseWeight * (experimentalLumi / simulatedLumi);
}


bool Weights::exportCSVWithWeights(const std::vector<CSVRow>& rows, const std::string &outFilePath) const {
    // Write the main CSV with the appended weight column.
    std::ofstream ofs(outFilePath);
    if (!ofs.is_open()) {
         cerr << "Error: Unable to open output file: " << outFilePath << endl;
         return false;
    }
    // Header for full event information.
    ofs << "filename,Q2_min,Q2_max,electron_energy,hadron_energy,n_events,cross_section_pb,weight\n";
    for (const auto &row : rows) {
         // Determine the Q2 bracket using a value slightly above the minimum.
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

    // Determine the output filename for the pre-calculated weights file.
    std::string outWeightsFilePath = outFilePath;
    size_t pos = outWeightsFilePath.rfind(".");
    if (pos != std::string::npos)
         outWeightsFilePath = outWeightsFilePath.substr(0, pos) + "_weights.csv";
    else
         outWeightsFilePath += "_weights.csv";

    // Write the pre-calculated weights CSV.
    std::ofstream ofs_weights(outWeightsFilePath);
    if (!ofs_weights.is_open()) {
         cerr << "Error: Unable to open output weights file: " << outWeightsFilePath << endl;
         return false;
    }
    // Header for pre-calculated weights.
    ofs_weights << "Q2_min,Q2_max,collisionType,eEnergy,hEnergy,weight\n";
    // For each Q2 bracket, determine the collision type:
    // "ep" if the first matching CSVRow's filename contains "pythia8", otherwise "en".
    for (size_t i = 0; i < Q2mins.size(); i++) {
         std::string collisionType = "en";
         for (const auto &row : rows) {
             if (row.q2Min == static_cast<int>(Q2mins[i]) && row.q2Max == static_cast<int>(Q2maxs[i])) {
                 if (row.filename.find("pythia8") != std::string::npos)
                     collisionType = "ep";
                 else
                     collisionType = "en";
                 break;
             }
         }
         ofs_weights << Q2mins[i] << ","
                     << Q2maxs[i] << ","
                     << collisionType << ","
                     << energy_e << ","
                     << energy_h << ","
                     << Q2weights[i]*(experimentalLumi / simulatedLumi) << "\n";
    }
    ofs_weights.close();
    
    return true;
}