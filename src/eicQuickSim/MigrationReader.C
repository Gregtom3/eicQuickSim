#include "MigrationReader.h"
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <vector>
#include <string>
#include <cstdlib>

using namespace std;

MigrationReader::MigrationReader(const std::string &yamlFilePath) {
    YAML::Node config = YAML::LoadFile(yamlFilePath);
    
    // Check for required keys.
    if (!config["energy_config"])
        throw runtime_error("YAML file missing 'energy_config'.");
    if (!config["dimensions"])
        throw runtime_error("YAML file missing 'dimensions'.");
    
    energyConfig = config["energy_config"].as<string>();
    YAML::Node dimensions = config["dimensions"];
    if (!dimensions["names"] || !dimensions["dims"] || !dimensions["bin_edges"])
        throw runtime_error("YAML file 'dimensions' must contain 'names', 'dims', and 'bin_edges' keys.");
    
    dimensionNames = dimensions["names"].as<vector<string>>();
    dims = dimensions["dims"].as<vector<int>>();
    
    // Load bin edges for each dimension (keyed by dimension name).
    for (size_t i = 0; i < dimensionNames.size(); i++) {
        YAML::Node edgesNode = dimensions["bin_edges"][ dimensionNames[i] ];
        vector<double> edges;
        for (size_t j = 0; j < edgesNode.size(); j++) {
            edges.push_back(edgesNode[j].as<double>());
        }
        // Check: number of edges must equal dims[i] + 1.
        if (edges.size() != dims[i] + 1)
            throw runtime_error("Mismatch in number of bin edges for dimension " + dimensionNames[i]);
        binEdges.push_back(edges);
    }
    
    // Compute total number of bins (flattened) for true (and reco) space.
    int totalBins = 1;
    for (size_t i = 0; i < dims.size(); i++) {
        totalBins *= dims[i];
    }
    
    // Read the migration response matrix.
    YAML::Node mResp = config["migration_response"];
    if (mResp.size() != (unsigned)totalBins)
        throw runtime_error("Mismatch in migration_response matrix row count.");
    
    migrationResponse.resize(totalBins, vector<double>(totalBins, 0.0));
    for (int i = 0; i < totalBins; i++) {
        YAML::Node row = mResp[i];
        if (row.size() != (unsigned)totalBins) {
            ostringstream oss;
            oss << "Row " << i << " of migration_response has wrong size.";
            throw runtime_error(oss.str());
        }
        for (int j = 0; j < totalBins; j++) {
            migrationResponse[i][j] = row[j].as<double>();
        }
    }
    
    // Load true_counts if available; otherwise, fill with zeros.
    if (config["true_counts"]) {
        YAML::Node trueC = config["true_counts"];
        if (trueC.size() != (unsigned)totalBins)
            throw runtime_error("Mismatch in true_counts size.");
        trueCounts.resize(totalBins, 0.0);
        for (int i = 0; i < totalBins; i++)
            trueCounts[i] = trueC[i].as<double>();
    } else {
        trueCounts.resize(totalBins, 0.0);
    }
}

string MigrationReader::getEnergyConfig() const {
    return energyConfig;
}

const vector<string>& MigrationReader::getDimensionNames() const {
    return dimensionNames;
}

size_t MigrationReader::getNumDimensions() const {
    return dims.size();
}

int MigrationReader::getTotalBins() const {
    int total = 1;
    for (auto d : dims)
        total *= d;
    return total;
}

int MigrationReader::getNumBinsInDimension(int dimIndex) const {
    if (dimIndex < 0 || dimIndex >= (int)dims.size())
        throw out_of_range("Dimension index out of bounds");
    return dims[dimIndex];
}

const vector<double>& MigrationReader::getBinEdges(int dimIndex) const {
    if (dimIndex < 0 || dimIndex >= (int)binEdges.size())
        throw out_of_range("Dimension index out of bounds");
    return binEdges[dimIndex];
}

vector<vector<double>> MigrationReader::getAllBinEdges() const {
    return binEdges;
}

double MigrationReader::getResponse(int trueFlat, int recoFlat) const {
    int total = getTotalBins();
    if (trueFlat < 0 || trueFlat >= total || recoFlat < 0 || recoFlat >= total)
        throw out_of_range("Flat bin index out of bounds");
    return migrationResponse[trueFlat][recoFlat];
}

