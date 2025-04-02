#include "BinningScheme.h"
#include <yaml-cpp/yaml.h>
#include <sstream>
#include <stdexcept>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <functional>
#include <iterator>
#include <cctype>

// -------------------------
// Helper: trim whitespace
static std::string trim(const std::string &s) {
    auto start = s.begin();
    while (start != s.end() && std::isspace(*start)) start++;
    auto end = s.end();
    do { end--; } while (std::distance(start, end) > 0 && std::isspace(*end));
    return std::string(start, end + 1);
}

// -------------------------
// YAML parsing (unchanged from original)
void BinningScheme::parseYAML(const std::string &yamlFilePath) {
    pathToBinScheme = yamlFilePath;
    YAML::Node config = YAML::LoadFile(yamlFilePath);
    
    // Get the energy configuration.
    if (!config["energy_config"]) {
        throw std::runtime_error("BinningScheme: 'energy_config' key not found in YAML file.");
    }
    energy_config = config["energy_config"].as<std::string>();

    // Get the dimensions.
    if (!config["dimensions"]) {
        throw std::runtime_error("BinningScheme: 'dimensions' key not found in YAML file.");
    }
    
    YAML::Node dims = config["dimensions"];
    if (!dims.IsSequence()) {
        throw std::runtime_error("BinningScheme: 'dimensions' should be a sequence.");
    }
    
    // Loop through each dimension entry.
    for (std::size_t i = 0; i < dims.size(); ++i) {
        Dimension dim;
        
        if (!dims[i]["name"]) {
            std::ostringstream oss;
            oss << "BinningScheme: dimension at index " << i << " is missing the 'name' key.";
            throw std::runtime_error(oss.str());
        }
        dim.name = dims[i]["name"].as<std::string>();

        if (!dims[i]["branch_true"]) {
            std::ostringstream oss;
            oss << "BinningScheme: dimension " << dim.name << " is missing the 'branch_true' key.";
            throw std::runtime_error(oss.str());
        }
        dim.branch_true = dims[i]["branch_true"].as<std::string>();

        if (!dims[i]["branch_reco"]) {
            std::ostringstream oss;
            oss << "BinningScheme: dimension " << dim.name << " is missing the 'branch_reco' key.";
            throw std::runtime_error(oss.str());
        }
        dim.branch_reco = dims[i]["branch_reco"].as<std::string>();

        if (!dims[i]["edges"]) {
            std::ostringstream oss;
            oss << "BinningScheme: dimension " << dim.name << " is missing the 'edges' key.";
            throw std::runtime_error(oss.str());
        }
        YAML::Node edgesNode = dims[i]["edges"];
        if (!edgesNode.IsSequence()) {
            std::ostringstream oss;
            oss << "BinningScheme: dimension " << dim.name << " 'edges' must be a sequence.";
            throw std::runtime_error(oss.str());
        }
        for (std::size_t j = 0; j < edgesNode.size(); ++j) {
            dim.edges.push_back(edgesNode[j].as<double>());
        }
        
        dimensions.push_back(dim);
    }
}

