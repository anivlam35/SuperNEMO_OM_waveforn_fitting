#include <TFile.h>
#include <TTree.h>
#include <TGraph.h>
#include <TCanvas.h>
#include <vector>
#include <iostream>

int main() {

    // Відкрити вхідний файл
    TFile *fin = new TFile("./build/waveforms.root", "READ");
    TTree *tree = (TTree*)fin->Get("t");

    // Підключити гілку
    int run, event, om;
    std::vector<double>* waveform = nullptr;

    tree->SetBranchAddress("run_id", &run);
    tree->SetBranchAddress("event_id", &event);
    // tree->SetBranchAddress("om_id", &om);
    tree->SetBranchAddress("waveform", &waveform);

    TCanvas canvas("c","c",800,600);

    int nEntries = tree->GetEntries();

    for (int i = 0; i < nEntries; i++) {

        tree->GetEntry(i);

        int n = waveform->size();

        TGraph graph(n);

        for (int j = 0; j < n; j++) {
            double timestamp = j * 6.25E-9;
            graph.SetPoint(j, timestamp, waveform->at(j));
        }

        graph.SetTitle("Waveform;Time;Amplitude");
        graph.Draw("AL");

        std::string filename = "Histos/Waveform_run" +
                                std::to_string(run) +
                                "_event" +
                                std::to_string(event) +
                                // "_om" +
                                // std::to_string(om) +
                                std::to_string(i) +
                                ".png";

        canvas.SaveAs(filename.c_str());
    }

    fin->Close();

    std::cout << "Saved " << nEntries << " graphs\n";
    return 0;
}