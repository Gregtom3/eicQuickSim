#include "FileManager.h"
#include "FileDataSummary.h"

// ROOT & HepMC3 includes:
#include "HepMC3/GenEvent.h"
#include "HepMC3/ReaderRootTree.h"
#include "HepMC3/GenParticle.h"
#include "TLorentzVector.h"
#include "TCanvas.h"
#include "TH1D.h"

#include <iostream>
#include <vector>
using namespace HepMC3;

int main() {
    // -----------------------------------------------------------
    // Step 1: Load CSV data for the 5x41 configuration over three Q2 ranges.
    // We assume the CSV (en_files.csv) has data for multiple Q2 ranges.
    // For 5x41: electron energy=5 and hadron energy=41.
    FileManager fm("src/eicQuickSim/en_files.csv");
    
    std::cout << "Loading CSV data for Q2 ranges: 1-10, 10-100, and 100-1000 for 5x41.\n";
    const int MAX_EVENTS = 10000;
    auto rows_1_10    = fm.getCSVData(5, 41, 1, 10, 3, MAX_EVENTS);
    auto rows_10_100  = fm.getCSVData(5, 41, 10, 100, 3, MAX_EVENTS);
    auto rows_100_1000 = fm.getCSVData(5, 41, 100, 1000, 3, MAX_EVENTS);
    
    // Combine all rows into one vector.
    std::vector<std::vector<CSVRow>> groups = { rows_1_10, rows_10_100, rows_100_1000 };
    std::vector<CSVRow> combinedRows = FileManager::combineCSV(groups);
    std::cout << "Combined " << combinedRows.size() << " CSV rows.\n";
    
    // -----------------------------------------------------------
    // Step 2: Load experimental luminosity info and compute scaled weights.
    // en_lumi.csv contains: electron_energy,hadron_energy,expected_lumi
    FileDataSummary summarizer("src/eicQuickSim/en_lumi.csv");
    auto scaledWeights = summarizer.getScaledWeights(combinedRows);
    if (scaledWeights.size() != combinedRows.size()) {
        std::cerr << "Error: Number of scaled weights does not match number of CSV rows.\n";
        return 1;
    }
    
    // -----------------------------------------------------------
    // Step 3: Create a global histogram for Q2.
    // For example, we set up a histogram from 0 to 1000 GeV2 with 100 bins.
    TH1D *hQ2 = new TH1D("hQ2", "Q^{2} Distribution;Q^{2} [GeV^{2}];Weighted Event Count", 100, 0, 1000);
    
    // -----------------------------------------------------------
    // Step 4: Loop over each CSVRow.
    // For each file, open it and process its events
    // Compute Q2 from the initial and scattered electron four-momenta.
    for (size_t i = 0; i < combinedRows.size(); ++i) {
        CSVRow row = combinedRows[i];
        double fileWeight = scaledWeights[i]; // scaled weight for this file
        
        // Read filepath from row
        std::string fullPath = row.filename;
        std::cout << "Processing file: " << fullPath << " with weight " << fileWeight << std::endl;
        
        // Open the file using ROOT's ReaderRootTree.
        ReaderRootTree root_input(fullPath);
        if (root_input.failed()) {
            std::cerr << "Failed to open file: " << fullPath << std::endl;
            continue;
        }
        
        int eventsParsed = 0;
        while (!root_input.failed() && eventsParsed < MAX_EVENTS) {
            if(eventsParsed%1000==0){std::cout<<"Event " << eventsParsed << std::endl;}
            GenEvent evt;
            root_input.read_event(evt);
            if (root_input.failed()) break;
            eventsParsed++;
            
            // Get the initial and scattered electron.
            GenParticlePtr initElectron = nullptr;
            GenParticlePtr scatElectron = nullptr;
            const auto& particles = evt.particles();
            for (const auto& particle : particles) {
                int status = particle->status();
                int pid = particle->pid();
                // initial electron: status==4, pid==11
                // scattered electron: status==21, pid==11
                if (status == 4 && pid == 11) {
                    initElectron = particle;
                } else if (status == 21 && pid == 11) {
                    scatElectron = particle;
                }
            }
            
            // Only proceed if both electrons are found.
            if (initElectron && scatElectron) {
                // Compute Q2 = -(p_in - p_out)2.
                TLorentzVector p_in, p_out;
                p_in.SetPxPyPzE(initElectron->momentum().px(),
                                initElectron->momentum().py(),
                                initElectron->momentum().pz(),
                                initElectron->momentum().e()
                                );
                p_out.SetPxPyPzE(scatElectron->momentum().px(),
                                scatElectron->momentum().py(),
                                scatElectron->momentum().pz(),
                                scatElectron->momentum().e()
                                );
                TLorentzVector q = p_in - p_out;
                double Q2 = -q.M2();
                
                // Fill the histogram with the Q2 value weighted by the file's scaled weight.
                hQ2->Fill(Q2, fileWeight);
            }
        }
        root_input.close();
    }
    
    // -----------------------------------------------------------
    // Step 5: Save the histogram to a ROOT file so it can be uploaded as an artifact.
    TCanvas *c1 = new TCanvas("c1", "c1", 800, 600);
    hQ2->Draw();
    c1->SaveAs("hQ2.png");
    std::cout << "Saved histogram to hQ2.png\n";
    
    return 0;
}
