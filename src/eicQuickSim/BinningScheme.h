#ifndef BINNINGSCHEME_H
#define BINNINGSCHEME_H

#include <string>
#include <vector>
#include <unordered_map>
#include <regex>

class BinningScheme {
public:
    // Define the two types of binning schemes.
    enum class BinningType {
        RECTANGULAR_YAML,
        ND_CSV
    };

    // Each dimension (for YAML or CSV) carries a name and branch info.
    // For YAML the "edges" are taken directly from the file.
    // For CSV the edges are built as the unique sorted set of boundaries from the CSV.
    struct Dimension {
        std::string name;
        std::string branch_true;
        std::string branch_reco;
        std::vector<double> edges;
    };

    // Structure for an individually defined ND bin (CSV type).
    struct ND_CSV_Bin {
        std::vector<double> minEdges;  // One per dimension.
        std::vector<double> maxEdges;  // One per dimension.
    };

    // Constructor: reads a file (YAML or CSV) and parses the binning scheme.
    BinningScheme(const std::string& filePath, BinningType type = BinningType::RECTANGULAR_YAML);

    // Accessors.
    const std::string& getEnergyConfig() const;
    const std::vector<Dimension>& getDimensions() const;

    // Given a vector of values (one per dimension), returns a vector of bin indices.
    // For each dimension, if the value is out of range, returns -1.
    std::vector<int> findBins(const std::vector<double>& values) const;

    // Add an event to the internal bin counts.
    void addEvent(const std::vector<double>& values, double eventWeight);

    // Save the internal binned event counts to a CSV file.
    // For YAML the CSV contains all rectangular bins,
    // for CSV it writes one line per ND bin.
    void saveCSV(const std::string &outFilePath) const;

    // Utility: given a vector of bin indices, return a string key (e.g., "2_5").
    std::string makeBinKey(const std::vector<int>& bins) const;

    // Get the file name (without directory and extension).
    std::string getSchemeName() const;

    // Get the reconstructed branch names (one per dimension).
    std::vector<std::string> getReconstructedBranches() const;
    
private:
    std::string pathToBinScheme; // Path to input file (YAML or CSV)
    std::string energy_config;   // For YAML, read from file. For CSV, may be set to a default.
    std::vector<Dimension> dimensions;

    // For ND_CSV type: store the individual bin definitions.
    std::vector<ND_CSV_Bin> csvBins_;

    // Internal storage of binned event counts.
    std::unordered_map<std::string, double> binCounts_;

    // Binning scheme type.
    BinningType type_;

    // Helpers to load and parse the input file.
    void parseYAML(const std::string &filePath);
    void parseCSV(const std::string &filePath);
};

#endif // BINNINGSCHEME_H
