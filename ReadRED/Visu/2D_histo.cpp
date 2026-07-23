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
    // int16_t om_num;
    // int32_t run, event, ht, lt;
    std::vector<double>* waveform = nullptr;

    // tree->SetBranchAddress("run_id", &run);
    // tree->SetBranchAddress("event_id", &event);
    // tree->SetBranchAddress("om_num", &om_num);
    // tree->SetBranchAddress("high_threshold", &ht);
    // tree->SetBranchAddress("low_threshold", &lt);
    tree->SetBranchAddress("waveform", &waveform);

    TH2F *h2 = new TH2F("h2", "Example 2D Hist;X-axis;Y-axis", 1024, 0, 6.4E-6, 700, 2900, 3600);
    h2->SetStats(0);

    int nEntries = tree->GetEntries();

    for (int i = 0; i < nEntries; i++) {

        tree->GetEntry(i);

        int n = waveform->size();

        for (int j = 0; j < n; j++) {
            double timestamp = j * 6.25E-9;
            h2->Fill(timestamp, waveform->at(j));
        }
    }

    std::cout << "Name: " << h2->GetName() << std::endl;
    std::cout << "Title: " << h2->GetTitle() << std::endl;
    std::cout << "Entries: " << h2->GetEntries() << std::endl;
    std::cout << "NbinsX: " << h2->GetNbinsX() << std::endl;
    std::cout << "NbinsY: " << h2->GetNbinsY() << std::endl;

    TCanvas *c1 = new TCanvas("c1", "2D Hist", 800, 600);
    h2->Draw("COLZ");
    c1->SaveAs("2D_histo.png");

    fin->Close();

    std::cout << "Saved " << nEntries << " graphs\n";
    return 0;
}