#include "MigrationReader.h"
#include "FileManager.h"
#include "FileDataSummary.h"
#include "Kinematics.h"
#include "HepMC3/ReaderRootTree.h"
#include "HepMC3/GenEvent.h"
#include "TCanvas.h"
#include "TH2D.h"
#include "TStyle.h"
#include "TLegend.h"
#include <iostream>
#include <vector>

using namespace std;
using namespace HepMC3;

int main() {
    // Create a MigrationReader instance from YAML file.
    MigrationReader mr("src/tests/responseMatrices/test07_response_xQ2_5x41.yaml");
    
    // Get bin edges using the built-in function.
    std::vector<std::vector<double>> allEdges = mr.getAllBinEdges();
    if (allEdges.size() < 2) {
        cerr << "Error: Expected at least 2 dimensions in the response YAML." << endl;
        return 1;
    }
    // For this example, assume dimension 0 is "X" and dimension 1 is "Q2".
    std::vector<double> xEdges = allEdges[1];
    std::vector<double> q2Edges = allEdges[0];
    int nBinsX = xEdges.size() - 1;
    int nBinsQ2 = q2Edges.size() - 1;
    
    // Create true and predicted histograms.
    TH2D* hTrue = new TH2D("hTrue", "True (Generated) Distribution; Bjorken x; Q^{2} [GeV^{2}]",
                           nBinsX, &xEdges[0], nBinsQ2, &q2Edges[0]);
    TH2D* hPred = new TH2D("hPred", "Predicted Reconstructed Distribution; Bjorken x; Q^{2} [GeV^{2}]",
                           nBinsX, &xEdges[0], nBinsQ2, &q2Edges[0]);
    
    // Load CSV file info using FileManager.
    FileManager fm("src/eicQuickSim/en_files.csv");
    const int MAX_EVENTS = 10000;
    auto rows_1_10    = fm.getCSVData(5, 41, 1, 10, 3, MAX_EVENTS);
    auto rows_10_100  = fm.getCSVData(5, 41, 10, 100, 3, MAX_EVENTS);
    auto rows_100_1000 = fm.getCSVData(5, 41, 100, 1000, 3, MAX_EVENTS);
    std::vector<std::vector<CSVRow>> groups = { rows_1_10, rows_10_100, rows_100_1000 };
    std::vector<CSVRow> combinedRows = FileManager::combineCSV(groups);
    cout << "Combined " << combinedRows.size() << " CSV rows." << endl;
    
    // Get weights.
    FileDataSummary summarizer("src/eicQuickSim/en_lumi.csv");
    auto weights = summarizer.getWeights(combinedRows);
    if (weights.size() != combinedRows.size()) {
        cerr << "Error: Number of weights does not match CSV rows." << endl;
        return 1;
    }
    
    // Loop over .root files and fill the true histogram.
    for (size_t i = 0; i < combinedRows.size(); ++i) {
        CSVRow row = combinedRows[i];
        double fileWeight = weights[i];
        string fullPath = row.filename;
        cout << "Processing file: " << fullPath << " with weight " << fileWeight << endl;
        
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
            if (dis.Q2 > 0 && dis.x > 0)
                hTrue->Fill(dis.x, dis.Q2, fileWeight);
        }
        root_input.close();
    }
    
    // Use MigrationReader's predictEvents() to fill the predicted histogram.
    int totalBins = mr.getTotalBins();  // flattened total bins
    for (int binX = 1; binX <= nBinsX; binX++) {
        for (int binY = 1; binY <= nBinsQ2; binY++) {
            // Compute flat index assuming row-major order.
            int flatTrue = (binY - 1) * nBinsX + (binX - 1);
            double trueCount = hTrue->GetBinContent(binX, binY);
            std::vector<double> pred = mr.predictEvents(flatTrue, trueCount);
            for (int j = 0; j < totalBins; j++) {
                double predicted = pred[j];
                int recoBinX = (j % nBinsX) + 1;
                int recoBinY = (j / nBinsX) + 1;
                double current = hPred->GetBinContent(recoBinX, recoBinY);
                hPred->SetBinContent(recoBinX, recoBinY, current + predicted);
            }
        }
    }
    
    // Plot side-by-side true and predicted histograms.
    TCanvas* canvas = new TCanvas("canvas", "True vs Predicted Distributions", 1600, 800);
    canvas->Divide(2,1);
    
    canvas->cd(1);
    gPad->SetLogx();
    gPad->SetLogy();
    hTrue->SetTitle("True (Generated) Distribution");
    hTrue->GetXaxis()->SetTitle("Bjorken x");
    hTrue->GetYaxis()->SetTitle("Q^{2} [GeV^{2}]");
    hTrue->Draw("COLZ");
    
    canvas->cd(2);
    gPad->SetLogx();
    gPad->SetLogy();
    hPred->SetTitle("Predicted Reconstructed Distribution");
    hPred->GetXaxis()->SetTitle("Bjorken x");
    hPred->GetYaxis()->SetTitle("Q^{2} [GeV^{2}]");
    hPred->Draw("COLZ");
    
    canvas->SaveAs("artifacts/test08_beforeAndAfter.png");
    cout << "Saved true and predicted histograms to artifacts/test08_beforeAndAfter.png" << endl;
    
    return 0;
}
