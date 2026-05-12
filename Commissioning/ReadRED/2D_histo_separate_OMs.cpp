#include <TFile.h>
#include <TTree.h>
#include <TGraph.h>
#include <TH2.h>
#include <TCanvas.h>
#include <vector>
#include <iostream>

int main() {

    // Відкрити вхідний файл
    TFile *fin = new TFile("./build/waveforms.root", "READ");
    TTree *tree = (TTree*)fin->Get("t");

    // Підключити гілку
    int16_t om_num;
    // int32_t run, event, ht, lt;
    std::vector<int16_t>* waveform = nullptr;

    // tree->SetBranchAddress("run_id", &run);
    // tree->SetBranchAddress("event_id", &event);
    tree->SetBranchAddress("om_num", &om_num);
    // tree->SetBranchAddress("high_threshold", &ht);
    // tree->SetBranchAddress("low_threshold", &lt);
    tree->SetBranchAddress("waveform", &waveform);

    int nEntries = tree->GetEntries();

    for (int OM = 0; OM <= 711; OM++){
        TH2F *histo = new TH2F(
            Form("h2_om_%d", OM),
            Form("OM %d;Time;Amplitude", OM),
            1024, 0, 6.4E-6,
            700, 2900, 3600
        );
        histo->SetStats(0);
        for (int i = 0; i < nEntries; i++) {
            tree->GetEntry(i);    
            if (om_num != OM) continue;
            int n = waveform->size();

            for (int j = 0; j < n; j++) {
                double timestamp = j * 6.25E-9;
                histo->Fill(timestamp, waveform->at(j));
            }
        }
        if (histo->GetEntries() == 0){
            std::cout << "OM " << OM << " has no data!" << std ::endl;
            continue;
        }
        TCanvas *c1 = new TCanvas("c1", "2D Hist", 800, 600);
        histo->Draw("COLZ");
        c1->SaveAs(Form("./build/Histos by OMs/OM%d_2D_histo.png", OM));


    }
    fin->Close();
    return 0;
}