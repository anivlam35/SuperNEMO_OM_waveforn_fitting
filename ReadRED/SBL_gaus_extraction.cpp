#include <TFile.h>
#include <TTree.h>

#include <iostream>
#include <vector>
#include <algorithm>
#include <cmath>

#include <omp.h>

#include "BLlibGPT.h"

#include <ROOT/TThreadExecutor.hxx>
#include <ROOT/RDataFrame.hxx>
#include <ROOT/RLogger.hxx>

#include <chrono>

int main() {
    auto start = std::chrono::high_resolution_clock::now();

    ROOT::EnableThreadSafety();

    TFile warmup("./build/sorted_by_OM.root", "READ");
    TTree *tmp = nullptr;
    warmup.GetObject("t_om_0", tmp);
    warmup.Close();

    std::vector<TTree*> outputTrees;

    #pragma omp parallel for schedule(dynamic)
    for (int om_num = 0; om_num < 712; om_num++) {

        // thread-local file
        TFile *fin =
            TFile::Open(
                "./build/sorted_by_OM.root",
                "READ"
            );

        if (!fin || fin->IsZombie()) {

            #pragma omp critical
            {
                std::cout
                    << "Cannot open input file\n";
            }

            continue;
        }

        TTree *tree = nullptr;

        fin->GetObject(
            Form("t_om_%d", om_num),
            tree
        );

        if (!tree) {

            #pragma omp critical
            {
                std::cout
                    << "Tree not found: "
                    << om_num
                    << "\n";
            }

            fin->Close();
            delete fin;

            continue;
        }

        int32_t run = 0;
        int32_t event = 0;
        int32_t ht = 0;
        int32_t lt = 0;

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



        TTree *SBL_tree =
            new TTree(Form("SBL_OM%d", om_num),
                    Form("OM%d sample baselines", om_num));
        SBL_tree->SetDirectory(nullptr);

        double bl[1024];

        SBL_tree->Branch(
            "baseline",
            bl,
            "baseline[1024]/D"
        );

        double local_bl[1024];

        bool skipOM = false;
        for (int sample = 0; sample < 1024; sample++) {

            local_bl[sample] =
                get_sample_baseline(
                    tree,
                    sample,
                    ht,
                    lt,
                    waveform
                );

            if (std::isnan(local_bl[sample])) {
                skipOM = true;
                break;
            }
        }

        if (!skipOM) {

            #pragma omp critical
            {
                std::copy(
                    local_bl,
                    local_bl + 1024,
                    bl
                );

                SBL_tree->Fill();
                outputTrees.push_back(SBL_tree);
            }
        }

        fin->Close();
        delete fin;

        std::cout << "OM" << om_num << " SBLs extracted.\n"; 
    }

    
    TFile fout("./build/SBLs.root", "RECREATE");

    for (auto tree : outputTrees) {

        fout.cd();

        tree->Write();

        delete tree;
    }

    fout.Close();

    auto end = std::chrono::high_resolution_clock::now();

    auto duration =
        std::chrono::duration_cast<std::chrono::milliseconds>(
            end - start
        );

    std::cout << "Execution time: "
            << duration.count()
            << " ms\n";

    return 0;
}