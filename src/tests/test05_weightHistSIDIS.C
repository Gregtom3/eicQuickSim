#include "FileManager.h"
#include "FileDataSummary.h"
#include "Kinematics.h"

// ROOT & HepMC3 includes:
#include "HepMC3/GenEvent.h"
#include "HepMC3/ReaderRootTree.h"
#include "TCanvas.h"
#include "TH1D.h"
#include "TLegend.h"

#include <iostream>
#include <vector>
using namespace HepMC3;

int main() {
    // --- Step 1: Load CSV data for the 5x41 configuration ---
    FileManager fm("src/eicQuickSim/en_files.csv");
    std::cout << "Loading CSV data for SIDIS overlay analysis (5x41 configuration).\n";
    const int MAX_EVENTS = 10000;
    auto rows_1_10    = fm.getCSVData(5, 41, 1, 10, 3, MAX_EVENTS);
    auto rows_10_100  = fm.getCSVData(5, 41, 10, 100, 3, MAX_EVENTS);
    auto rows_100_1000 = fm.getCSVData(5, 41, 100, 1000, 3, MAX_EVENTS);
    std::vector<std::vector<CSVRow>> groups = { rows_1_10, rows_10_100, rows_100_1000 };
    std::vector<CSVRow> combinedRows = FileManager::combineCSV(groups);
    std::cout << "Combined " << combinedRows.size() << " CSV rows.\n";
    
    // --- Step 2: Load luminosity info and get scaled weights ---
    FileDataSummary summarizer("src/eicQuickSim/en_lumi.csv");
    auto weights = summarizer.getWeights(combinedRows);
    if (weights.size() != combinedRows.size()) {
        std::cerr << "Error: Number of weights does not match CSV rows.\n";
        return 1;
    }
    
    // --- Step 3: Create xF histograms for π⁺ (pid==211) and π⁻ (pid==-211) ---
    TH1D* h_xF_piPlus = new TH1D("h_xF_piPlus", "xF Distribution; xF; Weighted Count", 100, -1.0, 1.0);
    TH1D* h_xF_piMinus = new TH1D("h_xF_piMinus", "xF Distribution; xF; Weighted Count", 100, -1.0, 1.0);
    
    // Style the histograms
    h_xF_piPlus->SetLineWidth(2);
    h_xF_piPlus->SetLineColor(kMagenta);
    h_xF_piPlus->SetFillColorAlpha(kMagenta-9, 0.35);
    h_xF_piPlus->SetStats(0);
    
    h_xF_piMinus->SetLineWidth(2);
    h_xF_piMinus->SetLineColor(kCyan);
    h_xF_piMinus->SetFillColorAlpha(kCyan-9, 0.35);
    h_xF_piMinus->SetStats(0);
    
    // --- Step 4: Process each CSVRow and fill the xF histograms ---
    for (size_t i = 0; i < combinedRows.size(); ++i) {
        CSVRow row = combinedRows[i];
        double fileWeight = weights[i];
        std::string fullPath = row.filename;
        std::cout << "Processing file: " << fullPath << " with weight " << fileWeight << std::endl;
        
        ReaderRootTree root_input(fullPath);
        if (root_input.failed()) {
            std::cerr << "Failed to open file: " << fullPath << std::endl;
            continue;
        }
        
        int eventsParsed = 0;
        while (!root_input.failed() && eventsParsed < MAX_EVENTS) {
            GenEvent evt;
            root_input.read_event(evt);
            if (root_input.failed()) break;
            eventsParsed++;
            
            // Create one Kinematics object and compute DIS kinematics.
            eicQuickSim::Kinematics kin;
            kin.computeDIS(evt);
            
            // Compute SIDIS for π⁺ (pid==211) and store the results.
            kin.computeSIDIS(evt, 211);
            eicQuickSim::sidisKinematics sidis_piPlus = kin.getSIDISKinematics();
            
            // Compute SIDIS for π⁻ (pid==-211) and store the results.
            kin.computeSIDIS(evt, -211);
            eicQuickSim::sidisKinematics sidis_piMinus = kin.getSIDISKinematics();
            
            // Fill the histograms with xF values.
            for (double xf_val : sidis_piPlus.xF) {
                h_xF_piPlus->Fill(xf_val, fileWeight);
            }
            for (double xf_val : sidis_piMinus.xF) {
                h_xF_piMinus->Fill(xf_val, fileWeight);
            }
        }
        root_input.close();
    }
    
    // --- Step 5: Draw both histograms on the same canvas with a legend ---
    TCanvas* c1 = new TCanvas("c1", "xF Distribution for #pi^{+} and #pi^{-}", 800, 600);
    c1->SetMargin(0.12, 0.05, 0.12, 0.08);
    
    // Draw the π⁺ histogram first.
    h_xF_piPlus->Draw("HIST");
    // Overlay the π⁻ histogram.
    h_xF_piMinus->Draw("HIST SAME");
    
    // Create a legend.
    TLegend* legend = new TLegend(0.65, 0.70, 0.88, 0.88);
    legend->AddEntry(h_xF_piPlus, "#pi^{+}", "l");
    legend->AddEntry(h_xF_piMinus, "#pi^{-}", "l");
    legend->Draw();
    
    c1->RedrawAxis();
    c1->SaveAs("artifacts/test05_xF_piOverlay.png");
    
    std::cout << "Saved overlay xF histogram for #pi^{+} and #pi^{-} to artifacts/test05_xF_piOverlay.png\n";
    return 0;
}
