#include "FileManager.h"
#include "FileDataSummary.h"

// ROOT & HepMC3 includes:
#include "HepMC3/GenEvent.h"
#include "HepMC3/ReaderRootTree.h"
#include "HepMC3/GenParticle.h"
#include "TLorentzVector.h"
#include "TCanvas.h"
#include "TH1D.h"
#include "TPad.h"

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
    auto weights = summarizer.getWeights(combinedRows);
    if (weights.size() != combinedRows.size()) {
        std::cerr << "Error: Number of scaled weights does not match number of CSV rows.\n";
        return 1;
    }
    
    // -----------------------------------------------------------
    // Step 3: Create a global histogram for Q2 using logarithmic bins
    int nBins = 100;
    double q2Min = 0.1;  // can't start from 0 for log scale
    double q2Max = 1000.0;

    std::vector<double> logBins(nBins + 1);
    double logMin = std::log10(q2Min);
    double logMax = std::log10(q2Max);
    double binWidth = (logMax - logMin) / nBins;

    for (int i = 0; i <= nBins; ++i) {
        logBins[i] = std::pow(10, logMin + i * binWidth);
    }

    TH1D* hQ2 = new TH1D("hQ2", "Q^{2} Distribution;Q^{2} [GeV^{2}];Weighted Event Count", nBins, logBins.data());
    
    // -----------------------------------------------------------
    // Step 4: Loop over each CSVRow.
    // For each file, open it and process its events
    // Compute Q2 from the initial and scattered electron four-momenta.
    for (size_t i = 0; i < combinedRows.size(); ++i) {
        CSVRow row = combinedRows[i];
        double fileWeight = weights[i]; // weight for this file
        
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
    // Step 5: Save the histogram
    TCanvas *c1 = new TCanvas("c1", "Q^{2} Distribution", 800, 600);
    c1->SetLogx();
    c1->SetLogy();
    c1->SetMargin(0.12, 0.05, 0.12, 0.08); // Left, right, bottom, top
    
    hQ2->SetTitle("Q^{2} Distribution (5x41)");
    hQ2->GetXaxis()->SetTitle("Q^{2} [GeV^{2}]");
    hQ2->GetYaxis()->SetTitle("Event Counts");
    
    hQ2->GetXaxis()->CenterTitle();
    hQ2->GetYaxis()->CenterTitle();
    
    hQ2->GetXaxis()->SetTitleSize(0.045);
    hQ2->GetYaxis()->SetTitleSize(0.045);
    hQ2->GetXaxis()->SetLabelSize(0.04);
    hQ2->GetYaxis()->SetLabelSize(0.04);
    
    hQ2->GetXaxis()->SetTitleOffset(1.2);
    hQ2->GetYaxis()->SetTitleOffset(1.3);
    
    hQ2->SetLineWidth(2);
    hQ2->SetLineColor(kBlue + 1);
    hQ2->SetFillColorAlpha(kBlue - 9, 0.35);
    hQ2->SetStats(0); // Hide stats box
    
    hQ2->Draw("HIST"); // clean outline
    c1->RedrawAxis();
    c1->SaveAs("artifacts/test03_Q2hist.png");
    
    std::cout << "Saved histogram to artifacts/test03_Q2hist.png\n";
    return 0;
}
