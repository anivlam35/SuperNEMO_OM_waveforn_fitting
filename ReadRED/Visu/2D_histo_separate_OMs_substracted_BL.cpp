#include <TFile.h>
#include <TTree.h>
#include <TGraph.h>
#include <TH2.h>
#include <TCanvas.h>
#include <vector>
#include <iostream>
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

    // Відкрити вхідний файл
    TFile *fin = new TFile("./build/sorted_by_OM.root", "READ");
    
    TIter next(fin->GetListOfKeys());
    TKey *key;

    // Підключити гілку
    // int16_t om_num;
    int32_t run, event, ht, lt;
    std::vector<int16_t>* waveform = nullptr;

    while ((key = (TKey*)next())){
        TObject *obj = key->ReadObj();

        if (obj->InheritsFrom(TTree::Class())) {
            TTree *tree = (TTree*)obj;
            
            // tree->SetBranchAddress("run_id", &run);
            // tree->SetBranchAddress("event_id", &event);
            tree->SetBranchAddress("om_num", &om_num);
            tree->SetBranchAddress("high_threshold", &ht);
            tree->SetBranchAddress("low_threshold", &lt);
            tree->SetBranchAddress("waveform", &waveform);

            tree->GetEntry(0);

            TH2F *histo = new TH2F(
                Form("h2_om_%d", om_num),
                Form("OM %d (substracted BL);Time;Amplitude", om_num),
                1024, 0, 6.4E-6,
                4800, -3e-2, 9e-2
            );
            histo->SetStats(0);

            double ymin = 1e9;
            double ymax = -1e9;

            double baseline = baselines[om_num];
            int nEntries = tree->GetEntries();

            for (int i = 0; i < nEntries; i++) {
                tree->GetEntry(i);
                if (ht == 0) continue;
                int n = waveform->size();
                double normalization_sum = 0;
                for (int j = 0; j < n; j++) {
                    normalization_sum += waveform->at(j) - baseline;
                }
                for (int j = 0; j < n; j++) {
                    double timestamp = j * 6.25E-9;
                    double value = (waveform->at(j) - baseline) / normalization_sum;
                    if (value > ymax) ymax = value;
                    if (value < ymin) ymin = value;
                    histo->Fill(timestamp, value);
                }
            }
            // ymax += 20;
            // ymin -= 20;
            if (histo->GetEntries() == 0){
                std::cout << "OM " << om_num << " has no event data!" << std ::endl;
                continue;
            }

            histo->GetYaxis()->SetRangeUser(ymin, ymax);

            TCanvas *c1 = new TCanvas("c1", "2D Hist", 800, 600);
            histo->Draw("COLZ");
            c1->SaveAs(Form("./build/Histos_by_OMs_substracted_BL/OM%d_2D_histo_substracted_BL.png", om_num));
            delete c1;
        }

    }
    fin->Close();
    return 0;
}