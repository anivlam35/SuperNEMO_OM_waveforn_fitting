#include <omp.h>

#include <TFile.h>
#include <TTree.h>
#include <TH2F.h>
#include <TCanvas.h>

#include <vector>
#include <iostream>
#include <cmath>

#include "BLlib.h"

int main() {

    ROOT::EnableThreadSafety();

    TH2F *histo = new TH2F(
        "h2",
        "Gauss vs Mode baselines (All OMs);BL mode;BL gauss",
        600, 3300, 3600,
        600, 3300, 3600
    );

    histo->SetDirectory(nullptr);
    histo->SetStats(0);

    #pragma omp parallel
    {
        // thread-local histogram
        TH2F local_histo(
            Form("h_thread_%d", omp_get_thread_num()),
            "",
            600, 3300, 3600,
            600, 3300, 3600
        );

        local_histo.SetDirectory(nullptr);

        #pragma omp for schedule(dynamic)
        for (int om_num = 0; om_num < 712; om_num++) {

            // thread-local file
            TFile *fin = TFile::Open(
                "./build/sorted_by_OM.root",
                "READ"
            );

            if (!fin || fin->IsZombie()) {

                #pragma omp critical
                {
                    std::cout << "Cannot open file\n";
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
                        << "Tree not found for OM "
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

            const Long64_t nSamples = 1024;

            for (int sample = 0; sample < nSamples; sample++) {

                double mode = get_sample_baseline_mode(
                    tree,
                    sample,
                    ht,
                    lt,
                    waveform
                );

                double gauss = get_sample_baseline(
                    tree,
                    sample,
                    ht,
                    lt,
                    waveform
                );

                if (
                    std::isnan(mode) ||
                    std::isnan(gauss)
                ) {
                    continue;
                }

                local_histo.Fill(mode, gauss);
            }

            fin->Close();
            delete fin;

            #pragma omp critical
            {
                std::cout
                    << "OM "
                    << om_num
                    << " done\n";
            }
        }

        // merge histograms
        #pragma omp critical
        {
            histo->Add(&local_histo);
        }
    }

    // save ROOT histogram
    TFile fout(
        "./build/All_OMs_gauss_vs_mode_BL.root",
        "RECREATE"
    );

    histo->Write();

    fout.Close();

    // draw canvases
    TCanvas *c1 = new TCanvas(
        "c1",
        "Graph",
        800,
        600
    );

    histo->Draw("COLZ");

    c1->SaveAs(
        "./build/All_OMs_gauss_vs_mode_BL.png"
    );

    // upper peak
    histo->GetXaxis()->SetRangeUser(3510, 3590);
    histo->GetYaxis()->SetRangeUser(3510, 3590);

    histo->Draw("COLZ");

    c1->SaveAs(
        "./build/All_OMs_gauss_vs_mode_BL_UP.png"
    );

    // lower peak
    histo->GetXaxis()->SetRangeUser(3350, 3390);
    histo->GetYaxis()->SetRangeUser(3350, 3390);

    histo->Draw("COLZ");

    c1->SaveAs(
        "./build/All_OMs_gauss_vs_mode_BL_LP.png"
    );

    delete c1;
    delete histo;

    return 0;
}