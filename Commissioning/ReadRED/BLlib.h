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

double get_sample_baseline(
    TTree* tree,
    int sample,
    int32_t &ht,
    int32_t &lt,
    std::vector<int16_t>* &waveform
){
    TH1D *histo = new TH1D(
        "h2_om",
        "OM SAMPLE baseline (substracted BL);Time;Amplitude",
        2e3, 2e3, 4e3
    );

    double xmin = 1e9;
    double xmax = -1e9;

    int nEntries = tree->GetEntries();
    
    for (int i = 0; i < nEntries; i++) {
        tree->GetEntry(i);
        if ((ht == 1) || (lt == 1)) continue;                
        double value = waveform->at(sample);
        if (value > xmax) xmax = value;
        if (value < xmin) xmin = value;
        histo->Fill(value);
    }
    if (histo->GetEntries() < 100){
        delete histo;
        return NAN;
    }

    xmax += 20;
    xmin -= 20;
    histo->GetXaxis()->SetRangeUser(xmin, xmax);

    TF1 *gaus = new TF1("gaus", "gaus", xmin, xmax);
    histo->Fit(gaus, "Q");

    double mean = gaus->GetParameter(1);
    delete histo;
    delete gaus;
    return mean;
}

double get_sample_baseline_average(
    TTree* tree,
    int sample,
    int32_t &ht,
    int32_t &lt,
    std::vector<int16_t>* &waveform
){
    int counts = 0;
    double sum = 0;
    double avg;

    int nEntries = tree->GetEntries();
    
    for (int i = 0; i < nEntries; i++) {
        tree->GetEntry(i);
        if ((ht == 1) || (lt == 1)) continue;                
        sum += waveform->at(sample);
        counts++;
    }
    if (counts < 100){
        return NAN;
    }

    avg = sum / counts;
    return avg;
}

double get_general_baseline(
    TTree* tree,
    int32_t &ht,
    int32_t &lt,
    std::vector<int16_t>* &waveform
){
    TH1D *histo = new TH1D(
        "h2_om",
        "OM baseline (substracted BL);Time;Amplitude",
        2e3, 2e3, 4e3
    );

    double xmin = 1e9;
    double xmax = -1e9;

    int nEntries = tree->GetEntries();
    
    for (int i = 0; i < nEntries; i++) {
        tree->GetEntry(i);
        if ((ht == 1) || (lt == 1)) continue;
        for (int sample = 0; sample < 1024; sample++){                
            double value = waveform->at(sample);
            if (value > xmax) xmax = value;
            if (value < xmin) xmin = value;
            histo->Fill(value);
        }
    }
    if (histo->GetEntries() < 100){
        delete histo;
        return NAN;
    }

    xmax += 20;
    xmin -= 20;
    histo->GetXaxis()->SetRangeUser(xmin, xmax);

    TF1 *gaus = new TF1("gaus", "gaus", xmin, xmax);
    histo->Fit(gaus, "R");

    double mean = gaus->GetParameter(1);
    delete histo;
    delete gaus;
    return mean;
}

