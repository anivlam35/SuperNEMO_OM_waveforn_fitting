#include <TFile.h>
#include <TH1D.h>
#include <TCanvas.h>
#include <TLegend.h>
#include <TStyle.h>
#include <TKey.h>
#include <TList.h>
#include <TString.h>
#include <TLatex.h>

#include <iostream>
#include <vector>
#include <string>
#include <algorithm>

// ------------------------------------------------------------
// USER SETTINGS
// ------------------------------------------------------------
const std::vector<std::string> INPUT_FILES = {
    "tot_energy_spectrum-BOTH_0-30.root",
    "tot_energy_spectrum-BOTH_30-60.root",
    "tot_energy_spectrum-BOTH_60-90.root",
    "tot_energy_spectrum-ADV-SIMU.root"
};

const std::vector<std::string> LABELS = {
    "REAL 15.0, 40.0, new src positions, 0-30 deg",
    "REAL 15.0, 40.0, new src positions, 30-60 deg",
    "REAL 15.0, 40.0, new src positions, 60-90 deg",
    "SIMU 15.0, 40.0, new src positions, SOLID ANGLE"
};

const std::vector<int> COLORS = {
    kGreen + 2,
    kBlue + 1,
    kViolet + 1,
    kRed + 1

};

const char* OUTPUT_ROOT = "tot_spectr_angular_overlay+SIMU.root";
const char* CANVAS_NAME = "c_overlay";
const char* CANVAS_TITLE = "Total spectrum angular overlay";

const double XMIN_KEV = 0.0;
const double XMAX_KEV = 2000.0;

// histogram title/axes
const char* HIST_TITLE = "Total spectrum angular overlay;Energy [keV];Normalized counts";

// ------------------------------------------------------------
// Find first TH1D in file
// ------------------------------------------------------------
TH1D* get_first_histogram(TFile* f)
{
    if (!f) return nullptr;

    TList* keys = f->GetListOfKeys();
    if (!keys) return nullptr;

    TIter next(keys);
    TKey* key = nullptr;

    while ((key = (TKey*)next())) {
        TObject* obj = key->ReadObj();
        if (!obj) continue;

        TH1D* h = dynamic_cast<TH1D*>(obj);
        if (h) {
            h->SetDirectory(nullptr); // detach from file
            return h;
        }

        delete obj;
    }

    return nullptr;
}

// ------------------------------------------------------------
// MAIN
// ------------------------------------------------------------
void draw_spectra_overlay()
{
    gStyle->SetOptStat(0);

    const int nFiles = (int)INPUT_FILES.size();
    if (nFiles == 0) {
        std::cerr << "No input files provided.\n";
        return;
    }

    std::vector<TFile*> inFiles;
    std::vector<TH1D*>  hists;
    std::vector<std::string> usedLabels;
    std::vector<int> usedColors;
    std::vector<long long> usedEntries;

    double maxY = 0.0;

    // --------------------------------------------------------
    // Open files and load one histogram from each
    // --------------------------------------------------------
    for (int i = 0; i < nFiles; ++i) {
        TFile* f = TFile::Open(INPUT_FILES[i].c_str(), "READ");
        if (!f || f->IsZombie()) {
            std::cerr << "ERROR: cannot open " << INPUT_FILES[i] << "\n";
            inFiles.push_back(nullptr);
            continue;
        }

        std::cout << "Opened: " << INPUT_FILES[i] << "\n";
        inFiles.push_back(f);

        TH1D* h = get_first_histogram(f);
        if (!h) {
            std::cerr << "WARNING: no TH1D found in " << INPUT_FILES[i] << "\n";
            continue;
        }

        const int color = COLORS[i % (int)COLORS.size()];

        h->SetLineColor(color);
        h->SetMarkerColor(color);
        h->SetLineWidth(1);
        h->SetFillStyle(0);
        h->SetOption("E");

        double thisMax = h->GetMaximum();
        if (thisMax > maxY) maxY = thisMax;

        hists.push_back(h);
        usedLabels.push_back(i < (int)LABELS.size() ? LABELS[i] : INPUT_FILES[i]);
        usedColors.push_back(color);
        usedEntries.push_back((long long)h->GetEntries());
    }

    if (hists.empty()) {
        std::cerr << "No histograms loaded.\n";
        for (auto* f : inFiles) {
            if (f) { f->Close(); delete f; }
        }
        return;
    }

    // --------------------------------------------------------
    // Output file
    // --------------------------------------------------------
    TFile* fout = TFile::Open(OUTPUT_ROOT, "RECREATE");
    if (!fout || fout->IsZombie()) {
        std::cerr << "ERROR: cannot create output file " << OUTPUT_ROOT << "\n";
        for (auto* h : hists) delete h;
        for (auto* f : inFiles) {
            if (f) { f->Close(); delete f; }
        }
        return;
    }

    // --------------------------------------------------------
    // Canvas
    // --------------------------------------------------------
    TCanvas* c = new TCanvas(CANVAS_NAME, CANVAS_TITLE, 1000, 700);
    c->cd();

    hists[0]->SetTitle(HIST_TITLE);
    hists[0]->SetMaximum(maxY * 1.20);
    hists[0]->SetMinimum(0.0);
    hists[0]->GetXaxis()->SetRangeUser(XMIN_KEV, XMAX_KEV);
    hists[0]->Draw();

    for (int j = 1; j < (int)hists.size(); ++j) {
        hists[j]->GetXaxis()->SetRangeUser(XMIN_KEV, XMAX_KEV);
        hists[j]->Draw("SAME");
    }

    // --------------------------------------------------------
    // Legend
    // --------------------------------------------------------
    /*
    TLegend* leg = new TLegend(0.62, 0.68, 0.88, 0.88);
    leg->SetBorderSize(0);
    leg->SetFillStyle(0);

    for (int j = 0; j < (int)hists.size(); ++j) {
        leg->AddEntry(hists[j], usedLabels[j].c_str(), "l");
    }
    
    leg->Draw();
*/
    // --------------------------------------------------------
    // Colored entry counts
    // --------------------------------------------------------
    TLatex latex;
    latex.SetNDC();
    latex.SetTextSize(0.030);

    double xText = 0.16;
    double yText0 = 0.86;
    double dy = 0.045;

    for (int j = 0; j < (int)hists.size(); ++j) {
        latex.SetTextColor(usedColors[j]);
        latex.DrawLatex(xText, yText0 - j * dy,
                        Form("%s: Entries = %lld",
                             usedLabels[j].c_str(),
                             usedEntries[j]));
    }

    // --------------------------------------------------------
    // Write output
    // --------------------------------------------------------
    fout->cd();
    c->Write();
    fout->Close();

    // optional image export
    //c->SaveAs("_overlay.png");

    // cleanup
    //delete leg;
    delete c;
    delete fout;

    for (auto* h : hists) delete h;

    for (auto* f : inFiles) {
        if (f) {
            f->Close();
            delete f;
        }
    }

    std::cout << "Done. Written overlay canvas to " << OUTPUT_ROOT << "\n";
}
