#ifndef WEIGHTS_H
#define WEIGHTS_H

#include <vector>
#include <string>
#include <stdexcept>

#include "FileManager.h"

class Weights {
public:
    // Constructor: receives the CSV row data.
    Weights(const std::vector<CSVRow>& combinedRows);

    // Public function to load experimental luminosity from a CSV file.
    // The CSV file is expected to have a header and three columns:
    // electron_energy,hadron_energy,expected_lumi
    void loadExperimentalLuminosity(const std::string& lumiCSVFilename);

    // Returns the weight for a given Q2 value.
    double getWeight(double Q2) const;

    // Export CSV with the computed weight appended to each row.
    // The output CSV will have the same columns as the input plus a final column for weight.
    bool exportCSVWithWeights(const std::vector<CSVRow>& rows, const std::string &outFilePath) const;

    void clearUserProvidedWeights();
    void updateUserProvidedWeight(double userQ2min, double userQ2max, double userWeight);
    
private:
    // Unique vectors for Q2 ranges, event counts, and cross sections.
    std::vector<double> Q2mins;
    std::vector<double> Q2maxs;
    std::vector<long long> Q2entries;
    std::vector<double> Q2xsecs;
    std::vector<double> Q2weights;
    double totalCrossSection; // determined from the Q2 ranges

    std::vector<double> providedWeights;
    bool weightsWereProvided = false;
    
    // For luminosity scaling.
    double experimentalLumi = 1.0;
    double simulatedLumi = 1.0;
    long long totalEvents = 0;
    int energy_e = 0, energy_h = 0; // common electron and hadron energies (must be the same for all CSVRows)

    // Private helper functions.
    void calculateUniqueRanges(const std::vector<CSVRow>& rows);
    void calculateEntriesAndXsecs(const std::vector<CSVRow>& rows);
    void determineTotalCrossSection();
    void calculateWeights();

    // Returns true if 'value' lies within [minVal, maxVal) (or [minVal, maxVal] if inclusiveUpper==true).
    bool inQ2Range(double value, double minVal, double maxVal, bool inclusiveUpper = false) const;
};

#endif // WEIGHTS_H
