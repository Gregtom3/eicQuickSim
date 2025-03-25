#include "FileDataSummary.h"
#include <iostream>
#include <algorithm>
#include <cmath>

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
 * If the rows have mixed energies, return 0.0. Otherwise, sum(nEvents_i * crossSection_i).
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
        // each file i => nEvents_i * crossSection_i
        totalLumi += (static_cast<double>(r.nEvents) * r.crossSectionPb);
    }
    return totalLumi;
}