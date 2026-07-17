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
            // Reading baselines
    std::ifstream in("OM_baselines_sorted.txt");

    std::map<int16_t, double> baselines;

    std::string header;
    std::getline(in, header);

    int16_t om_num;
    double mean, sigma;

    while (in >> om_num >> mean >> sigma) {
        baselines[om_num] = mean;
    }

    in.close();

    TFile *fin = new TFile("./build/sorted_by_OM.root", "READ");

    TIter next(fin->GetListOfKeys());
    TKey *key;

    std::map<int, std::pair<double, double>> baseline_map;

    while ((key = (TKey*)next())){
        TObject *obj = key->ReadObj();

        if (obj->InheritsFrom(TTree::Class())) {
            TTree *tree = (TTree*)obj;

            // Вхідні змінні
            int32_t run, event, ht, lt;
            std::vector<int16_t>* waveform = nullptr;

            tree->SetBranchAddress("om_num", &om_num);
            tree->SetBranchAddress("run_id", &run);
            tree->SetBranchAddress("event_id", &event);
            tree->SetBranchAddress("high_threshold", &ht);
            tree->SetBranchAddress("low_threshold", &lt);
            tree->SetBranchAddress("waveform", &waveform);

            // Вихід
            TH2F *histo = new TH2F(
                Form("h2_om_%d", om_num),
                Form("OM%d baseline (substracted BL);Time;Amplitude", om_num),
                1024, 0, 6.4E-6,
                200, -1e2, 1e2
            );
            histo->SetStats(0);

            double ymin = 1e9;
            double ymax = -1e9;

            tree->GetEntry(0);

            double baseline = baselines[om_num];
            int nEntries = tree->GetEntries();

            for (int i = 0; i < nEntries; i++) {
                tree->GetEntry(i);
                if ((ht == 1) || (lt == 1)) continue;
                int n = waveform->size();
                for (int j = 0; j < n; j++) {
                    double timestamp = j * 6.25E-9;
                    double value = waveform->at(j) - baseline;
                    if (value > ymax) ymax = value;
                    if (value < ymin) ymin = value;
                    histo->Fill(timestamp, value);
                }
            }
            if (histo->GetEntries() == 0){
                for (int i = 0; i < nEntries; i++) {

                    tree->GetEntry(i);

                    for (int j = 0; j < 220; j++) {
                        double timestamp = j * 6.25E-9;
                        double value = waveform->at(j) - baseline;
                        if (value > ymax) ymax = value;
                        if (value < ymin) ymin = value;
                        histo->Fill(timestamp, value);
                    }
                }
                double xmax = 220*6.25E-9;
                histo->GetXaxis()->SetRangeUser(0, xmax);

            }
            ymax += 20;
            ymin -= 20;
            histo->GetYaxis()->SetRangeUser(ymin, ymax);
            TCanvas *c1 = new TCanvas("c1", "2D Hist", 800, 600);
            histo->Draw("COLZ");
            c1->SaveAs(Form("./build/2D_baselines_histos_subBL/OM%d_2D_baseline_subBL.png", om_num));
            delete c1;
        }

    }
}