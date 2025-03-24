#ifndef FILEMANAGER_H
#define FILEMANAGER_H

#include <string>
#include <vector>
#include <unordered_map>

/**
 * Simple struct holding parsed info about a file entry:
 * - The original file path (e.g. /volatile/eic/.../q2_100to1000/..._run098.ab.hepmc3.tree.root)
 * - The energy configuration (like "18x275")
 * - The Q² range (like "100to1000")
 * - The run number (e.g. 98)
 */
struct FileInfo {
    std::string fullPath;
    std::string energyConfig;
    std::string q2Range;
    int         runNumber;
};

/**
 * Key used to look up groups of files in a map.
 * This pairs the energy configuration (e.g. "18x275")
 * with the Q² range (e.g. "100to1000").
 */
struct EnergyQ2Key {
    std::string energy;
    std::string q2Range;

    // Equality operator for use in an unordered_map.
    bool operator==(const EnergyQ2Key &other) const {
        return (energy == other.energy && q2Range == other.q2Range);
    }
};

/**
 * Hash functor that allows us to use EnergyQ2Key as a key
 * in an unordered_map. Combines the hash of the energy and q2Range.
 */
struct EnergyQ2KeyHash {
    std::size_t operator()(const EnergyQ2Key &k) const {
        // Basic string hash combination
        static std::hash<std::string> hasher;
        auto h1 = hasher(k.energy);
        auto h2 = hasher(k.q2Range);
        // Mix them up to reduce collisions
        return h1 ^ (h2 + 0x9e3779b97f4a7c15ULL + (h1 << 6) + (h1 >> 2));
    }
};

/**
 * The FileManager class reads a list of root file paths (e.g. from "en_files.dat"),
 * parses each path to extract metadata, and stores them in a lookup table.
 * Clients can request subsets of files based on energy config, Q² range, and file count.
 */
class FileManager {
public:
    /**
     * @param filename Path to a file containing lines of absolute ROOT file paths.
     *                 Example line:
     *                   /volatile/eic/EPIC/EVGEN/.../18x275/q2_100to1000/...run098.ab.hepmc3.tree.root
     */
    FileManager(const std::string &filename);

    /**
     * Get up to nFiles matching the specified energy config (e.g. "18x275") and
     * Q² range (e.g. "100to1000"). If nFiles <= 0 or exceeds the total matching count,
     * return ALL matching files.
     *
     * @param energyConfig  String describing beam energies (like "18x275").
     * @param q2Range       String describing Q² bracket (like "100to1000").
     * @param nFilesRequested How many files to retrieve. Negative or larger than available => get them all.
     * @return A vector of full paths with "root://dtn-eic.jlab.org/" prefixed if found. Empty if none found.
     */
    std::vector<std::string> getFiles(const std::string &energyConfig,
                                      const std::string &q2Range,
                                      int nFilesRequested) const;

private:
    /**
     * Map from (energy, q2Range) -> list of FileInfo objects.
     * This allows us to quickly find files relevant to a specific beam setup & Q² range.
     */
    std::unordered_map<EnergyQ2Key, std::vector<FileInfo>, EnergyQ2KeyHash> fileMap_;

    /**
     * Internal helper that takes a single line (file path) and extracts:
     *  - energyConfig (like "18x275")
     *  - q2Range (like "100to1000")
     *  - runNumber
     *  - fullPath (the original line)
     */
    FileInfo parseFileLine(const std::string &line) const;
};

#endif // FILEMANAGER_H
