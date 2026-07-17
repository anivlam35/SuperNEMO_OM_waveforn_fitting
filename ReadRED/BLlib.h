#pragma once

#include <TTree.h>
#include <TH1D.h>
#include <TF1.h>
#include <TROOT.h>

#include <vector>
#include <cmath>
#include <string>
#include <omp.h>

inline double get_sample_baseline(
    TTree* tree,
    int sample,
    int32_t &ht,
    int32_t &lt,
    std::vector<int16_t>* &waveform
){
    // unique names per thread/sample
    std::string hname =
        "h_" + std::to_string(omp_get_thread_num()) +
        "_" + std::to_string(sample);

    std::string fname =
        "f_" + std::to_string(omp_get_thread_num()) +
        "_" + std::to_string(sample);

    TH1D histo(
        hname.c_str(),
        "",
        2000,
        2000,
        4000
    );

    // IMPORTANT
    histo.SetDirectory(nullptr);

    double xmin = 1e9;
    double xmax = -1e9;

    const Long64_t nEntries = tree->GetEntries();

    for (Long64_t i = 0; i < nEntries; i++) {

        tree->GetEntry(i);

        if (ht || lt) continue;

        double value = waveform->at(sample);

        if (value > xmax) xmax = value;
        if (value < xmin) xmin = value;

        histo.Fill(value);
    }

    if (histo.GetEntries() < 100) {
        return NAN;
    }

    xmin -= 20.0;
    xmax += 20.0;

    TF1 gaus(
        fname.c_str(),
        "gaus",
        xmin,
        xmax
    );

    histo.Fit(&gaus, "Q0");

    return gaus.GetParameter(1);
}


inline double get_sample_baseline_average(
    TTree* tree,
    int sample,
    int32_t &ht,
    int32_t &lt,
    std::vector<int16_t>* &waveform
){
    double sum = 0.0;
    int counts = 0;

    const Long64_t nEntries = tree->GetEntries();

    for (Long64_t i = 0; i < nEntries; i++) {

        tree->GetEntry(i);

        if (ht || lt) continue;

        sum += waveform->at(sample);

        counts++;
    }

    if (counts < 100) {
        return NAN;
    }

    return sum / counts;
}

inline double get_sample_baseline_mode(
    TTree* tree,
    int sample,
    int32_t &ht,
    int32_t &lt,
    std::vector<int16_t>* &waveform
){
    std::string hname =
        "h_mode_" + std::to_string(omp_get_thread_num()) +
        "_" + std::to_string(sample);

    TH1D h(
        hname.c_str(),
        "",
        600, 3300.25, 3600.25
    );

    h.SetDirectory(nullptr);

    const Long64_t nEntries = tree->GetEntries();

    for (Long64_t i = 0; i < nEntries; i++) {
        tree->GetEntry(i);

        if (ht || lt) continue;

        h.Fill(waveform->at(sample));
    }

    if (h.GetEntries() < 100)
        return NAN;

    int maxBin = h.GetMaximumBin();
    return h.GetBinCenter(maxBin);
}