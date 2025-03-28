#include "BinningScheme.h"
#include <yaml-cpp/yaml.h>
#include <iostream>
#include <stdexcept>
#include <sstream>
#include <algorithm>

BinningScheme::BinningScheme(const std::string& yamlFilePath) {
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
            // Use std::upper_bound to find the first edge greater than val.
            auto it = std::upper_bound(edges.begin(), edges.end(), val);
            // The bin index is the index of the previous edge.
            int index = std::distance(edges.begin(), it) - 1;
            binIndices.push_back(index);
        }
    }
    return binIndices;
}
