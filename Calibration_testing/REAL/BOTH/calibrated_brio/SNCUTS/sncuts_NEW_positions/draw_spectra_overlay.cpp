#include <TFile.h>
#include <TH1D.h>
#include <TCanvas.h>
#include <TLegend.h>
#include <TStyle.h>
#include <TKey.h>
#include <TList.h>
#include <TString.h>

#include <iostream>
#include <vector>
#include <string>

// ------------------------------------------------------------
// USER SETTINGS
// ------------------------------------------------------------
const std::vector<std::string> INPUT_FILES = {
    "tot_energy_spectrum-BOTH_0-30.root",
    "tot_energy_spectrum-BOTH_30-60.root",
    "tot_energy_spectrum-BOTH_60-90.root"
};

const std::vector<std::string> LABELS = {
"[0^{#circ}, 30^{#circ})",
"[30^{#circ}, 60^{#circ})",
"[60^{#circ}, 90^{#circ}]"
};

const std::vector<int> COLORS = {
    TColor::GetColor("#0072B2"),   
    TColor::GetColor("#E69F00"),   
    TColor::GetColor("#CC79A7")    
};

const char* OUTPUT_ROOT = "tot_4spectr_overlay.root";
const char* CANVAS_NAME = "c_overlay";
const char* CANVAS_TITLE = "Total spectrum angular overlay";

const double XMIN_KEV = 0.0;
const double XMAX_KEV = 1400.0;

const char* HIST_TITLE = ";Energy [keV];Normalized counts";

// ---- DRAW STYLE: uncomment ONE option ----
// Option 1: step histogram (currently active)
const char* DRAW_STYLE = "HIST";
// Option 2: error bars / crosses ? comment line above and uncomment below
// const char* DRAW_STYLE = "E";
// ------------------------------------------

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
        if (h) { h->SetDirectory(nullptr); return h; }
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
    if (nFiles == 0) { std::cerr << "No input files provided.\n"; return; }

    std::vector<TFile*> inFiles;
    std::vector<TH1D*>  hists;
    std::vector<std::string> usedLabels;
    std::vector<int> usedColors;

    double maxY = 0.0;

    // --------------------------------------------------------
    // Open files and load histograms
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
        h->SetLineWidth(2);
        h->SetFillStyle(0);
        h->SetOption(DRAW_STYLE);

        double thisMax = h->GetMaximum();
        if (thisMax > maxY) maxY = thisMax;

        hists.push_back(h);
        usedLabels.push_back(i < (int)LABELS.size() ? LABELS[i] : INPUT_FILES[i]);
        usedColors.push_back(color);
    }

    if (hists.empty()) {
        std::cerr << "No histograms loaded.\n";
        for (auto* f : inFiles) { if (f) { f->Close(); delete f; } }
        return;
    }

    // --------------------------------------------------------
    // Output file
    // --------------------------------------------------------
    TFile* fout = TFile::Open(OUTPUT_ROOT, "RECREATE");
    if (!fout || fout->IsZombie()) {
        std::cerr << "ERROR: cannot create output file " << OUTPUT_ROOT << "\n";
        for (auto* h : hists) delete h;
        for (auto* f : inFiles) { if (f) { f->Close(); delete f; } }
        return;
    }

    // --------------------------------------------------------
    // Canvas
    // --------------------------------------------------------
    TCanvas* c = new TCanvas(CANVAS_NAME, CANVAS_TITLE, 1100, 550);
    c->SetLeftMargin(0.10);
    c->SetRightMargin(0.04);
    c->SetBottomMargin(0.13);
    c->SetTopMargin(0.08);
    c->cd();

    hists[0]->SetTitle(HIST_TITLE);
    hists[0]->SetMaximum(maxY * 1.20);
    hists[0]->SetMinimum(0.0);
    hists[0]->GetXaxis()->SetRangeUser(XMIN_KEV, XMAX_KEV);
    hists[0]->GetXaxis()->SetTitleSize(0.05);
    hists[0]->GetXaxis()->SetLabelSize(0.04);
    hists[0]->GetYaxis()->SetTitleSize(0.05);
    hists[0]->GetYaxis()->SetLabelSize(0.04);
    hists[0]->GetYaxis()->SetTitleOffset(0.85);
    hists[0]->Draw(DRAW_STYLE);

    for (int j = 1; j < (int)hists.size(); ++j) {
        hists[j]->GetXaxis()->SetRangeUser(XMIN_KEV, XMAX_KEV);
        hists[j]->Draw(Form("%s SAME", DRAW_STYLE));
    }

    // --------------------------------------------------------
    // Legend
    // --------------------------------------------------------
    TLegend* leg = new TLegend(0.15, 0.75, 0.40, 0.88);
    leg->SetBorderSize(0);
    leg->SetFillColor(0);
    leg->SetFillStyle(1001);
    leg->SetTextSize(0.040);

    for (int j = 0; j < (int)hists.size(); ++j)
        leg->AddEntry(hists[j], usedLabels[j].c_str(), "l");

    leg->Draw();


    // --------------------------------------------------------
    // Write output
    // --------------------------------------------------------
    fout->cd();
    c->Write();
    fout->Close();

    c->SaveAs("angular_overlay.png");

    delete c;
    delete fout;
    for (auto* h : hists) delete h;
    for (auto* f : inFiles) { if (f) { f->Close(); delete f; } }

    std::cout << "Done. Written overlay canvas to " << OUTPUT_ROOT << "\n";
}