double MigrationReader::getResponse(const vector<int>& trueBins, const vector<int>& recoBins) const {
    if (trueBins.size() != dims.size() || recoBins.size() != dims.size())
        throw invalid_argument("Number of indices must match number of dimensions");
    
    int flatTrue = 0, flatReco = 0, multiplier = 1;
    for (int d = dims.size() - 1; d >= 0; d--) {
        if (trueBins[d] < 0 || trueBins[d] >= dims[d])
            throw out_of_range("True bin index out of range in dimension " + to_string(d));
        if (recoBins[d] < 0 || recoBins[d] >= dims[d])
            throw out_of_range("Reco bin index out of range in dimension " + to_string(d));
        flatTrue += trueBins[d] * multiplier;
        flatReco += recoBins[d] * multiplier;
        multiplier *= dims[d];
    }
    return getResponse(flatTrue, flatReco);
}

int MigrationReader::getAbsoluteBinNumber(const vector<int>& multiIndices) const {
    if (multiIndices.size() != dims.size())
        throw invalid_argument("Number of indices must match number of dimensions");
    
    int flatIndex = 0, multiplier = 1;
    for (int d = dims.size() - 1; d >= 0; d--) {
        if (multiIndices[d] < 0 || multiIndices[d] >= dims[d])
            throw out_of_range("Dimension index out of range in dimension " + to_string(d));
        flatIndex += multiIndices[d] * multiplier;
        multiplier *= dims[d];
    }
    return flatIndex;
}

vector<double> MigrationReader::predictEvents(int trueAbsoluteBin, double events) const {
    int total = getTotalBins();
    if (trueAbsoluteBin < 0 || trueAbsoluteBin >= total)
        throw out_of_range("True bin absolute index out of bounds");
    
    vector<double> prediction(total, 0.0);
    // For each reco bin, predicted events = (response percentage / 100) * events.
    for (int j = 0; j < total; j++) {
        prediction[j] = migrationResponse[trueAbsoluteBin][j] / 100.0 * events;
    }
    return prediction;
}

vector<int> MigrationReader::unflattenIndex(int flatIndex) const {
    int total = getTotalBins();
    if (flatIndex < 0 || flatIndex >= total)
        throw out_of_range("Flat index out of range");
    vector<int> indices(dims.size(), 0);
    for (int d = dims.size() - 1; d >= 0; d--) {
        indices[d] = flatIndex % dims[d];
        flatIndex /= dims[d];
    }
    return indices;
}

string MigrationReader::buildBinDescription(const vector<int>& multiIndex) const {
    if (multiIndex.size() != dims.size())
        throw invalid_argument("Multi-index size does not match number of dimensions");
    ostringstream oss;
    for (size_t d = 0; d < multiIndex.size(); d++) {
        double low = binEdges[d][multiIndex[d]];
        double high = binEdges[d][multiIndex[d] + 1];
        oss << "(" << low << " < " << dimensionNames[d] << " < " << high << ")";
        if (d != multiIndex.size() - 1)
            oss << " && ";
    }
    return oss.str();
}

void MigrationReader::printSummary() const {
    cout << "Energy Configuration: " << energyConfig << endl;
    cout << "Dimensions: ";
    for (size_t i = 0; i < dimensionNames.size(); i++) {
        cout << dimensionNames[i] << " (" << dims[i] << " bins) ";
    }
    cout << endl;
    
    int total = getTotalBins();
    cout << "Total bins (flattened): " << total << endl;
    cout << "Migration Response:" << endl;
    
    // For each true bin and reco bin, print a human-friendly description.
    for (int i = 0; i < total; i++) {
        vector<int> trueMulti = unflattenIndex(i);
        string trueBinDesc = buildBinDescription(trueMulti);
        for (int j = 0; j < total; j++) {
            vector<int> recoMulti = unflattenIndex(j);
            string recoBinDesc = buildBinDescription(recoMulti);
            double resp = migrationResponse[i][j];
            cout << "True: " << trueBinDesc << "  -->  Reco: " << recoBinDesc 
                 << " : " << resp << endl;
        }
        cout << endl;
    }
}
