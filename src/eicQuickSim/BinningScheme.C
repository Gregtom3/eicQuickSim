#include "BinningScheme.h"
#include <yaml-cpp/yaml.h>
#include <sstream>
#include <stdexcept>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <functional>

BinningScheme::BinningScheme(const std::string& yamlFilePath) {
    parseYAML(yamlFilePath);
}

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

const std::string& BinningScheme::getEnergyConfig() const {
    return energy_config;
}

const std::vector<BinningScheme::Dimension>& BinningScheme::getDimensions() const {
    return dimensions;
}

std::vector<int> BinningScheme::findBins(const std::vector<double>& values) const {
    if (values.size() != dimensions.size()) {
        throw std::runtime_error("BinningScheme::findBins: Number of values does not match number of dimensions.");
    }
    std::vector<int> binIndices;
    binIndices.reserve(dimensions.size());
    for (size_t i = 0; i < dimensions.size(); ++i) {
        double val = values[i];
        const std::vector<double>& edges = dimensions[i].edges;
        // Check if the value is below the first edge or above/equal to the last edge.
        if (val < edges.front() || val >= edges.back()) {
            binIndices.push_back(-1);
        } else {
            auto it = std::upper_bound(edges.begin(), edges.end(), val);
            int index = std::distance(edges.begin(), it) - 1;
            binIndices.push_back(index);
        }
    }
    return binIndices;
}

std::string BinningScheme::makeBinKey(const std::vector<int>& bins) const {
    std::ostringstream oss;
    for (size_t i = 0; i < bins.size(); ++i) {
        if (i > 0) oss << "_";
        oss << bins[i];
    }
    return oss.str();
}

void BinningScheme::addEvent(const std::vector<double>& values, double eventWeight) {
    // Compute bin indices.
    std::vector<int> binIndices = findBins(values);
    // If any dimension is out-of-range, skip this event.
    for (int idx : binIndices) {
        if (idx < 0) return;
    }
    std::string key = makeBinKey(binIndices);
    binCounts_[key] += eventWeight;
}

void BinningScheme::saveCSV(const std::string &outFilePath) const {
    std::ofstream ofs(outFilePath);
    if (!ofs.is_open()) {
         throw std::runtime_error("BinningScheme::saveCSV: Unable to open file: " + outFilePath);
    }
    
    // Write header.
    // For each dimension, write two columns: <dimension>_min and <dimension>_max.
    // Then, the last column is "scaled_events".
    for (const auto &dim : dimensions) {
        ofs << dim.name << "_min," << dim.name << "_max,";
    }
    ofs << "scaled_events" << "\n";
    
    // Generate all possible bin combinations.
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
    
    // For each possible bin combination, look up the event count (defaulting to 0 if missing).
    for (const auto &bins : allBinCombinations) {
        std::string key = makeBinKey(bins);
        double count = 0.0;
        auto it = binCounts_.find(key);
        if (it != binCounts_.end()) {
            count = it->second;
        }
        // For each dimension, write the bin's min and max edges.
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
    
    ofs.close();
}

std::string BinningScheme::getSchemeName() const {
    std::regex re(R"(([^/\\]+)\.yaml$)");
    std::smatch match;
    std::string binName;
    if (std::regex_search(pathToBinScheme, match, re)) {
        binName = match[1].str();
    } else {
        binName = pathToBinScheme;
    }
    return binName;
}