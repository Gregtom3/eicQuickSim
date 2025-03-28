#ifndef BINNINGSCHEME_H
#define BINNINGSCHEME_H

#include <string>
#include <vector>

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

private:
    std::string energy_config;
    std::vector<Dimension> dimensions;
};

#endif // BINNINGSCHEME_H
