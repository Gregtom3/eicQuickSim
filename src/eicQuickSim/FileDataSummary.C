#include "FileDataSummary.h"
#include <iostream>
#include <algorithm>
#include <cmath>

FileDataSummary::FileDataSummary(const std::string &expLumiCSV)
{
    loadExperimentalLum(expLumiCSV);
}

/**
 * Parse the CSV lines, store into realLumMap_ keyed by (e,h).
 */
 void FileDataSummary::loadExperimentalLum(const std::string& csvPath)
 {
     std::ifstream infile(csvPath);
     if (!infile.is_open()) {
         std::cerr << "[FileDataSummary] Warning: could not open " << csvPath << std::endl;
         return;
     }
 
     bool isHeader = true;
     std::string line;
     while (std::getline(infile, line)) {
         if (isHeader) {
             isHeader = false;
             continue; // skip header
         }
         if (line.empty()) continue;
 
         std::stringstream ss(line);
         std::string token;
         std::vector<std::string> fields;
         while (std::getline(ss, token, ',')) {
             fields.push_back(token);
         }
         if (fields.size() < 3) {
             std::cerr << "[FileDataSummary] Malformed row in " << csvPath << ": " << line << std::endl;
             continue;
         }
         try {
             int e = std::stoi(fields[0]);
             int h = std::stoi(fields[1]);
             double lum = std::stod(fields[2]);
             EHKey key { e, h };
             realLumMap_[key] = lum;
         } catch (const std::exception &ex) {
             std::cerr << "[FileDataSummary] parse error: " << ex.what()
                       << " on line: " << line << std::endl;
         }
     }
 }


int FileDataSummary::getTotalEvents(const std::vector<CSVRow>& rows) const
{
    int total = 0;
    for (const auto &r : rows) {
        total += r.nEvents;
    }
    return total;
}


bool FileDataSummary::checkUniformEnergy(const std::vector<CSVRow>& rows) const
{
    if (rows.empty()) {
        return true; // no conflict if empty
    }
    int eEnergy = rows[0].eEnergy;
    int hEnergy = rows[0].hEnergy;
    for (const auto &r : rows) {
        if (r.eEnergy != eEnergy || r.hEnergy != hEnergy) {
            std::cerr << "[FileDataSummary] Error: Mixed energies in row list. "
                      << "Found (" << r.eEnergy << "x" << r.hEnergy << ") vs ("
                      << eEnergy << "x" << hEnergy << ")\n";
            return false;
        }
    }
    return true;
}

/**
 * If the rows have mixed electron/hadron energies, we print an error and return 0.0.
 * Otherwise, we treat each Q² range as an interval. If we have a bigger interval
 * that includes a smaller one, we skip the smaller. We sum up the cross sections
 * for all distinct intervals in the sense of "non-contained" sets.
 *
 * Example: If row includes [1..100] with cross=5 pb, and another row includes
 * [10..100] with cross=3 pb, the total is 5 pb (not 8).
 *
 * If we have [1..10] (2 pb) + [10..100] (3 pb) and no [1..100],
 * the total is 5 (2 + 3) because those intervals abut but do not nest.
 */
 double FileDataSummary::getTotalCrossSection(const std::vector<CSVRow>& rows) const
 {
     if (rows.empty()) {
         return 0.0;
     }
 
    // 1) energy check
    if (!checkUniformEnergy(rows)) {
        return 0.0;
    }
 
     // 2) Build local vector of intervals with: (q2min, q2max, crossPb).
     struct Interval {
         int qmin;
         int qmax;
         double cross;
     };
     std::vector<Interval> intervals;
     intervals.reserve(rows.size());
 
     for (const auto &r : rows) {
         Interval iv { r.q2Min, r.q2Max, r.crossSectionPb };
         intervals.push_back(iv);
     }
 
     // 3) Sort intervals by descending size, i.e. (q2max - q2min).
     //    Tiebreak smaller qmin first (arbitrary choice).
     std::sort(intervals.begin(), intervals.end(), [&](const Interval &a, const Interval &b){
         int sizeA = a.qmax - a.qmin;
         int sizeB = b.qmax - b.qmin;
         if (sizeA == sizeB) {
             return a.qmin < b.qmin; // tie-break
         }
         return sizeA > sizeB; // bigger first
     });
 
     // 4) Keep a "chosen" set. For each interval i in sorted order,
     //    if it is contained by any chosen interval j, skip it.
     //    else, add it to chosen.
     std::vector<Interval> chosen;
     chosen.reserve(intervals.size());
 
     for (auto &iv : intervals) {
         bool skip = false;
         for (auto &ch : chosen) {
             // if this interval is fully inside a chosen interval, skip
             if (isContained(iv.qmin, iv.qmax, ch.qmin, ch.qmax)) {
                 skip = true;
                 break;
             }
         }
         if (!skip) {
             chosen.push_back(iv);
         }
     }
 
     // 5) Sum up the cross from the chosen intervals
     double totalCross = 0.0;
     for (auto &civ : chosen) {
         totalCross += civ.cross;
     }
 
     return totalCross;
 }

 /**
 * If the rows have mixed energies, return 0.0. Otherwise, sum(nEvents_i/crossSection_i).
 */
double FileDataSummary::getTotalLuminosity(const std::vector<CSVRow>& rows) const
{
    if (rows.empty()) {
        return 0.0;
    }
    if (!checkUniformEnergy(rows)) {
        return 0.0;
    }

    double totalLumi = 0.0;
    for (auto &r : rows) {
        // each file i => nEvents_i/crossSection_i
        totalLumi += (static_cast<double>(r.nEvents)/r.crossSectionPb);
    }
    return totalLumi;
}


/**
 * getScaledWeights:
 *   1) Ensure uniform (e,h)
 *   2) Compute totalSimLum = sum_i(nEvents_i/crossSection_i)
 *   3) fraction_i = simLum_i / totalSimLum
 *   4) realLum = from realLumMap_ for (e,h)
 *   5) scaledWeight_i = fraction_i * realLum
 *   Returns a vector of that size. If no realLum found, prints error => returns empty.
 */
std::vector<double> FileDataSummary::getScaledWeights(const std::vector<CSVRow>& rows) const
{
    std::vector<double> weights;
    if (rows.empty()) {
        return weights; // empty
    }
    if (!checkUniformEnergy(rows)) {
        // mismatch => return empty
        return weights;
    }

    // figure out the single (e,h) for these rows
    int e = rows[0].eEnergy;
    int h = rows[0].hEnergy;
    EHKey key { e, h };
    auto it = realLumMap_.find(key);
    if (it == realLumMap_.end()) {
        std::cerr << "[FileDataSummary] No realLum found for e=" << e << ", h=" << h
                  << " in en_lumi.csv\n";
        return weights; // empty
    }
    double realLum = it->second; // experimental luminosity from en_lumi.csv

    // sum up total sim lum
    double totalSimLum = 0.0;
    for (auto &r : rows) {
        totalSimLum += (static_cast<double>(r.nEvents)/r.crossSectionPb);
    }
    if (totalSimLum <= 0.0) {
        std::cerr << "[FileDataSummary] totalSimLum <= 0 => can't scale.\n";
        return weights;
    }

    // build weights
    weights.reserve(rows.size());
    for (auto &r : rows) {
        double thisSimLum = static_cast<double>(r.nEvents)/r.crossSectionPb;
        double fraction   = thisSimLum / totalSimLum;
        double scaledW    = fraction * realLum; 
        weights.push_back(scaledW);
    }

    return weights;
}