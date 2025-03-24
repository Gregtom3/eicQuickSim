#include "FileDataSummary.h"

long long FileDataSummary::getTotalEvents(const std::vector<CSVRow>& rows) const
{
    long long total = 0;
    for (const auto &r : rows) {
        total += r.nEvents;
    }
    return total;
}
