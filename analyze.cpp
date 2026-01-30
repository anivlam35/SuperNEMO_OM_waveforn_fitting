    
    void analyze()
    {
        TFile* f = new TFile("./MiModule.root");
        TTree* strom =(TTree *)(f->Get("Event"));
        strom->GetListOfBranches()->Print();
        int n_calos, n_electrons = 0;
        double calo_energy;
        bool are_two_tracks;
        strom->SetBranchAddress("reco.calorimeter_hit_count", &n_calos);
        strom->SetBranchAddress("reco.passes_two_tracks", &are_two_tracks);
        strom->SetBranchAddress("reco.number_of_electrons", &n_electrons);
        strom->SetBranchAddress("reco.total_calorimeter_energy", &calo_energy);
        int passed1 = 0;
        int passed2 = 0;

        int passed3 = 0;
        int passed4 = 0;
        int N = strom->GetEntries();
        for (int i = 0; i < N; i++)
        {
            strom->GetEntry(i);
            if (n_calos == 2) passed1++;
            if (n_calos == 2 && are_two_tracks) passed2++;
            if (n_calos == 2 && are_two_tracks && n_electrons == 2) passed3++;
            if (n_calos == 2 && are_two_tracks && n_electrons == 2 && calo_energy > 2.0) passed4++;
        }
        cout << endl << "EFFICIENCIES :" << endl;
        cout << "eps1 = " << (100.0 * passed1) / N << "% +- " << (100.0 * sqrt(double(passed1)) ) / N << "%" << endl;
        cout << "eps2 = " << (100.0 * passed2) / N << "% +- " << (100.0 * sqrt(double(passed2)) ) / N << "%" << endl;
        cout << "eps3 = " << (100.0 * passed3) / N << "% +- " << (100.0 * sqrt(double(passed3)) ) / N << "%" << endl;
        cout << "eps4 = " << (100.0 * passed4) / N << "% +- " << (100.0 * sqrt(double(passed4)) ) / N << "%" << endl;
    
    
    }