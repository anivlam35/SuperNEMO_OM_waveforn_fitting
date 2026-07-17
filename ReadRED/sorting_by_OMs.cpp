#include <TFile.h>
#include <TTree.h>
#include <vector>
#include <iostream>
#include <unordered_map>

int main() {

    TFile *fin = new TFile("./build/waveforms.root", "READ");
    TTree *tree = (TTree*)fin->Get("t");

    // Вхідні змінні
    int16_t om_num;
    int32_t run, event, ht, lt;
    std::vector<int16_t>* waveform = nullptr;

    tree->SetBranchAddress("om_num", &om_num);
    tree->SetBranchAddress("run_id", &run);
    tree->SetBranchAddress("event_id", &event);
    tree->SetBranchAddress("high_threshold", &ht);
    tree->SetBranchAddress("low_threshold", &lt);
    tree->SetBranchAddress("waveform", &waveform);

    // Вихід
    TFile *fout = new TFile("./build/sorted_by_OM.root", "RECREATE");

    std::unordered_map<int, TTree*> trees;

    // Буфери (ДУЖЕ ВАЖЛИВО — не pointer)
    int16_t out_om;
    int32_t out_run, out_event, out_ht, out_lt;
    std::vector<int16_t> out_waveform;

    int nEntries = tree->GetEntries();

    for (int i = 0; i < nEntries; i++) {
        tree->GetEntry(i);

        if (!waveform) continue;

        // створення дерева при першій появі OM
        auto &t = trees[om_num];
        if (!t) {
            fout->cd();

            t = new TTree(Form("t_om_%d", om_num),
                          Form("Waveforms for OM %d", om_num));

            t->Branch("om_num", &out_om);
            t->Branch("run_id", &out_run);
            t->Branch("event_id", &out_event);
            t->Branch("high_threshold", &out_ht);
            t->Branch("low_threshold", &out_lt);
            t->Branch("waveform", &out_waveform);
        }

        // копіюємо дані
        out_om = om_num;
        out_run = run;
        out_event = event;
        out_ht = ht;
        out_lt = lt;
        out_waveform = *waveform;

        t->Fill();
    }

    fin->Close();

    std::cout << "Created " << trees.size() << " trees\n";

    fout->cd();
    for (auto &p : trees) {
        if (!p.second) continue;
        p.second->Write("", TObject::kOverwrite);
    }

    fout->Close();

    return 0;
}