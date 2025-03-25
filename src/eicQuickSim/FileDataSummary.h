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
    int getTotalEvents(const std::vector<CSVRow>& rows) const;

    /**
     * Computes the "total cross section" from a list of CSVRow entries,
     * respecting the rule that if a larger Q² interval is present, it
     * includes the cross sections of fully nested intervals.
     */
     double getTotalCrossSection(const std::vector<CSVRow>& rows) const;

private:
    /**
    * Helper to check if interval (m1..M1) is contained in (m2..M2).
    */
    bool isContained(int m1, int M1, int m2, int M2) const {
        return (m1 >= m2 && M1 <= M2);
    }

};

#endif // FILEDATASUMMARY_H
