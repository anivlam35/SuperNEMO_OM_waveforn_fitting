#include <TFile.h>
#include <TTree.h>
#include <TGraph.h>
#include <vector>
#include <iostream>

int main() {

    // Відкрити вхідний файл
    TFile *fin = new TFile("waveforms.root", "READ");
    TTree *t = (TTree*)fin->Get("t");

    // Підключити гілку
    std::vector<short>* wf = nullptr;
    t->SetBranchAddress("waveform", &wf);

    // Створити вихідний файл
    TFile *fout = new TFile("graphs.root", "RECREATE");

    int nEntries = t->GetEntries();

    for (int i = 0; i < nEntries; i++) {

        t->GetEntry(i);

        // Створити графік
        TGraph *g = new TGraph(wf->size());

        for (int j = 0; j < wf->size(); j++)
            g->SetPoint(j, j, (*wf)[j]);

        // Унікальне ім’я
        g->SetName(Form("waveform_%d", i));
        g->SetTitle(Form("Waveform %d", i));

        g->Write();   // записати в файл

        delete g;     // щоб не текла пам’ять
    }

    fout->Close();
    fin->Close();

    std::cout << "Saved " << nEntries << " graphs\n";
    return 0;
}