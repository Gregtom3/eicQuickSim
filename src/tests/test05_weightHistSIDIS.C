#include "FileManager.h"
#include "FileDataSummary.h"
#include "Kinematics.h"

// ROOT & HepMC3 includes:
#include "HepMC3/GenEvent.h"
#include "HepMC3/ReaderRootTree.h"
#include "TLorentzVector.h"
#include "TCanvas.h"
#include "TH1D.h"

#include <iostream>
#include <vector>
using namespace HepMC3;

int main() {
    // --- Step 1: Load CSV data for the 5x41 configuration ---
    FileManager fm("src/eicQuickSim/en_files.csv");
    std::cout << "Loading CSV data for SIDIS analysis (5x41 configuration).\n";
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
    
    // --- Step 3: Create xF histogram for pi+ (pid==211) ---
    TH1D* h_xF = new TH1D("h_xF", "xF Distribution for #pi^{+};xF;Weighted Count", 100, -1.0, 1.0);
    
    // --- Step 4: Process each CSVRow and fill SIDIS xF histogram ---
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
            
            // Create a Kinematics object and compute DIS kinematics.
            eicQuickSim::Kinematics kin;
            kin.computeDIS(evt);
            
            // Now compute SIDIS kinematics for pi+ (pid==211).
            kin.computeSIDIS(evt, 211);
            
            // Retrieve the SIDIS kinematics and fill the xF histogram.
            eicQuickSim::sidisKinematics sidis = kin.getSIDISKinematics();
            for (double xf_val : sidis.xF) {
                h_xF->Fill(xf_val, fileWeight);
            }
        }
        root_input.close();
    }
    
    // --- Step 5: Save the xF histogram ---
    TCanvas* c1 = new TCanvas("c1", "xF Distribution for #pi^{+}", 800, 600);
    c1->SetMargin(0.12, 0.05, 0.12, 0.08);
    h_xF->SetLineWidth(2);
    h_xF->SetLineColor(kMagenta);
    h_xF->SetFillColorAlpha(kMagenta-9, 0.35);
    h_xF->SetStats(0);
    h_xF->Draw("HIST");
    c1->RedrawAxis();
    c1->SaveAs("artifacts/test05_xF_piPlus.png");
    
    std::cout << "Saved xF histogram for #pi^{+} to artifacts/test05_xF_piPlus.png\n";
    return 0;
}
