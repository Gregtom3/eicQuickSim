#ifndef FILEDATASUMMARY_H
#define FILEDATASUMMARY_H

#include <vector>
#include <unordered_map>
#include "FileManager.h" 

/**
 * The FileDataSummary class can compute various aggregate info about a
 * set of files.
 */
class FileDataSummary {
public:
    explicit FileDataSummary(const std::string& expLumiCSV);
    ~FileDataSummary() = default;

    /**
     * Return the sum of nEvents for the given set of CSVRow objects.
     */
    int getTotalEvents(const std::vector<CSVRow>& rows) const;

    /**
     * Integrated cross section, skipping contained Q² intervals.
     */
     double getTotalCrossSection(const std::vector<CSVRow>& rows) const;

    /**
     * Compute the total "simulated luminosity" by summing:
     *   nEvents_i/crossSection_i
     * for each file i in rows. Requires a uniform (eEnergy, hEnergy).
     */
     double getTotalLuminosity(const std::vector<CSVRow>& rows) const;

    /**
     * Return a vector of scaled weights, one per row.
     */
     std::vector<double> getWeights(const std::vector<CSVRow>& rows) const;

private:

    bool checkUniformEnergy(const std::vector<CSVRow>& rows) const;

    bool isContained(int m1, int M1, int m2, int M2) const {
        return (m1 >= m2 && M1 <= M2);
    }

    struct EHKey {
        int e;
        int h;
        bool operator==(const EHKey &o) const { return (e == o.e && h == o.h); }
    };
    struct EHKeyHash {
        std::size_t operator()(const EHKey &k) const {
            // Combine e,h
            auto he = std::hash<int>()(k.e);
            auto hh = std::hash<int>()(k.h);
            // simple combine
            return he ^ (hh + 0x9e3779b97f4a7c15ULL + (he << 6) + (he >> 2));
        }
    };

    std::unordered_map<EHKey, double, EHKeyHash> realLumMap_;

    void loadExperimentalLum(const std::string& csvPath);
};

#endif // FILEDATASUMMARY_H
