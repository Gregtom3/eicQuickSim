#ifndef FILEDATASUMMARY_H
#define FILEDATASUMMARY_H

#include <vector>
#include "FileManager.h" 

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
     * Return a list of weights, one per row
     */

     std::vector<double> getWeights(const std::vector<CSVRow>& rows) const;
     
private:

    bool checkUniformEnergy(const std::vector<CSVRow>& rows) const;

    bool isContained(int m1, int M1, int m2, int M2) const {
        return (m1 >= m2 && M1 <= M2);
    }

};

#endif // FILEDATASUMMARY_H
