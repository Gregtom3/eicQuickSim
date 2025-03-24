#include "FileManager.h"
#include <iostream>
#include <fstream>
#include <regex>

/**
 * Constructs a FileManager by reading every line from the given file.
 * Each line is expected to be a path to a .root file with a structure like:
 *   .../18x275/q2_100to1000/..._run098.ab.hepmc3.tree.root
 * We parse out the (energy, q2Range, runNumber) and store them.
 */
FileManager::FileManager(const std::string &filename)
{
    std::ifstream infile(filename);
    if (!infile.is_open()) {
        std::cerr << "[FileManager] Error: Could not open " << filename << std::endl;
        return;
    }

    std::string line;
    while (std::getline(infile, line)) {
        // Skip lines if they're empty or just whitespace
        if (line.empty()) continue;

        // Parse the line
        FileInfo fi = parseFileLine(line);

        // If parseFileLine returned something sensible, store it in the map
        // (We rely on an "UNKNOWN" check or runNumber < 0 if it fails)
        if (fi.runNumber >= 0 && fi.energyConfig != "UNKNOWN" && fi.q2Range != "UNKNOWN") {
            EnergyQ2Key key { fi.energyConfig, fi.q2Range };
            fileMap_[key].push_back(fi);
        }
    }
}

/**
 * Returns up to nFiles for the specified (energyConfig, q2Range).
 * If you ask for more than exist (or a negative value), we'll just return them all.
 * If the key doesn't exist, we log an error and return an empty vector.
 */
std::vector<std::string> FileManager::getFiles(const std::string &energyConfig,
                                               const std::string &q2Range,
                                               int nFilesRequested) const
{
    // Build the key for lookup
    EnergyQ2Key key { energyConfig, q2Range };

    // See if we have an entry
    auto it = fileMap_.find(key);
    if (it == fileMap_.end()) {
        std::cerr << "[FileManager] Error: No files found for energy="
                  << energyConfig << ", q2Range=" << q2Range << std::endl;
        return {};
    }

    const auto &fileList = it->second;
    if (fileList.empty()) {
        std::cerr << "[FileManager] Warning: Entry found for ("
                  << energyConfig << ", " << q2Range << ") but it's empty?" << std::endl;
        return {};
    }

    // Decide how many to return
    int total = static_cast<int>(fileList.size());
    if (nFilesRequested <= 0 || nFilesRequested > total) {
        nFilesRequested = total;
    }

    // Build the result vector, prepending "root://dtn-eic.jlab.org/"
    // so they can be read from that server
    std::vector<std::string> results;
    results.reserve(nFilesRequested);
    for (int i = 0; i < nFilesRequested; ++i) {
        const auto &fullPath = fileList[i].fullPath;
        results.push_back("root://dtn-eic.jlab.org/" + fullPath);
    }

    return results;
}

/**
 * Attempts to parse a line with a path like:
 *   /volatile/eic/.../18x275/q2_100to1000/..._run098.ab.hepmc3.tree.root
 * using a regex that captures:
 *   - The beam energies: (\d+x\d+)
 *   - The Q2 range: (\d+to\d+)
 *   - The run number: (\d+)
 */
FileInfo FileManager::parseFileLine(const std::string &line) const
{
    FileInfo info;
    info.fullPath = line;
    info.runNumber = -1;      // If parse fails, we leave it negative
    info.energyConfig = "UNKNOWN";
    info.q2Range = "UNKNOWN";

    // Regex to match e.g. "18x275", "q2_100to1000", runNNN
    // Adjust if your real path format differs
    static const std::regex pattern(
        R"(.*\/(\d+x\d+)\/q2_(\d+to\d+)\/.*_run(\d+)\.ab\.hepmc3\.tree\.root$)"
    );

    std::smatch matches;
    if (std::regex_match(line, matches, pattern)) {
        info.energyConfig = matches[1].str(); // e.g. "18x275"
        info.q2Range      = matches[2].str(); // e.g. "100to1000"
        info.runNumber    = std::stoi(matches[3].str()); // e.g. 98
    } else {
        std::cerr << "[FileManager] Warning: Could not parse this line:\n  " << line << std::endl;
    }

    return info;
}
