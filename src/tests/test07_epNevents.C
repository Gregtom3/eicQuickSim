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

int main() {
    // Step 1: Load CSV data for DIS events.
    FileManager fm("src/eicQuickSim/en_files.csv");
    cout << "Loading CSV data for DIS events for 5x41 configuration.\n";
    const int MAX_EVENTS = 100;
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
    BinningScheme binScheme("src/tests/bins/test07.yaml");
    cout << "Loaded binning scheme for energy config: " << binScheme.getEnergyConfig() << "\n";
    
    // Step 4: Loop over events and add to the internal bin counts.
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
            
            // Prepare a vector of values for each binning dimension (assumed order: Q2, X).
            std::vector<double> values = { dis.Q2, dis.x };
            
            // Add this event to the binning scheme.
            binScheme.addEvent(values, eventWeight);
        }
        root_input.close();
    }
    
    // Step 5: Save the binned, scaled event counts to a CSV file.
    try {
        binScheme.saveCSV("artifacts/test07_epNevents.csv");
        cout << "Saved binned scaled event counts to artifacts/test07_epNevents.csv" << endl;
    } catch (const std::exception &ex) {
        cerr << "Error saving CSV: " << ex.what() << endl;
        return 1;
    }
    
    return 0;
}
