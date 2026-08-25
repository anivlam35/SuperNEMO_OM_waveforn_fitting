#include <TCanvas.h>
#include <TH1D.h>
#include <TFile.h>
#include <TPaveText.h>
#include <TStyle.h>

#include <vector>
#include <cstdio>
#include <utility>
#include <cmath>

void analyze_spectrum_from_txt_ADV(
    const char* infile = "energy_spectrum_electrons_ptd_merged_week_2025_X.txt",
    double binWidth_keV = 5.0)
{
    gStyle->SetOptStat(0);

    TCanvas* canvas_spectrum =
        new TCanvas("canvas_spectrum", "Total Energy Spectrum BOTH", 800, 600);

    FILE* f = fopen(infile, "r");
    if (!f) {
        printf("Cannot open %s\n", infile);
        return;
    }

    std::vector<std::pair<long long,double>> rows;
    rows.reserve(100000);

    long long N_total = 0;

    char line[256];
    while (fgets(line, sizeof(line), f)) {
        if (line[0] == '#' || line[0] == '\n') continue;

        long long cnt = 0;
        double E = 0.0;

        if (sscanf(line, "%lld %lf", &cnt, &E) == 2 && cnt > 0) {
            rows.emplace_back(cnt, E);
            N_total += cnt;
        }
    }
    fclose(f);

    if (rows.empty()) {
        printf("No data\n");
        return;
    }

    double minE = rows.front().second;
    double maxE = rows.front().second;

    for (const auto& r : rows) {
        if (r.second < minE) minE = r.second;
        if (r.second > maxE) maxE = r.second;
    }

    const double lo    = minE - 0.5 * binWidth_keV;
    const double hi    = maxE + 0.5 * binWidth_keV;
    const int    nbins = int((hi - lo) / binWidth_keV + 0.5);

    TH1D* h = new TH1D("h_energy",
                       "Total Energy spectrum BOTH;Energy [keV];Normalized counts",
                       nbins, lo, lo + nbins * binWidth_keV);

    h->Sumw2(kTRUE);

    for (const auto& r : rows) {
        const long long cnt = r.first;
        const double E      = r.second;

        const int ibin = h->FindBin(E);
        if (ibin < 1 || ibin > h->GetNbinsX()) continue;

        h->SetBinContent(ibin, (double)cnt);
        h->SetBinError  (ibin, std::sqrt((double)cnt));
    }

    h->SetEntries((Double_t)N_total);

    if (h->Integral() > 0)
        h->Scale(1.0 / h->Integral());

    h->SetLineWidth(1);
    h->SetFillStyle(0);
    h->SetOption("E");

   h->GetXaxis()->SetRangeUser(0.0, 2500.0);  // X

    TPaveText* pt = new TPaveText(0.58, 0.74, 0.88, 0.88, "NDC");
    pt->SetBorderSize(1);
    pt->SetFillColor(0);
    pt->SetTextAlign(12);
    pt->AddText(Form("Entries = %lld", N_total));
    h->GetListOfFunctions()->Add(pt);

    h->Draw();

    canvas_spectrum->SaveAs("tot_energy_spectrum-ADV-BOTH.png");

    TFile fout("tot_energy_spectrum-ADV-BOTH.root", "RECREATE");
    h->Write();
    fout.Close();

    printf("Histogram: %d bins from %.3f to %.3f keV (bin=%.3f keV)\n",
           nbins, lo, lo + nbins * binWidth_keV, binWidth_keV);
    printf("Total entries: %lld\n", N_total);
}
