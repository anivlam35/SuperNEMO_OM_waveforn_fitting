// reading_root_TOTAL_ELECTRON_SPECTRUM_PTD.C

#include "/sps/nemo/scratch/ykozina/Falaise/tutorial/MiModule/include/MiEvent.h"

#include <TFile.h>
#include <TTree.h>

#include <iostream>
#include <vector>
#include <map>
#include <fstream>
#include <iomanip>
#include <cmath>
#include <string>

R__LOAD_LIBRARY(/sps/nemo/scratch/ykozina/Falaise/tutorial/MiModule/lib/libMiModule.so);

void reading_root_totE()
{
    const std::string INPUT_ROOT =
        "/sps/nemo/scratch/ykozina/Falaise/calibration/calibration-CORRECTIONS/OPTICAL/calibrated_brio/SNCUTS/sncuts_NEW_positions/merged_week_2025_X-OPTIC.root";

    const std::string OUTPUT_TXT =
        "/sps/nemo/scratch/ykozina/Falaise/calibration/calibration-CORRECTIONS/OPTICAL/calibrated_brio/SNCUTS/sncuts_NEW_positions/energy_spectrum_electrons_ptd_merged_week_2025_X.txt";

    const double BIN_KEV = 5.0;

    TFile* f = TFile::Open(INPUT_ROOT.c_str(), "READ");
    if (!f || f->IsZombie()) {
        std::cerr << "ERROR: cannot open input ROOT file: " << INPUT_ROOT << "\n";
        if (f) {
            f->Close();
            delete f;
        }
        return;
    }

    TTree* s = dynamic_cast<TTree*>(f->Get("Event"));
    if (!s) {
        std::cerr << "ERROR: Tree 'Event' not found\n";
        f->Close();
        delete f;
        return;
    }

    const Long64_t N = s->GetEntries();

    std::map<long long, long long> binCounts;

    MiEvent* Eve = new MiEvent();
    s->SetBranchAddress("Eventdata", &Eve);

    long long n_events_processed   = 0;
    long long n_events_with_ptd    = 0;
    long long n_particles_total    = 0;
    long long n_selected_particles = 0;
    long long n_hits_used          = 0;

    std::cout << "Input: " << INPUT_ROOT << "\n";
    std::cout << "Entries: " << N << "\n";
    std::cout << "Bin width: " << BIN_KEV << " keV\n";
    std::cout << "Output: " << OUTPUT_TXT << "\n";

    for (Long64_t i = 0; i < N; ++i)
    {
        s->GetEntry(i);
        n_events_processed++;

        MiPTD* ptd = Eve->getPTD();
        if (!ptd) {
            if ((i + 1) % 100000 == 0) {
                std::cout << "Processed " << (i + 1) << " / " << N
                          << "  events_with_PTD=" << n_events_with_ptd
                          << "  particles=" << n_particles_total
                          << "  selected=" << n_selected_particles
                          << "  used_hits=" << n_hits_used
                          << "  filled_bins=" << binCounts.size() << "\n";
            }
            continue;
        }

        n_events_with_ptd++;

        std::vector<MiCDParticle>* partv = ptd->getpartv();
        if (!partv) {
            if ((i + 1) % 100000 == 0) {
                std::cout << "Processed " << (i + 1) << " / " << N
                          << "  events_with_PTD=" << n_events_with_ptd
                          << "  particles=" << n_particles_total
                          << "  selected=" << n_selected_particles
                          << "  used_hits=" << n_hits_used
                          << "  filled_bins=" << binCounts.size() << "\n";
            }
            continue;
        }

        const int nParts = (int)partv->size();
        n_particles_total += nParts;

        for (int ip = 0; ip < nParts; ++ip)
        {
            MiCDParticle* particle = ptd->getpart(ip);
            if (!particle) continue;

            // IMPORTANT: for your data selection use 1000
            if (particle->getcharge() != 1000) continue;

            n_selected_particles++;

            std::vector<MiCDCaloHit>* calov = particle->getcalohitv();
            if (!calov || calov->empty()) continue;

            MiCDCaloHit* hit = particle->getcalohit(0);
            if (!hit) continue;

            const double E_keV = hit->getE();
            if (!std::isfinite(E_keV)) continue;

            const long long binIdx = (long long)std::floor(E_keV / BIN_KEV);
            binCounts[binIdx]++;
            n_hits_used++;
        }

        if ((i + 1) % 100000 == 0) {
            std::cout << "Processed " << (i + 1) << " / " << N
                      << "  events_with_PTD=" << n_events_with_ptd
                      << "  particles=" << n_particles_total
                      << "  selected=" << n_selected_particles
                      << "  used_hits=" << n_hits_used
                      << "  filled_bins=" << binCounts.size() << "\n";
        }
    }

    std::cout << "Finished event loop.\n";
    std::cout << "Total processed events: " << n_events_processed << "\n";
    std::cout << "Events with PTD: " << n_events_with_ptd << "\n";
    std::cout << "Total PTD particles seen: " << n_particles_total << "\n";
    std::cout << "Selected particles: " << n_selected_particles << "\n";
    std::cout << "Hits written to spectrum: " << n_hits_used << "\n";
    std::cout << "Filled bins: " << binCounts.size() << "\n";

    std::ofstream out(OUTPUT_TXT.c_str());
    if (!out) {
        std::cerr << "ERROR: cannot open output txt file: " << OUTPUT_TXT << "\n";
        f->Close();
        delete f;
        delete Eve;
        return;
    }

    for (const auto& kv : binCounts) {
        const long long binIdx = kv.first;
        const long long count  = kv.second;
        const double energy_keV_center = (binIdx + 0.5) * BIN_KEV;

        out << count << " "
            << std::fixed << std::setprecision(3)
            << energy_keV_center << "\n";
    }

    out.close();

    std::cout << "Saved spectrum to: " << OUTPUT_TXT << "\n";
    std::cout << "Lines written: " << binCounts.size() << "\n";

    f->Close();
    delete f;
    delete Eve;
}
