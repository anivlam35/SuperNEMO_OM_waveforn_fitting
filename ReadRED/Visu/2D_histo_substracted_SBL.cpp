#include <TFile.h>
#include <TTree.h>
#include <TH2.h>
#include <TCanvas.h>
#include <TKey.h>

#include <omp.h>

#include <vector>
#include <string>
#include <iostream>
#include <cmath>

#include <ROOT/TThreadExecutor.hxx>
#include <ROOT/RDataFrame.hxx>
#include <ROOT/RLogger.hxx>

constexpr int NSAMPLES = 1024;
constexpr double DT = 6.25E-9;



// ======================================================
// GET TREE NAMES
// ======================================================

std::vector<std::string> GetTreeNames(TFile* file)
{
    std::vector<std::string> treeNames;

    TIter next(file->GetListOfKeys());
    TKey* key;

    while ((key = (TKey*)next()))
    {
        TObject* obj = key->ReadObj();

        if (obj->InheritsFrom(TTree::Class()))
        {
            treeNames.push_back(obj->GetName());
        }

        delete obj;
    }

    return treeNames;
}



// ======================================================
// CREATE HISTOGRAM
// ======================================================

TH2F* CreateHistogram(int om_num)
{
    TH2F* histo = new TH2F(
        Form("h2_om_%d", om_num),
        Form("OM %d (substracted SBL);Time;Amplitude", om_num),
        NSAMPLES,
        0,
        6.4E-6,
        5e2,
        -1e-2,
        4e-2
    );

    histo->SetStats(0);

    return histo;
}



// ======================================================
// SAVE HISTOGRAM
// ======================================================

void SaveHistogram(TH2F* histo, int om_num)
{
    TCanvas* c1 = new TCanvas(
        Form("c1_om_%d", om_num),
        "",
        1600,
        1200
    );

    histo->Draw("COLZ");

    c1->SaveAs(
        Form(
            "./build/2D_histos_sSBL_norm/OM%d_2D_histo_sSBL_norm_ht.png",
            om_num
        )
    );

    delete c1;
}

void FillHisto(TH2F* histo, double* bl, std::vector<int16_t>* waveform, int32_t ht){
        for (int sample = 0;
             sample < NSAMPLES;
             sample++)
        {
            double timestamp =
                sample * DT;

            double value =
                (-1) * (waveform->at(sample)
                - bl[sample]);

            histo->Fill(
                timestamp,
                value
            );
        }
}

void FillHistoNormalized(TH2F* histo, double* bl, std::vector<int16_t>* waveform, double normalization_sum){
        for (int sample = 0;
             sample < NSAMPLES;
             sample++)
        {
            double timestamp =
                sample * DT;

            double value =
                (-1) * (waveform->at(sample)
                - bl[sample]) / normalization_sum;

            histo->Fill(
                timestamp,
                value
            );
        }
}

double GetNormalizationSum(double* bl, std::vector<int16_t>* waveform){
    double normalization_sum = 0;
    for (int sample = 0;
             sample < NSAMPLES;
             sample++)
    {
        normalization_sum +=
            std::abs(waveform->at(sample)
            - bl[sample]);
    }

    return normalization_sum;
}



// ======================================================
// PROCESS SINGLE TREE
// ======================================================

