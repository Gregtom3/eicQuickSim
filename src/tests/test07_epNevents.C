#include "FileManager.h"
#include "Weights.h"
#include "Kinematics.h"
#include "BinningScheme.h"

// ROOT & HepMC3 includes:
#include "HepMC3/GenEvent.h"
#include "HepMC3/ReaderRootTree.h"
#include "HepMC3/GenParticle.h"
#include "TCanvas.h"
#include "TH1D.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <vector>
#include <string>

using namespace HepMC3;
using std::cout;
using std::cerr;
using std::endl;

// Helper: given a vector of bin indices, return a string key (e.g., "2_5").
std::string makeBinKey(const std::vector<int>& bins) {
    std::ostringstream oss;
    for (size_t i = 0; i < bins.size(); ++i) {
        if (i > 0) oss << "_";
        oss << bins[i];
    }
    return oss.str();
}

int main() {
    // Step 1: Load CSV data for DIS events.
    FileManager fm("src/eicQuickSim/en_files.csv");
    cout << "Loading CSV data for DIS events for 5x41 configuration.\n";
    const int MAX_EVENTS = 10000;
    auto rows_1_10    = fm.getCSVData(5, 41, 1, 10, 3, MAX_EVENTS);
    auto rows_10_100  = fm.getCSVData(5, 41, 10, 100, 3, MAX_EVENTS);
    auto rows_100_1000 = fm.getCSVData(5, 41, 100, 1000, 3, MAX_EVENTS);
    std::vector<std::vector<CSVRow>> groups = { rows_1_10, rows_10_100, rows_100_1000 };
    std::vector<CSVRow> combinedRows = FileManager::combineCSV(groups);
    cout << "Combined " << combinedRows.size() << " CSV rows.\n";
    
    // Step 2: Get Q2 weights.
    Weights q2Weights(combinedRows);
    q2Weights.loadExperimentalLuminosity("src/eicQuickSim/en_lumi.csv");
    
    // Step 3: Load BinningScheme from YAML file.
    // For this example, we assume there are two dimensions: Q2 and X.
    BinningScheme binScheme("src/bins/test07.yaml");
    cout << "Loaded binning scheme for energy config: " << binScheme.getEnergyConfig() << "\n";
    
    // Step 4: Loop over all events and fill a map keyed by bin indices.
    // We'll store the sum of scaled event weights for each bin.
    std::unordered_map<std::string, double> binCounts;
    
    for (size_t i = 0; i < combinedRows.size(); ++i) {
        CSVRow row = combinedRows[i];
        std::string fullPath = row.filename;
        cout << "Processing file: " << fullPath << endl;
        
        ReaderRootTree root_input(fullPath);
        if (root_input.failed()) {
            cerr << "Failed to open file: " << fullPath << endl;
            continue;
        }
        
        int eventsParsed = 0;
        while (!root_input.failed() && eventsParsed < MAX_EVENTS) {
            GenEvent evt;
            root_input.read_event(evt);
            if (root_input.failed()) break;
            eventsParsed++;
            
            eicQuickSim::Kinematics kin;
            kin.computeDIS(evt);
            eicQuickSim::disKinematics dis = kin.getDISKinematics();
            
            // Use the Q2 weight as a scaling factor.
            double eventWeight = q2Weights.getWeight(dis.Q2);
            
            // Prepare a vector of values corresponding to each dimension in the binning scheme.
            // Here we assume the dimensions are in the order they appear in the YAML file (e.g. Q2 and X).
            std::vector<double> values;
            values.push_back(dis.Q2); // for dimension "Q2"
            values.push_back(dis.x);  // for dimension "X"
            
            // Find the bin indices for this event.
            std::vector<int> binIndices = binScheme.findBins(values);
            // Skip events that fall outside any dimension.
            bool valid = true;
            for (int idx : binIndices) {
                if (idx < 0) { valid = false; break; }
            }
            if (!valid) continue;
            
            // Construct a key (e.g., "3_7") from the bin indices.
            std::string key = makeBinKey(binIndices);
            binCounts[key] += eventWeight;
        }
        root_input.close();
    }
    
    // Step 5: Write the binned, scaled event counts to a CSV file.
    std::ofstream ofs("artifacts/test07_epNevents.csv");
    if (!ofs.is_open()) {
        cerr << "Error: Unable to open output CSV file for writing." << endl;
        return 1;
    }
    
    // Write a header. For each dimension, output its name, then a final column "scaled_events".
    const auto& dims = binScheme.getDimensions();
    for (size_t i = 0; i < dims.size(); i++) {
         ofs << dims[i].name << "_bin";
         if (i < dims.size()-1)
              ofs << ",";
    }
    ofs << ",scaled_events" << std::endl;
    
    // Write one row per filled bin.
    for (const auto &entry : binCounts) {
         ofs << entry.first << "," << entry.second << std::endl;
    }
    ofs.close();
    
    cout << "Saved binned scaled event counts to artifacts/test07_epNevets.csv" << endl;
    return 0;
}
