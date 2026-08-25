void analyze_spectrum_from_txt(
    const char* infile = "energy_spectrum_electrons_ptd_merged_week_2025_X.txt",
    double binWidth_keV = 5.0)
{
    TCanvas* canvas_spectrum =
        new TCanvas("canvas_spectrum", "Total Energy Spectrum BOTH", 800, 600);

    FILE* f = fopen(infile, "r");
    if (!f) {
        printf("Cannot open %s\n", infile);
        return;
    }

    std::vector<std::pair<long long,double>> rows;
    rows.reserve(100000);

    char line[256];
    while (fgets(line, sizeof(line), f)) {
        if (line[0] == '#' || line[0] == '\n') continue;

        long long cnt = 0;
        double E = 0.0;

        if (sscanf(line, "%lld %lf", &cnt, &E) == 2 && cnt > 0) {
            rows.emplace_back(cnt, E);
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
                       "Total Energy spectrum BOTH;Energy [keV];Entries",
                       nbins, lo, lo + nbins * binWidth_keV);

    for (const auto& r : rows) {
        h->Fill(r.second, (double)r.first);
    }

    h->SetStats(0);
    h->SetLineWidth(2);
    h->Draw("HIST");

    canvas_spectrum->SaveAs("tot_energy_spectrum-BOTH.png");

    TFile fout("tot_energy_spectrum-BOTH.root", "RECREATE");
    h->Write();
    fout.Close();

    printf("Histogram: %d bins from %.3f to %.3f keV (bin=%.3f keV)\n",
           nbins, lo, lo + nbins * binWidth_keV, binWidth_keV);
}
