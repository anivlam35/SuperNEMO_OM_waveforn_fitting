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
#include <TLine.h>
#include <TGraph.h>
#include <filesystem>
#include <TLegend.h>
#include "BLlib.h"

int main() {
    TFile *fin = new TFile("./build/sorted_by_OM.root", "READ");

    // int OM = 412;
    TH2F *histo = new TH2F(
        "h2",
        // Form("OM%d baseline (substracted FBL);Time;Amplitude", om_num),
        "Gauss vs Avg defined baselines (All OMs);BL avg;BL gauss",
        800, 3300, 3600,
        800, 3300, 3600
    );
    histo->SetStats(0);
    for (int om_num = 0; om_num < 712; om_num++){
        // skipOM = false;


        // std::string dir = "build/1D_baselines_by_samples/OM_" + std::to_string(om);
        // if (fs::exists(dir)) fs::remove_all(dir);
        // fs::create_directories(dir);

        TTree *tree = nullptr;
        fin->GetObject(Form("t_om_%d", om_num), tree);

        if (!tree) {
            std::cout << "Tree not found\n";
            continue;;
        }
    
        int32_t run, event, ht, lt;
        std::vector<int16_t>* waveform = nullptr;

        // tree->SetBranchAddress("om_num", &om_num);
        tree->SetBranchAddress("run_id", &run);
        tree->SetBranchAddress("event_id", &event);
        tree->SetBranchAddress("high_threshold", &ht);
        tree->SetBranchAddress("low_threshold", &lt);
        tree->SetBranchAddress("waveform", &waveform);
    
        // double sample_baselines[1024];
        // double sample_baselines_avg[1024];        

        bool skipOM = false;
        for(int sample = 0; sample < 1024; sample++){
            double sample_baseline = get_sample_baseline(
                tree,
                sample,
                ht,
                lt,
                waveform
            );
            double sample_baseline_avg = get_sample_baseline_average(
                tree,
                sample,
                ht,
                lt,
                waveform
            );

            if (std::isnan(sample_baseline)) {
                skipOM = true;
                break;  // выходим из sample loop
            }

            histo->Fill(sample_baseline_avg, sample_baseline);
        }
        
        std::cout << "OM " << om_num << " done." << histo->GetEntries() << " entries.\n"; 


        //     // Вихід
        // TH2F *histo = new TH2F(
        //     Form("h2_om_%d", om_num),
        //     // Form("OM%d baseline (substracted FBL);Time;Amplitude", om_num),
        //     Form("OM%d 2D Histo (substracted FBL);Time;Amplitude", om_num),
        //     1024, 0, 6.4E-6,
        //     800, -7e2, 1e2
        // );
        // histo->SetStats(0);

        // double ymin = 1e9;
        // double ymax = -1e9;

        // tree->GetEntry(0);

        // int nEntries = tree->GetEntries();

        // for (int i = 0; i < nEntries; i++) {
        //     tree->GetEntry(i);
        //     // if ((ht == 1) || (lt == 1)) continue;
        //     int n = waveform->size();
        //     for (int j = 0; j < n; j++) {
        //         double timestamp = j * 6.25E-9;
        //         double value = waveform->at(j) - sample_baselines[j];
        //         if (value > ymax) ymax = value;
        //         if (value < ymin) ymin = value;
        //         histo->Fill(timestamp, value);
        //     }
        // }
        // ymax += 20;
        // ymin -= 20;
        // histo->GetYaxis()->SetRangeUser(ymin, ymax);
        // TCanvas *c1 = new TCanvas(Form("c1_%d", om_num), "2D Hist", 800, 600);
        // histo->Draw("COLZ");
        // c1->SaveAs(Form("./build/OM%d_1D_Histo_subSBL.png", om_num));
        // // c1->SaveAs(Form("./build/2D_baselines_histos_subFBL/OM%d_2D_baseline_subFBL.png", om_num));
        // delete histo;
        // delete c1;




//     break;
    }
    TCanvas *c1 = new TCanvas("c1", "Graph", 800, 600);

    histo->Draw("COLZ");
    c1->SaveAs("./build/All_OMs_gaus_vs_avg_BL.png");
}