// -------------------------
// CSV parsing: expected CSV format (for ND_CSV) is as follows:
// Header line: 
//    bin1min,bin1max,bin1_branch_true,bin1_branch_reco,bin2min,bin2max,bin2_branch_true,bin2_branch_reco,...
// Each subsequent line defines one ND bin.
void BinningScheme::parseCSV(const std::string &csvFilePath) {
    pathToBinScheme = csvFilePath;
    std::ifstream ifs(csvFilePath);
    if (!ifs.is_open()) {
        throw std::runtime_error("BinningScheme::parseCSV: Unable to open file: " + csvFilePath);
    }

    std::string headerLine;
    if (!std::getline(ifs, headerLine)) {
        throw std::runtime_error("BinningScheme::parseCSV: Empty CSV file.");
    }
    
    // Split header on commas.
    std::vector<std::string> headerTokens;
    std::istringstream headerStream(headerLine);
    std::string token;
    while (std::getline(headerStream, token, ',')) {
        headerTokens.push_back(trim(token));
    }
    
    // Expect 4 columns per dimension.
    if (headerTokens.size() % 4 != 0) {
        throw std::runtime_error("BinningScheme::parseCSV: Header does not contain a multiple of 4 columns.");
    }
    size_t nDim = headerTokens.size() / 4;
    
    // Create Dimension objects based on header.
    dimensions.clear();
    for (size_t i = 0; i < nDim; ++i) {
        Dimension dim;
        // For name, we can remove the "min" suffix from the first column.
        std::string nameToken = headerTokens[i * 4];
        size_t pos = nameToken.find("min");
        if (pos != std::string::npos)
            dim.name = nameToken.substr(0, pos);
        else
            dim.name = "Dimension" + std::to_string(i+1);
        dim.branch_true = headerTokens[i * 4 + 2];
        dim.branch_reco = headerTokens[i * 4 + 3];
        dimensions.push_back(dim);
    }
    
    // Read each CSV row.
    csvBins_.clear();
    std::string line;
    // For each dimension, we will also collect all boundaries.
    std::vector<std::vector<double>> allEdges(nDim);
    while (std::getline(ifs, line)) {
        if (line.empty()) continue;
        std::istringstream lineStream(line);
        std::vector<std::string> tokens;
        while (std::getline(lineStream, token, ',')) {
            tokens.push_back(trim(token));
        }
        if (tokens.size() != nDim * 4) {
            throw std::runtime_error("BinningScheme::parseCSV: Row does not have expected number of columns.");
        }
        ND_CSV_Bin bin;
        bin.minEdges.resize(nDim);
        bin.maxEdges.resize(nDim);
        for (size_t i = 0; i < nDim; ++i) {
            // Columns: 0: min, 1: max (columns 2 and 3 are branch names, assumed constant across rows)
            double minVal = std::stod(tokens[i * 4]);
            double maxVal = std::stod(tokens[i * 4 + 1]);
            bin.minEdges[i] = minVal;
            bin.maxEdges[i] = maxVal;
            // Save the boundaries if not already present.
            if (std::find(allEdges[i].begin(), allEdges[i].end(), minVal) == allEdges[i].end())
                allEdges[i].push_back(minVal);
            if (std::find(allEdges[i].begin(), allEdges[i].end(), maxVal) == allEdges[i].end())
                allEdges[i].push_back(maxVal);
        }
        csvBins_.push_back(bin);
    }
    ifs.close();

    // For each dimension, sort the unique edges.
    for (size_t i = 0; i < nDim; ++i) {
        std::sort(allEdges[i].begin(), allEdges[i].end());
        dimensions[i].edges = allEdges[i];
    }

    // Set energy_config to a default value for CSV.
    energy_config = "ND_CSV";
}

// -------------------------
// Constructor: choose parsing based on type.
BinningScheme::BinningScheme(const std::string& filePath, BinningType type)
    : type_(type) {
    pathToBinScheme = filePath;
    if (type_ == BinningType::RECTANGULAR_YAML) {
        parseYAML(filePath);
    } else if (type_ == BinningType::ND_CSV) {
        parseCSV(filePath);
    }
}

// -------------------------
const std::string& BinningScheme::getEnergyConfig() const {
    return energy_config;
}

const std::vector<BinningScheme::Dimension>& BinningScheme::getDimensions() const {
    return dimensions;
}

// -------------------------
// findBins: dispatch based on type.
std::vector<int> BinningScheme::findBins(const std::vector<double>& values) const {
    if (values.size() != dimensions.size()) {
        throw std::runtime_error("BinningScheme::findBins: Number of values does not match number of dimensions.");
    }
    if (type_ == BinningType::RECTANGULAR_YAML) {
        std::vector<int> binIndices;
        for (size_t i = 0; i < dimensions.size(); ++i) {
            double val = values[i];
            const std::vector<double>& edges = dimensions[i].edges;
            if (val < edges.front() || val >= edges.back()) {
                binIndices.push_back(-1);
            } else {
                auto it = std::upper_bound(edges.begin(), edges.end(), val);
                int index = std::distance(edges.begin(), it) - 1;
                binIndices.push_back(index);
            }
        }
        return binIndices;
    } else { // ND_CSV
        // Attempt to find a matching CSV-defined bin.
        for (size_t i = 0; i < csvBins_.size(); ++i) {
            bool match = true;
            for (size_t d = 0; d < dimensions.size(); ++d) {
                double val = values[d];
                const auto& bin = csvBins_[i];
                if (val < bin.minEdges[d] || val >= bin.maxEdges[d]) {
                    match = false;
                    break;
                }
            }
            if (match) {
                // Map the CSV bin's min edges to bin indices.
                std::vector<int> binIndices;
                for (size_t d = 0; d < dimensions.size(); ++d) {
                    const auto& edges = dimensions[d].edges;
                    auto it = std::find(edges.begin(), edges.end(), csvBins_[i].minEdges[d]);
                    int idx = (it != edges.end()) ? std::distance(edges.begin(), it) : -1;
                    binIndices.push_back(idx);
                }
                return binIndices;
            }
        }
        // If no CSV bin matches, compute the indices per dimension (as in YAML).
        std::vector<int> binIndices;
        for (size_t d = 0; d < dimensions.size(); d++){
            double val = values[d];
            const std::vector<double>& edges = dimensions[d].edges;
            if (val < edges.front() || val >= edges.back()){
                binIndices.push_back(-1);
            } else {
                auto it = std::upper_bound(edges.begin(), edges.end(), val);
                int index = std::distance(edges.begin(), it) - 1;
                binIndices.push_back(index);
            }
        }
        return binIndices;
    }
}

