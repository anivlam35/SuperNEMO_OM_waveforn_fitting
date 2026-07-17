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
#include <algorithm>
#include <omp.h>

int main() {

    TFile *fout = new TFile("./build/SBLs.root", "RECREATE");

    fout->cd();

    TTree *SBL_tree = new TTree("SBL_tree", "OM sample baselines");

    int om_num;
    double bl[1024];

    SBL_tree->Branch("om", &om_num);
    SBL_tree->Branch("baseline", bl, "baseline[1024]/D");

    #pragma omp parallel for schedule(dynamic)
    for (int om = 0; om < 19; om++){
        TFile *fin_local = new TFile("./build/sorted_by_OM.root", "READ");

        TTree *tree = nullptr;
        #pragma omp critical
        {
            fin_local->GetObject(Form("t_om_%d", om), tree);
        }

        if (!tree) {
            #pragma omp critical
            std::cout << "Tree not found\n";
            continue;
        }
    
        int32_t run, event, ht, lt;
        std::vector<int16_t>* waveform = nullptr;

        tree->SetBranchStatus("*", 0);
        tree->SetBranchStatus("run_id", 1);
        tree->SetBranchStatus("event_id", 1);
        tree->SetBranchStatus("high_threshold", 1);
        tree->SetBranchStatus("low_threshold", 1);
        tree->SetBranchStatus("waveform", 1);

        tree->SetBranchAddress("run_id", &run);
        tree->SetBranchAddress("event_id", &event);
        tree->SetBranchAddress("high_threshold", &ht);
        tree->SetBranchAddress("low_threshold", &lt);
        tree->SetBranchAddress("waveform", &waveform);
    
        double local_bl[1024] = {0};

        bool skipOM = false;
        for(int sample = 0; sample < 1024; sample++){
            local_bl[sample] = get_sample_baseline(
                tree,
                sample,
                ht,
                lt,
                waveform
            );

            if (std::isnan(local_bl[sample])) {
                skipOM = true;
                break;  // выходим из sample loop
            }
        }
        if (skipOM) continue;
        #pragma omp critical 
        {
            om_num = om;
            std::copy(local_bl, local_bl + 1024, bl);
            SBL_tree->Fill();
        }
        
        fin_local->Close();
        delete fin_local;
    }

    SBL_tree->Write();
    fout->Close();
}