TH2F* ProcessTree(
    TTree* tree,
    TFile& sbl_file
)
{
    if (!tree)
        return nullptr;

    int16_t om_num;
    int32_t ht, lt;

    std::vector<int16_t>* waveform = nullptr;

    tree->SetBranchAddress("om_num", &om_num);
    tree->SetBranchAddress("high_threshold", &ht);
    tree->SetBranchAddress("low_threshold", &lt);
    tree->SetBranchAddress("waveform", &waveform);

    tree->GetEntry(0);

    // --------------------------------------------------

    TTree* sbl_tree = nullptr;

    sbl_file.GetObject(
        Form("SBL_OM%d", om_num),
        sbl_tree
    );

    if (!sbl_tree)
    {
        #pragma omp critical
        {
            std::cout
                << "OM"
                << om_num
                << " is turned off or has low \"no trigger\" statictics.\n";
        }

        return nullptr;
    }

    // --------------------------------------------------

    double bl[NSAMPLES];

    sbl_tree->SetBranchAddress(
        "baseline",
        bl
    );

    sbl_tree->GetEntry(0);

    // --------------------------------------------------

    TH2F* histo = CreateHistogram(om_num);

    // --------------------------------------------------

    Long64_t nEntries = tree->GetEntries();

    for (Long64_t i = 0; i < nEntries; i++)
    {
        tree->GetEntry(i);

        if (!waveform || !ht)
            continue;

        double normalization_sum = GetNormalizationSum(bl, waveform);

        

        if (std::abs(normalization_sum) < 5000){
            std::cout << "OM" << om_num << " too low sum.\n";
            continue;
        }

        FillHistoNormalized(histo, bl, waveform, normalization_sum);
    }

    #pragma omp critical
    {
        std::cout
            << "OM"
            << om_num
            << " done.\n";
    }

    return histo;
}



// ======================================================
// MAIN
// ======================================================

int main()
{
    ROOT::EnableThreadSafety();

    // --------------------------------------------------

    TFile* fin = new TFile(
        "./build/sorted_by_OM.root",
        "READ"
    );

    if (!fin || fin->IsZombie())
    {
        std::cout
            << "Cannot open input ROOT file.\n";

        return 1;
    }

    // --------------------------------------------------

    std::vector<std::string> treeNames =
        GetTreeNames(fin);


    TH2F* global_histo = new TH2F(
        "h2_all_om",
        "All OMs (substracted SBL, normalized, ht);Time;Amplitude",
        NSAMPLES,
        0,
        6.4E-6,
        500,
        -1e-2,
        4e-2
    );

    global_histo->SetStats(0);

    // --------------------------------------------------

    #pragma omp parallel
    {
        // thread-local files

        TH2F* local_global_histo = new TH2F(
            Form("h2_global_thread_%d", omp_get_thread_num()),
            "",
            NSAMPLES,
            0,
            6.4E-6,
            500,
            -1e-2,
            4e-2
        );  

        TFile fin_local(
            "./build/sorted_by_OM.root",
            "READ"
        );

        TFile sbl_local(
            "./build/SBLs.root",
            "READ"
        );

        // --------------------------------------------------

        #pragma omp for schedule(dynamic)

        for (size_t t = 0;
             t < treeNames.size();
             t++)
        {
            TTree* tree = nullptr;

            fin_local.GetObject(
                treeNames[t].c_str(),
                tree
            );

            if (!tree)
                continue;

            // --------------------------------------------------

            TH2F* histo =
                ProcessTree(
                    tree,
                    sbl_local
                );

            if (!histo)
                continue;

            // --------------------------------------------------
            // ROOT GRAPHICS ARE NOT THREAD SAFE
            // --------------------------------------------------

            int om_num;

            sscanf(
                histo->GetName(),
                "h2_om_%d",
                &om_num
            );

            if (histo->GetEntries() == 0)
            {
                #pragma omp critical
                {
                    std::cout
                        << "OM"
                        << om_num
                        << " histogram is empty.\n";
                }

                delete histo;
                continue;
            }

            #pragma omp critical
            {
                // SaveHistogram(
                //     histo,
                //     om_num
                // );

                local_global_histo->Add(histo);
            }

            delete histo;
        }

        #pragma omp critical
        {
            global_histo->Add(local_global_histo);
        }

        delete local_global_histo;
    }

    // --------------------------------------------------

    TCanvas* c_global = new TCanvas(
        "c_global",
        "",
        1600,
        1200
    );

    global_histo->Draw("COLZ");

    c_global->SaveAs(
        "./build/ALL_OMs_2D_histo_sSBL_ht_absNS5000.png"
    );

    delete c_global;
    delete global_histo;

    fin->Close();

    delete fin;

    return 0;
}