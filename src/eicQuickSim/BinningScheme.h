#ifndef BINNINGSCHEME_H
#define BINNINGSCHEME_H

#include <string>
#include <vector>
#include <unordered_map>

class BinningScheme {
public:
    // Each dimension corresponds to one entry in the YAML "dimensions" list.
    struct Dimension {
        std::string name;
        std::string branch_true;
        std::string branch_reco;
        std::vector<double> edges;
    };

    // Constructor: reads a YAML file and parses the binning scheme.
    BinningScheme(const std::string& yamlFilePath);

    // Accessors.
    const std::string& getEnergyConfig() const;
    const std::vector<Dimension>& getDimensions() const;

    // Given a vector of values (one per dimension), returns a vector of bin indices.
    // For each dimension, if the value is out of [edge0, last_edge) range, returns -1.
    std::vector<int> findBins(const std::vector<double>& values) const;

    // Add an event to the internal bin counts.
    // This method finds the appropriate bin (via findBins) and adds the given eventWeight.
    // If any value is out of range (i.e. a bin index is -1), the event is skipped.
    void addEvent(const std::vector<double>& values, double eventWeight);

    // Save the internal binned event counts to a CSV file.
    // The CSV file will have columns: for each dimension, two columns (e.g., Q2_min, Q2_max),
    // followed by "scaled_events". The CSV will include all possible bins (even those with zero events).
    void saveCSV(const std::string &outFilePath) const;

    // Utility: given a vector of bin indices, return a string key (e.g., "2_5").
    std::string makeBinKey(const std::vector<int>& bins) const;

private:
    std::string energy_config;
    std::vector<Dimension> dimensions;

    // Internal storage of binned event counts.
    std::unordered_map<std::string, double> binCounts_;

    // Helper to load and parse the YAML file.
    void parseYAML(const std::string &yamlFilePath);
};

#endif // BINNINGSCHEME_H
