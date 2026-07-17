#include <TFile.h>
#include <TTree.h>
#include <TH2.h>
#include <TF1.h>
#include <TStyle.h>
#include <TCanvas.h>
#include <vector>
#include <iostream>
#include <unordered_map>
#include <fstream>
#include <TKey.h>

int main() {
    TFile *fin = new TFile("./build/sorted_by_OM.root", "READ");

    TIter next(fin->GetListOfKeys());
    TKey *key;

    std::map<int, std::pair<double, double>> baseline_map;

    while ((key = (TKey*)next())){
        TObject *obj = key->ReadObj();

        if (obj->InheritsFrom(TTree::Class())) {
            TTree *tree = (TTree*)obj;

            // Вхідні змінні
            int16_t om_num;
            int32_t run, event, ht, lt;
            std::vector<int16_t>* waveform = nullptr;

            tree->SetBranchAddress("om_num", &om_num);
            tree->SetBranchAddress("run_id", &run);
            tree->SetBranchAddress("event_id", &event);
            tree->SetBranchAddress("high_threshold", &ht);
            tree->SetBranchAddress("low_threshold", &lt);
            tree->SetBranchAddress("waveform", &waveform);

            // Вихід
            TH1D *h1 = new TH1D("h1", "Baseline 1D Hist;X-axis", 5000, 0, 5000);
            // h2->SetStats(0);

            int nEntries = tree->GetEntries();

            for (int i = 0; i < nEntries; i++) {

                tree->GetEntry(i);

                if ((ht != 0) || (lt != 0 )) continue;

                int n = waveform->size();

                for (int j = 0; j < n; j++) {
                    h1->Fill(waveform->at(j));
                }
            }
            if (h1->GetEntries() == 0){
                for (int i = 0; i < nEntries; i++) {

                    tree->GetEntry(i);

                    for (int j = 0; j < 220; j++) {
                        h1->Fill(waveform->at(j));
                    }
                }
            }
            int max_bin = h1->GetMaximumBin();
            double baseline = h1->GetBinCenter(max_bin);

            double xmin = h1->FindFirstBinAbove(0) - 10;
            double xmax = h1->FindLastBinAbove(0) + 10;
            h1->GetXaxis()->SetRangeUser(xmin, xmax);

            TF1 *gaus = new TF1("gaus", "gaus", xmin, xmax);
            h1->Fit(gaus, "R");
            gStyle->SetOptFit(0111);

            baseline_map[om_num] = {gaus->GetParameter(1), gaus->GetParameter(2)};

            TCanvas *c1 = new TCanvas("c1", "1D Hist", 800, 600);
            h1->Draw();
            gaus->Draw("same");
            c1->SaveAs(Form("./build/1D baselines histos/OM%d_1D_baseline_histo.png", om_num));
            
            delete c1;
            delete h1;
        }
    }
    fin->Close();

    std::ofstream out("OM_baselines_sorted.txt");
    out << "OM\tBaseline\tStd\n";
    for (auto &p : baseline_map) {
        out << p.first << "\t" << p.second.first << "\t" << p.second.second << "\n";
    }
    out.close();

    std::cout << "Done! Baselines calculated and sorted.\n";

    return 0;
}