// -------------------------
void BinningScheme::addEvent(const std::vector<double>& values, double eventWeight) {
    std::vector<int> binIndices = findBins(values);
    for (int idx : binIndices) {
        if (idx < 0) return;  // Skip event if any dimension is out of range.
    }
    std::string key = makeBinKey(binIndices);
    binCounts_[key] += eventWeight;
}

// -------------------------
std::string BinningScheme::makeBinKey(const std::vector<int>& bins) const {
    std::ostringstream oss;
    for (size_t i = 0; i < bins.size(); ++i) {
        if (i > 0) oss << "_";
        oss << bins[i];
    }
    return oss.str();
}

// -------------------------
void BinningScheme::saveCSV(const std::string &outFilePath) const {
    std::ofstream ofs(outFilePath);
    if (!ofs.is_open()) {
         throw std::runtime_error("BinningScheme::saveCSV: Unable to open file: " + outFilePath);
    }
    
    // Write header: for each dimension two columns, then "scaled_events".
    for (const auto &dim : dimensions) {
        ofs << dim.name << "_min," << dim.name << "_max,";
    }
    ofs << "scaled_events" << "\n";
    
    if (type_ == BinningType::RECTANGULAR_YAML) {
        // Generate all possible rectangular bin combinations.
        std::vector<std::vector<int>> allBinCombinations;
        std::function<void(size_t, std::vector<int>&)> rec;
        rec = [&](size_t dimIndex, std::vector<int>& current) {
            if (dimIndex == dimensions.size()) {
                allBinCombinations.push_back(current);
                return;
            }
            int nBins = dimensions[dimIndex].edges.size() - 1;
            for (int i = 0; i < nBins; i++) {
                current.push_back(i);
                rec(dimIndex + 1, current);
                current.pop_back();
            }
        };
        std::vector<int> current;
        rec(0, current);
    
        // Write each bin row.
        for (const auto &bins : allBinCombinations) {
            std::string key = makeBinKey(bins);
            double count = 0.0;
            auto it = binCounts_.find(key);
            if (it != binCounts_.end()) {
                count = it->second;
            }
            for (size_t i = 0; i < dimensions.size(); i++) {
                const auto &edges = dimensions[i].edges;
                int binIdx = bins[i];
                if (binIdx < 0 || binIdx >= static_cast<int>(edges.size() - 1))
                    ofs << "NA,NA,";
                else
                    ofs << edges[binIdx] << "," << edges[binIdx+1] << ",";
            }
            ofs << count << "\n";
        }
    } else { // ND_CSV: iterate over each CSV–defined bin.
        for (size_t i = 0; i < csvBins_.size(); ++i) {
            const auto& bin = csvBins_[i];
            // Compute the bin key by mapping each dimension's min edge to its index.
            std::vector<int> binIndices;
            for (size_t d = 0; d < dimensions.size(); ++d) {
                const auto& edges = dimensions[d].edges;
                auto it = std::find(edges.begin(), edges.end(), bin.minEdges[d]);
                int idx = (it != edges.end()) ? std::distance(edges.begin(), it) : -1;
                binIndices.push_back(idx);
            }
            std::string key = makeBinKey(binIndices);
            double count = 0.0;
            auto it = binCounts_.find(key);
            if (it != binCounts_.end()) {
                count = it->second;
            }
            // Write boundaries for each dimension from the CSV bin.
            for (size_t d = 0; d < dimensions.size(); ++d) {
                ofs << bin.minEdges[d] << "," << bin.maxEdges[d] << ",";
            }
            ofs << count << "\n";
        }
    }
    ofs.close();
}

// -------------------------
std::string BinningScheme::getSchemeName() const {
    std::regex re(R"(([^/\\]+)\.[^.]+$)");
    std::smatch match;
    std::string binName;
    if (std::regex_search(pathToBinScheme, match, re)) {
        binName = match[1].str();
    } else {
        binName = pathToBinScheme;
    }
    return binName;
}

// -------------------------
std::vector<std::string> BinningScheme::getReconstructedBranches() const {
    std::vector<std::string> branches;
    for (const auto& dim : dimensions) {
        branches.push_back(dim.branch_reco);
    }
    return branches;
}


