#ifndef FILEDATASUMMARY_H
#define FILEDATASUMMARY_H

#include <vector>
#include "FileManager.h"  // for CSVRow

/**
 * The FileDataSummary class can compute various aggregate info about a
 * set of files. For now, it just computes the total number of events.
 */
class FileDataSummary {
public:
    FileDataSummary() = default;
    ~FileDataSummary() = default;

    /**
     * Return the sum of nEvents for the given set of CSVRow objects.
     */
    long long getTotalEvents(const std::vector<CSVRow>& rows) const;
};

#endif // FILEDATASUMMARY_H
