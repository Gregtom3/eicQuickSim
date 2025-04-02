#ifndef WEIGHTS_H
#define WEIGHTS_H

#include <vector>
#include <string>
#include <stdexcept>

#include "FileManager.h"

// Initialization method for the weights.
enum class WeightInitMethod { LUMI_CSV, DEFAULT, PRECALCULATED };

class Weights {
public:
    // Constructor.
    // For modes LUMI_CSV and DEFAULT, combinedRows must be provided.
    // For mode PRECALCULATED, combinedRows can be empty and csvFilename is used to load the precalculated weights.
    Weights(const std::vector<CSVRow>& combinedRows, WeightInitMethod initMethod, const std::string &csvFilename = "");
    
    // Returns the weight for a given Q2 value.
    double getWeight(double Q2) const;
    
    // Exports a CSV file with an appended weight column.
    bool exportCSVWithWeights(const std::vector<CSVRow>& rows, const std::string &outFilePath) const;

private:
    // Energy and configuration.
    int energy_e = 0;
    int energy_h = 0;
    
    // Q2 ranges and related data.
    std::vector<double> Q2mins;
    std::vector<double> Q2maxs;
    std::vector<int> Q2entries;
    std::vector<double> Q2xsecs;
    std::vector<double> Q2weights;
    std::vector<double> providedWeights;
    
    // Total cross section, total events, simulated luminosity, and experimental luminosity.
    double totalCrossSection = 0.0;
    long long totalEvents = 0;
    double simulatedLumi = 0.0;
    double experimentalLumi = 0.0;
    
    // Flag indicating if weights were provided directly.
    bool weightsWereProvided = false;

    // Mode for initializing the weights.
    WeightInitMethod initMethod_;

    // Helper functions.
    void calculateUniqueRanges(const std::vector<CSVRow>& rows);
    void calculateEntriesAndXsecs(const std::vector<CSVRow>& rows);
    void determineTotalCrossSection();
    void calculateWeights();
    bool inQ2Range(double value, double minVal, double maxVal, bool inclusiveUpper) const;
    
    // Loads experimental luminosity from a CSV file.
    void loadExperimentalLuminosity(const std::string &lumiCSVFilename);
    // Loads precalculated weights from a CSV file (format: Q2min,Q2max,collisionType,eEnergy,hEnergy,weight).
    void loadPrecalculatedWeights(const std::string &precalcCSVFilename);
};

#endif // WEIGHTS_H
