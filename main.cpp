#include <TH1F.h>
#include <TCanvas.h>

int main() {
    TH1F *h = new TH1F("h", "Sample Histogram", 100, -4, 4);
    for (int i = 0; i < 10000; ++i)
        h->Fill(gRandom->Gaus(0, 1));

    TCanvas *c = new TCanvas();
    h->Draw();
    c->SaveAs("hist.png");
    return 0;
}
