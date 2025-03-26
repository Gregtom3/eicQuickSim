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
    
    // --- Step 3: Create SIDIS histograms for both π⁺ and π⁻ ---
    // (1) xF histograms (existing)
    TH1D* h_xF_piPlus = new TH1D("h_xF_piPlus", "xF Distribution; xF; Weighted Count", 100, -1.0, 1.0);
    TH1D* h_xF_piMinus = new TH1D("h_xF_piMinus", "xF Distribution; xF; Weighted Count", 100, -1.0, 1.0);
    
    // (2) z histograms
    TH1D* h_z_piPlus = new TH1D("h_z_piPlus", "z Distribution; z; Weighted Count", 100, 0.0, 1.0);
    TH1D* h_z_piMinus = new TH1D("h_z_piMinus", "z Distribution; z; Weighted Count", 100, 0.0, 1.0);
    
    // (3) φ (phi) histograms (using radians: -π to π)
    TH1D* h_phi_piPlus = new TH1D("h_phi_piPlus", "#phi Distribution; #phi [rad]; Weighted Count", 100, -3.14, 3.14);
    TH1D* h_phi_piMinus = new TH1D("h_phi_piMinus", "#phi Distribution; #phi [rad]; Weighted Count", 100, -3.14, 3.14);
    
    // (4) pT_lab histograms (assumed range in GeV)
    TH1D* h_pT_lab_piPlus = new TH1D("h_pT_lab_piPlus", "p_{T}^{lab} Distribution; p_{T}^{lab} [GeV]; Weighted Count", 100, 0.0, 5.0);
    TH1D* h_pT_lab_piMinus = new TH1D("h_pT_lab_piMinus", "p_{T}^{lab} Distribution; p_{T}^{lab} [GeV]; Weighted Count", 100, 0.0, 5.0);
    
    // (5) pT_com histograms (assumed range in GeV)
    TH1D* h_pT_com_piPlus = new TH1D("h_pT_com_piPlus", "p_{T}^{com} Distribution; p_{T}^{com} [GeV]; Weighted Count", 100, 0.0, 5.0);
    TH1D* h_pT_com_piMinus = new TH1D("h_pT_com_piMinus", "p_{T}^{com} Distribution; p_{T}^{com} [GeV]; Weighted Count", 100, 0.0, 5.0);
    
    // (6) Additional variable: η histograms (assumed range)
    TH1D* h_eta_piPlus = new TH1D("h_eta_piPlus", "#eta Distribution; #eta; Weighted Count", 100, -5.0, 5.0);
    TH1D* h_eta_piMinus = new TH1D("h_eta_piMinus", "#eta Distribution; #eta; Weighted Count", 100, -5.0, 5.0);
    
    // Set common histogram styles for π⁺
    h_xF_piPlus->SetLineWidth(2);      h_xF_piPlus->SetLineColor(kMagenta);
    h_xF_piPlus->SetFillColorAlpha(kMagenta-9, 0.35);
    h_xF_piPlus->SetStats(0);
    
    h_z_piPlus->SetLineWidth(2);       h_z_piPlus->SetLineColor(kMagenta);
    h_z_piPlus->SetFillColorAlpha(kMagenta-9, 0.35);
    h_z_piPlus->SetStats(0);
    
    h_phi_piPlus->SetLineWidth(2);     h_phi_piPlus->SetLineColor(kMagenta);
    h_phi_piPlus->SetFillColorAlpha(kMagenta-9, 0.35);
    h_phi_piPlus->SetStats(0);
    
    h_pT_lab_piPlus->SetLineWidth(2);  h_pT_lab_piPlus->SetLineColor(kMagenta);
    h_pT_lab_piPlus->SetFillColorAlpha(kMagenta-9, 0.35);
    h_pT_lab_piPlus->SetStats(0);
    
    h_pT_com_piPlus->SetLineWidth(2);  h_pT_com_piPlus->SetLineColor(kMagenta);
    h_pT_com_piPlus->SetFillColorAlpha(kMagenta-9, 0.35);
    h_pT_com_piPlus->SetStats(0);
    
    h_eta_piPlus->SetLineWidth(2);     h_eta_piPlus->SetLineColor(kMagenta);
    h_eta_piPlus->SetFillColorAlpha(kMagenta-9, 0.35);
    h_eta_piPlus->SetStats(0);
    
    // Set common histogram styles for π⁻
    h_xF_piMinus->SetLineWidth(2);      h_xF_piMinus->SetLineColor(kGreen+1);
    h_xF_piMinus->SetFillColorAlpha(kGreen-9, 0.35);
    h_xF_piMinus->SetStats(0);
    
    h_z_piMinus->SetLineWidth(2);       h_z_piMinus->SetLineColor(kGreen+1);
    h_z_piMinus->SetFillColorAlpha(kGreen-9, 0.35);
    h_z_piMinus->SetStats(0);
    
    h_phi_piMinus->SetLineWidth(2);     h_phi_piMinus->SetLineColor(kGreen+1);
    h_phi_piMinus->SetFillColorAlpha(kGreen-9, 0.35);
    h_phi_piMinus->SetStats(0);
    
    h_pT_lab_piMinus->SetLineWidth(2);  h_pT_lab_piMinus->SetLineColor(kGreen+1);
    h_pT_lab_piMinus->SetFillColorAlpha(kGreen-9, 0.35);
    h_pT_lab_piMinus->SetStats(0);
    
    h_pT_com_piMinus->SetLineWidth(2);  h_pT_com_piMinus->SetLineColor(kGreen+1);
    h_pT_com_piMinus->SetFillColorAlpha(kGreen-9, 0.35);
    h_pT_com_piMinus->SetStats(0);
    
    h_eta_piMinus->SetLineWidth(2);     h_eta_piMinus->SetLineColor(kGreen+1);
    h_eta_piMinus->SetFillColorAlpha(kGreen-9, 0.35);
    h_eta_piMinus->SetStats(0);
    
    // --- Step 4: Process each CSVRow and fill all SIDIS histograms ---
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
            
            // Compute DIS kinematics (if needed for other purposes)
            eicQuickSim::Kinematics kin;
            kin.computeDIS(evt);
            
            // Compute SIDIS for π⁺ (pid==211)
            kin.computeSIDIS(evt, 211);
            eicQuickSim::sidisKinematics sidis_piPlus = kin.getSIDISKinematics();
            
            // Compute SIDIS for π⁻ (pid==-211)
            kin.computeSIDIS(evt, -211);
            eicQuickSim::sidisKinematics sidis_piMinus = kin.getSIDISKinematics();
            
            // Fill xF histograms.
            for (double xf_val : sidis_piPlus.xF) {
                h_xF_piPlus->Fill(xf_val, fileWeight);
            }
            for (double xf_val : sidis_piMinus.xF) {
                h_xF_piMinus->Fill(xf_val, fileWeight);
            }
            
            // Fill z histograms.
            for (double z_val : sidis_piPlus.z) {
                h_z_piPlus->Fill(z_val, fileWeight);
            }
            for (double z_val : sidis_piMinus.z) {
                h_z_piMinus->Fill(z_val, fileWeight);
            }
            
            // Fill φ (phi) histograms.
            for (double phi_val : sidis_piPlus.phi) {
                h_phi_piPlus->Fill(phi_val, fileWeight);
            }
            for (double phi_val : sidis_piMinus.phi) {
                h_phi_piMinus->Fill(phi_val, fileWeight);
            }
            
            // Fill pT_lab histograms.
            for (double pt_val : sidis_piPlus.pT_lab) {
                h_pT_lab_piPlus->Fill(pt_val, fileWeight);
            }
            for (double pt_val : sidis_piMinus.pT_lab) {
                h_pT_lab_piMinus->Fill(pt_val, fileWeight);
            }
            
            // Fill pT_com histograms.
            for (double pt_val : sidis_piPlus.pT_com) {
                h_pT_com_piPlus->Fill(pt_val, fileWeight);
            }
            for (double pt_val : sidis_piMinus.pT_com) {
                h_pT_com_piMinus->Fill(pt_val, fileWeight);
            }
            
            // Fill η histograms
            for (double eta_val : sidis_piPlus.eta) {
                h_eta_piPlus->Fill(eta_val, fileWeight);
            }
            for (double eta_val : sidis_piMinus.eta) {
                h_eta_piMinus->Fill(eta_val, fileWeight);
            }
        }
        root_input.close();
    }
    
    // --- Step 5: Draw and save the overlay plots for each kinematic variable ---
    // (1) xF plot
    TCanvas* c1 = new TCanvas("c1", "xF Distribution for #pi^{+} and #pi^{-}", 800, 600);
    c1->SetMargin(0.12, 0.05, 0.12, 0.08);
    h_xF_piMinus->Draw("HIST");
    h_xF_piPlus->Draw("HIST SAME");
    TLegend* leg1 = new TLegend(0.65, 0.70, 0.88, 0.88);
    leg1->AddEntry(h_xF_piPlus, "#pi^{+}", "l");
    leg1->AddEntry(h_xF_piMinus, "#pi^{-}", "l");
    leg1->Draw();
    c1->RedrawAxis();
    c1->SaveAs("artifacts/test05_xF_piOverlay.png");
    
    // (2) z plot
    TCanvas* c2 = new TCanvas("c2", "z Distribution for #pi^{+} and #pi^{-}", 800, 600);
    c2->SetMargin(0.12, 0.05, 0.12, 0.08);
    h_z_piMinus->Draw("HIST");
    h_z_piPlus->Draw("HIST SAME");
    TLegend* leg2 = new TLegend(0.65, 0.70, 0.88, 0.88);
    leg2->AddEntry(h_z_piPlus, "#pi^{+}", "l");
    leg2->AddEntry(h_z_piMinus, "#pi^{-}", "l");
    leg2->Draw();
    c2->RedrawAxis();
    c2->SaveAs("artifacts/test05_z_piOverlay.png");
    
    // (3) φ plot
    TCanvas* c3 = new TCanvas("c3", "#phi Distribution for #pi^{+} and #pi^{-}", 800, 600);
    c3->SetMargin(0.12, 0.05, 0.12, 0.08);
    h_phi_piMinus->Draw("HIST");
    h_phi_piPlus->Draw("HIST SAME");
    TLegend* leg3 = new TLegend(0.65, 0.70, 0.88, 0.88);
    leg3->AddEntry(h_phi_piPlus, "#pi^{+}", "l");
    leg3->AddEntry(h_phi_piMinus, "#pi^{-}", "l");
    leg3->Draw();
    c3->RedrawAxis();
    c3->SaveAs("artifacts/test05_phi_piOverlay.png");
    
    // (4) pT_lab plot
    TCanvas* c4 = new TCanvas("c4", "p_{T}^{lab} Distribution for #pi^{+} and #pi^{-}", 800, 600);
    c4->SetMargin(0.12, 0.05, 0.12, 0.08);
    h_pT_lab_piMinus->Draw("HIST");
    h_pT_lab_piPlus->Draw("HIST SAME");
    TLegend* leg4 = new TLegend(0.65, 0.70, 0.88, 0.88);
    leg4->AddEntry(h_pT_lab_piPlus, "#pi^{+}", "l");
    leg4->AddEntry(h_pT_lab_piMinus, "#pi^{-}", "l");
    leg4->Draw();
    c4->RedrawAxis();
    c4->SaveAs("artifacts/test05_pT_lab_piOverlay.png");
    
    // (5) pT_com plot
    TCanvas* c5 = new TCanvas("c5", "p_{T}^{com} Distribution for #pi^{+} and #pi^{-}", 800, 600);
    c5->SetMargin(0.12, 0.05, 0.12, 0.08);
    h_pT_com_piMinus->Draw("HIST");
    h_pT_com_piPlus->Draw("HIST SAME");
    TLegend* leg5 = new TLegend(0.65, 0.70, 0.88, 0.88);
    leg5->AddEntry(h_pT_com_piPlus, "#pi^{+}", "l");
    leg5->AddEntry(h_pT_com_piMinus, "#pi^{-}", "l");
    leg5->Draw();
    c5->RedrawAxis();
    c5->SaveAs("artifacts/test05_pT_com_piOverlay.png");
    
    // (6) η plot
    TCanvas* c6 = new TCanvas("c6", "#eta Distribution for #pi^{+} and #pi^{-}", 800, 600);
    c6->SetMargin(0.12, 0.05, 0.12, 0.08);
    h_eta_piMinus->Draw("HIST");
    h_eta_piPlus->Draw("HIST SAME");
    TLegend* leg6 = new TLegend(0.65, 0.70, 0.88, 0.88);
    leg6->AddEntry(h_eta_piPlus, "#pi^{+}", "l");
    leg6->AddEntry(h_eta_piMinus, "#pi^{-}", "l");
    leg6->Draw();
    c6->RedrawAxis();
    c6->SaveAs("artifacts/test05_eta_piOverlay.png");
    
    std::cout << "Saved overlay plots for xF, z, #phi, pT_{lab}, pT_{com} and #eta to the artifacts directory.\n";
    return 0;
}
