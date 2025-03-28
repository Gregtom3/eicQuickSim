#include "FileManager.h"
#include "Kinematics.h"
#include "Weights.h"

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
#include <cmath>
using namespace HepMC3;
using std::cout;
using std::endl;

//------------------------------------------------------
// Main analysis code
int main() {
    // Step 1: Load CSV data to get the file list.
    FileManager fm("src/eicQuickSim/ep_files.csv");
    cout << "Loading CSV data for 5x41 configuration.\n";
    const int MAX_EVENTS = 1000;
    auto rows_1 = fm.getCSVData(5, 41, 1, 100000, 3, MAX_EVENTS);
    auto rows_10 = fm.getCSVData(5, 41, 10, 100000, 3, MAX_EVENTS);
    auto rows_100 = fm.getCSVData(5, 41, 100, 100000, 3, MAX_EVENTS);
    std::vector<std::vector<CSVRow>> groups = { rows_1, rows_10, rows_100 };
    std::vector<CSVRow> combinedRows = FileManager::combineCSV(groups);
    cout << "Combined " << combinedRows.size() << " CSV rows.\n";
    
    // Step 2: Get Q2 weights
    Weights q2Weights(combinedRows);
    // Load in experimental luminosity to scale weights
    q2Weights.loadExperimentalLuminosity("src/eicQuickSim/ep_lumi.csv");

    // Step 3: Create global histograms.
    int nBins = 100;
    double q2Min = 0.1;
    double q2Max = 1000.0;
    std::vector<double> logBins(nBins + 1);
    double logMin = std::log10(q2Min);
    double logMax = std::log10(q2Max);
    double binWidth = (logMax - logMin) / nBins;
    for (int i = 0; i <= nBins; ++i) {
        logBins[i] = std::pow(10, logMin + i * binWidth);
    }
    TH1D* hQ2 = new TH1D("hQ2", "Q^{2} Distribution;Q^{2} [GeV^{2}];Weighted Event Count", nBins, logBins.data());
    
    double xMin = 1e-5;
    double xMax = 1;
    std::vector<double> logXBins(nBins + 1);
    double logXMin = std::log10(xMin);
    double logXMax = std::log10(xMax);
    double binXWidth = (logXMax - logXMin) / nBins;
    for (int i = 0; i <= nBins; ++i) {
        logXBins[i] = std::pow(10, logXMin + i * binXWidth);
    }
    TH1D* hX = new TH1D("hX", "Bjorken x Distribution;x;Weighted Event Count", nBins, logXBins.data());
    
    TH1D* hW = new TH1D("hW", "W Distribution;W [GeV];Weighted Event Count", 100, 0.0, 100.0);
    
    // Step 4: Process each file
    for (size_t i = 0; i < combinedRows.size(); ++i) {
        CSVRow row = combinedRows[i];
        std::string fullPath = row.filename;
        cout << "Processing file: " << fullPath << endl;
        
        ReaderRootTree root_input(fullPath);
        if (root_input.failed()) {
            std::cerr << "Failed to open file: " << fullPath << endl;
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
            // Compute the weight for this event based on its Q2
            double eventWeight = q2Weights.getWeight(dis.Q2);
            hQ2->Fill(dis.Q2, eventWeight);
            hX->Fill(dis.x, eventWeight);
            hW->Fill(dis.W, eventWeight);
        }
        root_input.close();
    }
    
    // Step 5: Save the histograms.
    TCanvas *c1 = new TCanvas("c1", "Q^{2} Distribution", 800, 600);
    c1->SetLogx();
    c1->SetLogy();
    c1->SetMargin(0.12, 0.05, 0.12, 0.08);
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
    hQ2->SetStats(0);
    hQ2->Draw("HIST");
    c1->RedrawAxis();
    c1->SaveAs("artifacts/analysis_epDIS_Q2hist.png");
    
    TCanvas *c2 = new TCanvas("c2", "Bjorken x Distribution", 800, 600);
    c2->SetMargin(0.12, 0.05, 0.12, 0.08);
    hX->SetTitle("Bjorken x Distribution (5x41)");
    hX->GetXaxis()->SetTitle("x");
    hX->GetYaxis()->SetTitle("Event Counts");
    hX->GetXaxis()->CenterTitle();
    hX->GetYaxis()->CenterTitle();
    hX->GetXaxis()->SetTitleSize(0.045);
    hX->GetYaxis()->SetTitleSize(0.045);
    hX->GetXaxis()->SetLabelSize(0.04);
    hX->GetYaxis()->SetLabelSize(0.04);
    hX->GetXaxis()->SetTitleOffset(1.2);
    hX->GetYaxis()->SetTitleOffset(1.3);
    hX->SetLineWidth(2);
    hX->SetLineColor(kRed + 1);
    hX->SetFillColorAlpha(kRed - 9, 0.35);
    hX->SetStats(0);
    c2->SetLogx();
    c2->SetLogy();
    hX->Draw("HIST");
    c2->RedrawAxis();
    c2->SaveAs("artifacts/analysis_epDIS_xhist.png");
    
    TCanvas *c3 = new TCanvas("c3", "W Distribution", 800, 600);
    c3->SetMargin(0.12, 0.05, 0.12, 0.08);
    hW->SetTitle("W Distribution (5x41)");
    hW->GetXaxis()->SetTitle("W [GeV]");
    hW->GetYaxis()->SetTitle("Event Counts");
    hW->GetXaxis()->CenterTitle();
    hW->GetYaxis()->CenterTitle();
    hW->GetXaxis()->SetTitleSize(0.045);
    hW->GetYaxis()->SetTitleSize(0.045);
    hW->GetXaxis()->SetLabelSize(0.04);
    hW->GetYaxis()->SetLabelSize(0.04);
    hW->GetXaxis()->SetTitleOffset(1.2);
    hW->GetYaxis()->SetTitleOffset(1.3);
    hW->SetLineWidth(2);
    hW->SetLineColor(kGreen + 1);
    hW->SetFillColorAlpha(kGreen - 9, 0.35);
    hW->SetStats(0);
    hW->Draw("HIST");
    c3->RedrawAxis();
    c3->SaveAs("artifacts/analysis_epDIS_Whist.png");
    
    cout << "Saved histograms to artifacts directory.\n";
    return 0;
}
