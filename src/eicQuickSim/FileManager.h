#ifndef FILEMANAGER_H
#define FILEMANAGER_H

#include <string>
#include <vector>
#include <unordered_map>

/**
 * Holds the information from each CSV row:
 *  - filename          e.g. root://dtn-eic.jlab.org//some/path.root
 *  - q2Min, q2Max      integer Q2 range
 *  - eEnergy, hEnergy  integer beam energies
 *  - nEvents           how many events in that file
 *  - crossSectionPb    cross section in pb
 *  - weight            scaling weight
 */
struct CSVRow {
    std::string filename;
    int         q2Min;
    int         q2Max;
    int         eEnergy;
    int         hEnergy;
    int         nEvents;
    double      crossSectionPb;
    double      weight;
};

/**
 * Key structure used to group CSVRows by (e, h, q2Min, q2Max).
 * This allows quick lookups if the user wants "all files for e=10, h=100, Q2=10..100".
 */
struct EnergyQ2Key {
    int eEnergy;
    int hEnergy;
    int q2Min;
    int q2Max;

    bool operator==(const EnergyQ2Key &other) const {
        return (eEnergy == other.eEnergy &&
                hEnergy == other.hEnergy &&
                q2Min   == other.q2Min   &&
                q2Max   == other.q2Max);
    }
};

/**
 * Hash functor for EnergyQ2Key, so we can store it in an unordered_map.
 */
struct EnergyQ2KeyHash {
    std::size_t operator()(const EnergyQ2Key &k) const {
        auto h1 = std::hash<int>()(k.eEnergy);
        auto h2 = std::hash<int>()(k.hEnergy);
        auto h3 = std::hash<int>()(k.q2Min);
        auto h4 = std::hash<int>()(k.q2Max);

        std::size_t combined = h1;
        combined ^= (h2 + 0x9e3779b97f4a7c15ULL + (combined << 6) + (combined >> 2));
        combined ^= (h3 + 0x9e3779b97f4a7c15ULL + (combined << 6) + (combined >> 2));
        combined ^= (h4 + 0x9e3779b97f4a7c15ULL + (combined << 6) + (combined >> 2));

        return combined;
    }
};

/**
 * The FileManager class now reads a CSV file (e.g. summary.csv),
 * and provides a getFiles(...) method to retrieve up to N filenames
 * for the chosen energies and Q² bin.
 */
class FileManager {
public:
    /**
     * @param csvPath Path to a CSV file containing lines of:
     *        filename,q2Min,q2Max,eEnergy,hEnergy,nEvents,crossSectionPb
     */
    FileManager(const std::string &csvPath);

    /**
     * Retrieves up to nFiles matching the given (eEnergy, hEnergy, q2Min, q2Max).
     * If nFiles <= 0 or > total available, returns them all.
     *
     * @return A vector of file paths (filename). Returns empty if no match is found.
     */
    std::vector<std::string> getFiles(int eEnergy, int hEnergy,
                                      int q2Min, int q2Max,
                                      int nFilesRequested) const;
    /**
     * Return up to nRows of CSV data for the given (e,h,q2Min,q2Max).
     * This includes nEvents, crossSection, etc.
     */
     std::vector<CSVRow> getCSVData(int eEnergy, int hEnergy, int q2Min, int q2Max,
        int nRowsRequested, int maxEvents = -1) const;

    /**
     * Combine any number of CSVData vectors into one.
    */
    static std::vector<CSVRow> combineCSV(const std::vector<std::vector<CSVRow>> &dataSets);

private:
    /**
     * Internal map: (e, h, q2Min, q2Max) -> all CSVRows for that group
     */
    std::unordered_map<EnergyQ2Key, std::vector<CSVRow>, EnergyQ2KeyHash> csvMap_;

    /**
     * Helper function to parse a single CSV line and produce a CSVRow.
     */
    CSVRow parseLine(const std::string &line) const;
};

#endif // FILEMANAGER_H
