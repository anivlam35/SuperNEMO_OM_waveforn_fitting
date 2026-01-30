#include <iostream>
#include <string>
#include <cmath>
#include <TFile.h>
#include <TTree.h>

#include "./MiModule/include/MiEvent.h"
// R__LOAD_LIBRARY(./MiModule/lib/libMiModule.so);


void analyze(const std::string& filePath, int& n_calos, int& n_electrons, double& calo_energy, int& are_two_tracks, int& passed1, int& passed2, int& passed3, int& passed4, int& totalEntries)
{
    TFile* f = new TFile(filePath.c_str());
    if (!f || f->IsZombie()) {
        std::cerr << "Error opening file: " << filePath << std::endl;
        return;
    }
    TTree* strom = (TTree*) f->Get("SimData");
    // TTree* strom = (TTree*)(f->Get("Sensitivity"));
    MiEvent* Eve = new MiEvent();
    if (!strom) {
        std::cerr << "Error: TTree 'Event' not found in file: " << filePath << std::endl;
        return;
    }
    strom->SetBranchAddress("Eventdata", &Eve);
    // strom->SetBranchAddress("reco.calorimeter_hit_count", &n_calos);
    // strom->SetBranchAddress("reco.passes_two_tracks", &are_two_tracks);
    // strom->SetBranchAddress("reco.number_of_electrons", &n_electrons);
    // strom->SetBranchAddress("reco.total_calorimeter_energy", &calo_energy);

    int N = strom->GetEntries();

    int n_calo;

    totalEntries += N;

    for (UInt_t i = 0; i < N; i++)
    {
        strom->GetEntry(i);
        n_calo = 0;
        n_calos = 0;
        n_electrons = 0;
        calo_energy = 0.0;


        are_two_tracks = Eve->getPTD()->getpartv()->size();

        for(int j = 0; j < are_two_tracks; j++)
        {
            n_calo = Eve->getPTD()->getpartv()->at(j).getcalohitv()->size();
            if(Eve->getPTD()->getpartv()->at(j).getcharge() == -1) n_electrons++;
            for(int k = 0; k < n_calo; k++)
            {
                calo_energy += Eve->getPTD()->getpartv()->at(j).getcalohitv()->at(k).getE();
            }
            n_calos += n_calo;
        }

        if (n_calos == 2) passed1++;
        if (n_calos == 2 && are_two_tracks ==2) passed2++;
        if (n_calos == 2 && are_two_tracks ==2 && n_electrons == 2) passed3++;
        if (n_calos == 2 && are_two_tracks ==2 && n_electrons == 2 && calo_energy > 2.0) passed4++;
    }
    delete Eve;
    delete f;
}

int main(int argc, char* argv[])
{
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <path_to_root_file_prefix> <num_files>" << std::endl;
        return 1;
    }

    std::string filePathPrefix = argv[1];
    int num_files = std::stoi(argv[2]);

    int n_calos = 0, n_electrons = 0;
    double calo_energy = 0.0;
    int are_two_tracks = 0;

    int passed1 = 0;
    int passed2 = 0;
    int passed3 = 0;
    int passed4 = 0;
    int totalEntries = 0;
    
    for (int f = 0; f < num_files; f++)
    {
        std::string filePath = filePathPrefix + "/" + std::to_string(f) + "/MiModule.root";
        analyze(filePath, n_calos, n_electrons, calo_energy, are_two_tracks, passed1, passed2, passed3, passed4, totalEntries);
    }

    std::cout << std::endl << "EFFICIENCIES :" << std::endl;
    std::cout << "eps1 = " << (100.0 * passed1) / totalEntries << "% +- " << (100.0 * std::sqrt(double(passed1))) / totalEntries << "%" << std::endl;
    std::cout << "eps2 = " << (100.0 * passed2) / totalEntries << "% +- " << (100.0 * std::sqrt(double(passed2))) / totalEntries << "%" << std::endl;
    std::cout << "eps3 = " << (100.0 * passed3) / totalEntries << "% +- " << (100.0 * std::sqrt(double(passed3))) / totalEntries << "%" << std::endl;
    std::cout << "eps4 = " << (100.0 * passed4) / totalEntries << "% +- " << (100.0 * std::sqrt(double(passed4))) / totalEntries << "%" << std::endl;

    return 0;
}
