#include <TFile.h>
#include <TTree.h>
#include <TGraph.h>
#include <TH2.h>
#include <TCanvas.h>
#include <vector>
#include <iostream>
#include <fstream>
#include <TKey.h>
#include <omp.h>

#include <ROOT/TThreadExecutor.hxx>
#include <ROOT/RDataFrame.hxx>
#include <ROOT/RLogger.hxx>

int main() {

    ROOT::EnableThreadSafety();

    TFile *fin = new TFile("./build/sorted_by_OM.root", "READ");



    // ===================== CHANGED =====================
    // Собираем имена деревьев заранее
    std::vector<std::string> treeNames;

    TIter next(fin->GetListOfKeys());
    TKey *key;

    while ((key = (TKey*)next())) {

        TObject *obj = key->ReadObj();

        if (obj->InheritsFrom(TTree::Class())) 
        {
            treeNames.push_back(obj->GetName());
        }

        delete obj;
    }
    // ===================================================



    TH2F *histo = new TH2F(
        "h2_all_om",
        "All OMs (substracted BL);Time;Amplitude",
        1024, 0, 6.4E-6,
        400, -1e-2, 3e-2
    );

    histo->SetStats(0);

    double ymin = 1e9;
    double ymax = -1e9;



    // ===================== CHANGED =====================
    #pragma omp parallel
    {

        // thread-local histogram
        TH2F *local_histo = new TH2F(
            Form("h_thread_%d", omp_get_thread_num()),
            "",
            1024, 0, 6.4E-6,
            400, -1e-2, 3e-2
        );

        double local_ymin = 1e9;
        double local_ymax = -1e9;

        // thread-local files
        TFile fin_local("./build/sorted_by_OM.root", "READ");

        TFile sbl_local("./build/SBLs.root", "READ");



        #pragma omp for schedule(dynamic)
        for (int t = 0; t < treeNames.size(); t++) {
    // ===================================================



            TTree *tree = nullptr;

            // ===================== CHANGED =====================
            fin_local.GetObject(treeNames[t].c_str(), tree);
            // ===================================================

            if (!tree) continue;



            int16_t om_num;
            int32_t run, event, ht, lt;

            std::vector<int16_t>* waveform = nullptr;

            tree->SetBranchAddress("om_num", &om_num);
            tree->SetBranchAddress("high_threshold", &ht);
            tree->SetBranchAddress("low_threshold", &lt);
            tree->SetBranchAddress("waveform", &waveform);

            tree->GetEntry(0);

            TTree *SBLs_tree = nullptr;



            // ===================== CHANGED =====================
            sbl_local.GetObject(Form("SBL_OM%d", om_num), SBLs_tree);
            // ===================================================



            if (!SBLs_tree) {

                #pragma omp critical
                {
                    std::cout << "OM" << om_num << " is turned off.\n";
                }

                continue;
            }

            double bl[1024];

            SBLs_tree->SetBranchAddress("baseline", bl);

            SBLs_tree->GetEntry(0);

            Long64_t nEntries = tree->GetEntries();

            for (Long64_t i = 0;i < nEntries; i++) {
                tree->GetEntry(i);
                if (!waveform) continue;
                double normalization_sum = 0;
                for (int sample = 0; sample < 1024; sample++) {
                    normalization_sum += waveform->at(sample) - bl[sample];
                }

                // ===================== CHANGED =====================
                if (normalization_sum < 500)
                    continue;
                // ===================================================

                for (int sample = 0; sample < 1024; sample++) {
                    double timestamp = sample * 6.25E-9;
                    // double value = (waveform->at(sample) - bl[sample]) / normalization_sum;
                    double value = (waveform->at(sample) - bl[sample]);

                    if (value > local_ymax) local_ymax = value;
                    if (value < local_ymin) local_ymin = value;

                    // ===================== CHANGED =====================
                    local_histo->Fill(timestamp, value);
                    // ===================================================
                }
            }

            #pragma omp critical
            {
                std::cout << "OM" << om_num << " done.\n";
            }
        
        }

        // ===================== CHANGED =====================
        // merge histograms
        #pragma omp critical
        {
            histo->Add(local_histo);

            if (local_ymax > ymax) ymax = local_ymax;
            if (local_ymin < ymin) ymin = local_ymin;
        }

        delete local_histo;
    }
    // ===================================================



    TCanvas *c1 =new TCanvas("c1"," 2D Hist", 800, 600);

    histo->Draw("COLZ");

    c1->SaveAs("./build/2D_histo_substracted_SBL_normalized.png");

    fin->Close();

    // delete c1;
    // delete histo;
    // delete fin;

    return 0;
}