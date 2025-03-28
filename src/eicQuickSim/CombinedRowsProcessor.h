#ifndef COMBINED_ROWSPROCESSOR_H
#define COMBINED_ROWSPROCESSOR_H

#include <string>
#include <vector>
#include "FileManager.h"  // Assumes that FileManager.h defines CSVRow.

class CombinedRowsProcessor {
public:
    /**
     * Processes CSV files based on the given arguments and returns the combined CSV rows.
     *
     * @param energyConfig Energy configuration in the format "NxM" (e.g., "5x41").
     * @param numFiles Number of files to read per CSV group.
     * @param maxEvents Maximum events to process from each file.
     * @param collisionType Collision type ("ep" or "en").
     * @return A vector of CSVRow containing the combined rows.
     */
    static std::vector<CSVRow> getCombinedRows(const std::string& energyConfig,
                                               int numFiles,
                                               int maxEvents,
                                               const std::string& collisionType);
};

#endif // COMBINED_ROWSPROCESSOR_H
