#include "FileManager.h"
#include "Weights.h"
#include "Kinematics.h"

// ROOT & HepMC3 includes:
#include "HepMC3/GenEvent.h"
#include "HepMC3/ReaderRootTree.h"
#include "TCanvas.h"
#include "TH1D.h"

#include <iostream>
#include <vector>
#include <string>

using namespace HepMC3;
using namespace std;

int main() {
    // --- Step 1: Load CSV data for the 5x41 configuration ---
    FileManager fm("src/eicQuickSim/en_files.csv");
    std::cout << "Loading CSV data for dihadron (π⁺π⁻) analysis (5x41 configuration).\n";
    const int MAX_EVENTS = 10000;
    auto rows_1_10     = fm.getCSVData(5, 41, 1, 10, 3, MAX_EVENTS);
    auto rows_10_100   = fm.getCSVData(5, 41, 10, 100, 3, MAX_EVENTS);
    auto rows_100_1000 = fm.getCSVData(5, 41, 100, 1000, 3, MAX_EVENTS);
    std::vector<std::vector<CSVRow>> groups = { rows_1_10, rows_10_100, rows_100_1000 };
    std::vector<CSVRow> combinedRows = FileManager::combineCSV(groups);
    std::cout << "Combined " << combinedRows.size() << " CSV rows.\n";
    
    // --- Step 2: Setup Q2 weights ---
    Weights q2Weights(combinedRows, WeightInitMethod::LUMI_CSV, "src/eicQuickSim/en_lumi.csv");
    
    // --- Step 3: Create histograms for dihadron (π⁺π⁻) kinematics ---
    TH1D* h_z_pair         = new TH1D("h_z_pair", "Pair z Distribution; z_{pair}; Weighted Count", 100, 0.0, 1.0);
    TH1D* h_phi_h          = new TH1D("h_phi_h", "Pair #phi_{h} Distribution; #phi_{h} [rad]; Weighted Count", 100, -3.14, 3.14);
    TH1D* h_phi_R_method0  = new TH1D("h_phi_R_method0", "Pair #phi_{R} (Method 0) Distribution; #phi_{R} [rad]; Weighted Count", 100, -3.14, 3.14);
    TH1D* h_phi_R_method1  = new TH1D("h_phi_R_method1", "Pair #phi_{R} (Method 1) Distribution; #phi_{R} [rad]; Weighted Count", 100, -3.14, 3.14);
    TH1D* h_pT_lab_pair    = new TH1D("h_pT_lab_pair", "Pair p_{T}^{lab} Distribution; p_{T}^{lab} [GeV]; Weighted Count", 100, 0.0, 5.0);
    TH1D* h_pT_com_pair    = new TH1D("h_pT_com_pair", "Pair p_{T}^{com} Distribution; p_{T}^{com} [GeV]; Weighted Count", 100, 0.0, 5.0);
    TH1D* h_xF_pair        = new TH1D("h_xF_pair", "Pair xF Distribution; xF_{pair}; Weighted Count", 100, -1.0, 1.0);
    TH1D* h_com_th         = new TH1D("h_com_th", "Pair COM Polar Angle Distribution; com_{th} [rad]; Weighted Count", 100, 0.0, 3.14);
    TH1D* h_Mh             = new TH1D("h_Mh", "Pair Invariant Mass Distribution; M_{h} [GeV]; Weighted Count", 100, 0.0, 3.0);
    
    // --- Step 4: Process each CSVRow and fill dihadron histograms ---
    for (size_t i = 0; i < combinedRows.size(); ++i) {
        CSVRow row = combinedRows[i];
        std::string fullPath = row.filename;
        std::cout << "Processing file: " << fullPath << std::endl;
        
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
            
            // Compute DIS kinematics.
            eicQuickSim::Kinematics kin;
            kin.computeDIS(evt);
            eicQuickSim::disKinematics dis = kin.getDISKinematics();
            double eventWeight = q2Weights.getWeight(dis.Q2);
            
            // Compute dihadron kinematics for π⁺ (pid==211) and π⁻ (pid==-211)
            kin.computeDISIDS(evt, 211, -211);
            std::vector<eicQuickSim::dihadronKinematics> dihad = kin.getDISIDSKinematics();
            
            // Fill histograms with pair-level dihadron quantities.
            for (const auto& dih : dihad) {
                h_z_pair->Fill(dih.z_pair, eventWeight);
                h_phi_h->Fill(dih.phi_h, eventWeight);
                h_phi_R_method0->Fill(dih.phi_R_method0, eventWeight);
                h_phi_R_method1->Fill(dih.phi_R_method1, eventWeight);
                h_pT_lab_pair->Fill(dih.pT_lab_pair, eventWeight);
                h_pT_com_pair->Fill(dih.pT_com_pair, eventWeight);
                h_xF_pair->Fill(dih.xF_pair, eventWeight);
                h_com_th->Fill(dih.com_th, eventWeight);
                h_Mh->Fill(dih.Mh, eventWeight);
            }
        }
        root_input.close();
    }
    
    // --- Step 5: Draw and save the dihadron kinematic histograms ---
    TCanvas* c1 = new TCanvas("c1", "Pi+ Pi- Dihadron z Distribution", 800, 600);
    h_z_pair->Draw("HIST");
    c1->SaveAs("artifacts/test08_dihad_z_pair.png");
    
    TCanvas* c2 = new TCanvas("c2", "Pi+ Pi- Dihadron #phi_{h} Distribution", 800, 600);
    h_phi_h->Draw("HIST");
    c2->SaveAs("artifacts/test08_dihad_phi_h.png");
    
    TCanvas* c3 = new TCanvas("c3", "Pi+ Pi- Dihadron #phi_{R} (Method 0) Distribution", 800, 600);
    h_phi_R_method0->Draw("HIST");
    c3->SaveAs("artifacts/test08_dihad_phi_R_method0.png");
    
    TCanvas* c4 = new TCanvas("c4", "Pi+ Pi- Dihadron #phi_{R} (Method 1) Distribution", 800, 600);
    h_phi_R_method1->Draw("HIST");
    c4->SaveAs("artifacts/test08_dihad_phi_R_method1.png");
    
    TCanvas* c5 = new TCanvas("c5", "Pi+ Pi- Dihadron p_{T}^{lab} Distribution", 800, 600);
    h_pT_lab_pair->Draw("HIST");
    c5->SaveAs("artifacts/test08_dihad_pT_lab_pair.png");
    
    TCanvas* c6 = new TCanvas("c6", "Pi+ Pi- Dihadron p_{T}^{com} Distribution", 800, 600);
    h_pT_com_pair->Draw("HIST");
    c6->SaveAs("artifacts/test08_dihad_pT_com_pair.png");
    
    TCanvas* c7 = new TCanvas("c7", "Pi+ Pi- Dihadron xF Distribution", 800, 600);
    h_xF_pair->Draw("HIST");
    c7->SaveAs("artifacts/test08_dihad_xF_pair.png");
    
    TCanvas* c8 = new TCanvas("c8", "Pi+ Pi- Dihadron COM Polar Angle Distribution", 800, 600);
    h_com_th->Draw("HIST");
    c8->SaveAs("artifacts/test08_dihad_com_th.png");
    
    TCanvas* c9 = new TCanvas("c9", "Pi+ Pi- Dihadron Invariant Mass Distribution", 800, 600);
    h_Mh->Draw("HIST");
    c9->SaveAs("artifacts/test08_dihad_Mh.png");
    
    std::cout << "Saved dihadron kinematic histograms to the artifacts directory." << std::endl;
    return 0;
}
