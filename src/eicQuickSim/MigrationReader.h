#ifndef MIGRATION_READER_H
#define MIGRATION_READER_H

#include <string>
#include <vector>
#include "yaml-cpp/yaml.h"

class MigrationReader {
public:
    // Constructor. Throws runtime_error if the YAML file is missing required keys or has inconsistent binning.
    MigrationReader(const std::string &yamlFilePath);
    
    // Get the energy configuration string.
    std::string getEnergyConfig() const;
    
    // Get the dimension names.
    const std::vector<std::string>& getDimensionNames() const;
    
    // Get total number of dimensions.
    size_t getNumDimensions() const;
    
    // Get the total number of bins (flattened across all dimensions).
    int getTotalBins() const;
    
    // Get the number of bins for a given dimension (0-based index).
    int getNumBinsInDimension(int dimIndex) const;
    
    // Get the bin edges for a given dimension.
    const std::vector<double>& getBinEdges(int dimIndex) const;
    
    // Get the migration response given overall (flat) indices for the true and reco bins.
    double getResponse(int trueFlat, int recoFlat) const;
    
    // Get the migration response given N-dimensional bin indices.
    double getResponse(const std::vector<int>& trueBins, const std::vector<int>& recoBins) const;
    
    // Print a summary of the migration matrix with detailed bin descriptions.
    void printSummary() const;
    
    // Convert a multi-dimensional bin index into a flat (absolute) bin number.
    int getAbsoluteBinNumber(const std::vector<int>& multiIndices) const;
    
    // Predict how many events will migrate from a given true bin (absolute) to all reco bins.
    // The output vector has the same size as the flattened migration response matrix.
    std::vector<double> predictEvents(int trueAbsoluteBin, double events) const;
    
private:
    // Helper: Convert a flat index into a multi-index vector given dims.
    std::vector<int> unflattenIndex(int flatIndex) const;
    
    // Helper: Build a human-readable description for a bin (given its multi-index).
    std::string buildBinDescription(const std::vector<int>& multiIndex) const;
    
    // Data members loaded from YAML:
    std::string energyConfig;
    std::vector<std::string> dimensionNames;  // e.g. {"Q2", "X"}
    std::vector<int> dims;                    // number of bins per dimension
    std::vector<std::vector<double>> binEdges;  // for each dimension, the vector of bin edges
    
    // The migration response matrix, flattened. Dimensions: [totalBins x totalBins].
    std::vector<std::vector<double>> migrationResponse;
    
    // The flattened true counts (if available). Otherwise, zeros.
    std::vector<double> trueCounts;
};

#endif // MIGRATION_READER_H
