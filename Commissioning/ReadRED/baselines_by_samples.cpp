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
    namespace fs = std::filesystem;
    // Reading baselines
    // std::ifstream in("OM_baselines_sorted.txt");

    // std::map<int16_t, double> baselines;

    // std::string header;
    // std::getline(in, header);

    // int16_t om_num;
    // double mean, sigma;

    // while (in >> om_num >> mean >> sigma) {
    //     baselines[om_num] = mean;
    // }

    // in.close();

    TFile *fin = new TFile("./build/sorted_by_OM.root", "READ");

    
    for (int om_num = 18; om_num < 19; om_num++){
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
    
        double sample_baselines[1024];

        bool skipOM = false;
        for(int sample = 0; sample < 1024; sample++){
            sample_baselines[sample] = get_sample_baseline(
                tree,
                sample,
                ht,
                lt,
                waveform
            );

            if (std::isnan(sample_baselines[sample])) {
                skipOM = true;
                break;  // выходим из sample loop
            }
        }


        TH1D *histo = new TH1D(
            "h1",
            "1D baseline histo;mV;N",
            2e2, -1e2, 1e2
        );

        double xmin = 1e9;
        double xmax = -1e9;

        int nEntries = tree->GetEntries();
        
        for (int i = 0; i < nEntries; i++) {
            tree->GetEntry(i);
            if ((ht == 1) || (lt == 1)) continue;
            for (int sample = 0; sample < 1024; sample++){                
                double value = waveform->at(sample) - sample_baselines[sample];
                if (value > xmax) xmax = value;
                if (value < xmin) xmin = value;
                histo->Fill(value);
            }
        }

        xmax += 20;
        xmin -= 20;
        histo->GetXaxis()->SetRangeUser(xmin, xmax);
    
        

    
        double general_baseline = get_general_baseline(
                tree,
                ht,
                lt,
                waveform
            );

        TH1D *histo2 = new TH1D(
            "h1",
            "Baseline general correction;mV;N",
            2e2, -1e2, 1e2
        );

        xmin = 1e9;
        xmax = -1e9;
        
        for (int i = 0; i < nEntries; i++) {
            tree->GetEntry(i);
            if ((ht == 1) || (lt == 1)) continue;
            for (int sample = 0; sample < 1024; sample++){                
                double value = waveform->at(sample) - general_baseline;
                if (value > xmax) xmax = value;
                if (value < xmin) xmin = value;
                histo2->Fill(value);
            }
        }

        xmax += 20;
        xmin -= 20;
        histo2->GetXaxis()->SetRangeUser(xmin, xmax);

        TCanvas *c1 = new TCanvas(Form("c1_%d", om_num), "2D Hist", 800, 600);

        histo->SetLineColor(kBlue);
        histo->SetLineWidth(2);

        histo2->SetLineColor(kRed);
        histo2->SetLineWidth(2);

        histo->Draw("HIST");
        histo2->Draw("HIST SAME");

        TLegend *leg = new TLegend(0.65, 0.6, 0.88, 0.7); // (x1,y1,x2,y2)
        leg->AddEntry(histo,  "Baseline sample correction",  "l");
        leg->AddEntry(histo2, "Baseline general correction",  "l");
        leg->SetBorderSize(0);   // убрать рамку (по желанию)
        leg->Draw();

        c1->SaveAs(Form("./build/OM%d_1D_BLs_Histo_subBL_subSBL.png", om_num));
    


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
}

