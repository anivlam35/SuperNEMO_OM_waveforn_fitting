// reading_root_OM_SPECTRA_PTD_CD_OMNUM.C

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
#include <sstream>
#include <limits>
#include <cstdlib>

R__LOAD_LIBRARY(/sps/nemo/scratch/ykozina/Falaise/tutorial/MiModule/lib/libMiModule.so);

// ============================================================
// SETTINGS
// ============================================================
const std::string INPUT_ROOT =
"/sps/nemo/scratch/ykozina/Falaise/calibration/calibration-CORRECTIONS/BOTH/calibrated_brio/SNCUTS/15-40/ROOT/merged_week_2025_X.root";

const std::string OUTPUT_DIR =
"/sps/nemo/scratch/ykozina/Falaise/calibration/calibration-CORRECTIONS/BOTH/calibrated_brio/SNCUTS/15-40/ROOT/general_spect_txt/";

const double BIN_KEV = 5.0;

// tolerant matching PTD hit -> CD hit
const double MATCH_E_TOL_KEV = 1e-3;
const double MATCH_T_TOL_NS  = 1e-3;

// ============================================================
// HELPERS
// ============================================================
static int to_int_safe(const std::string& s)
{
    if (s.empty() || s == "*") return -1;
    return std::atoi(s.c_str());
}

static std::string make_om_key(MiGID* gid)
{
    std::ostringstream os;
    os << "T" << gid->gettype()
       << "_M" << gid->getmodule()
       << "_S" << gid->getside()
       << "_W" << gid->getwall()
       << "_C" << gid->getcolumn()
       << "_R" << gid->getrow();
    return os.str();
}

// Global OM numbering exactly as in your later analysis code
static int gid_to_global_omnum(MiGID* gid)
{
    if (!gid) return -1;

    const int type   = to_int_safe(gid->gettype());
    const int side   = to_int_safe(gid->getside());
    const int wall   = to_int_safe(gid->getwall());
    const int column = to_int_safe(gid->getcolumn());
    const int row    = to_int_safe(gid->getrow());

    // MW
    if (type == 1302) {
        if (!(side == 0 || side == 1)) return -1;
        if (wall < 0 || wall >= 20) return -1;
        if (column < 0 || column >= 13) return -1;

        const int base = (side == 0) ? 0 : 260;
        return base + 13 * wall + column;
    }

    // XW
    if (type == 1232) {
        if (!(side == 0 || side == 1)) return -1;
        if (!(wall == 0 || wall == 1)) return -1;
        if (column < 0 || column >= 2) return -1;
        if (row < 0 || row >= 16) return -1;

        const int base = (side == 0) ? 520 : 584;
        return base + 32 * wall + 16 * column + row;
    }

    // GV
    if (type == 1252) {
        if (!(side == 0 || side == 1)) return -1;
        if (!(wall == 0 || wall == 1)) return -1;
        if (column < 0 || column >= 16) return -1;

        const int base = (side == 0) ? 648 : 680;
        return base + 16 * wall + column;
    }

    return -1;
}

static MiCDCaloHit* match_ptd_hit_to_cd_hit(MiCDCaloHit* ptdHit,
                                            MiCD* cd,
                                            std::vector<bool>& cdUsed)
{
    if (!ptdHit || !cd) return nullptr;

    const double e_ptd = ptdHit->getE();
    const double t_ptd = ptdHit->gett();

    const int ncd = cd->getnoofcaloh();

    int bestIdx = -1;
    double bestScore = std::numeric_limits<double>::max();

    for (int ih = 0; ih < ncd; ++ih) {
        if (ih < (int)cdUsed.size() && cdUsed[ih]) continue;

        MiCDCaloHit* cdHit = cd->getcalohit(ih);
        if (!cdHit) continue;

        const double e_cd = cdHit->getE();
        const double t_cd = cdHit->gett();

        if (!std::isfinite(e_cd) || !std::isfinite(t_cd)) continue;

        const double dE = std::fabs(e_cd - e_ptd);
        const double dT = std::fabs(t_cd - t_ptd);

        if (dE > MATCH_E_TOL_KEV) continue;
        if (dT > MATCH_T_TOL_NS)  continue;

        const double score = dE + dT;
        if (score < bestScore) {
            bestScore = score;
            bestIdx = ih;
        }
    }

    if (bestIdx < 0) return nullptr;

    cdUsed[bestIdx] = true;
    return cd->getcalohit(bestIdx);
}

// ============================================================
// MAIN
// ============================================================
void reading_root_OM_SPECTRA()
{
    TFile* f = TFile::Open(INPUT_ROOT.c_str(), "READ");
    if (!f || f->IsZombie()) {
        std::cerr << "ERROR: cannot open input file: " << INPUT_ROOT << "\n";
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

    MiEvent* Eve = new MiEvent();
    s->SetBranchAddress("Eventdata", &Eve);

    const Long64_t N = s->GetEntries();

    std::cout << "Input: " << INPUT_ROOT << "\n";
    std::cout << "Entries: " << N << "\n";
    std::cout << "Bin width: " << BIN_KEV << " keV\n";
    std::cout << "Output dir: " << OUTPUT_DIR << "\n";

    // OM_num -> (bin -> counts)
    std::map<int, std::map<long long, long long>> spectraPerOM;
    std::map<int, std::string> omnumToKey;

    long long n_events_processed   = 0;
    long long n_events_with_ptd    = 0;
    long long n_events_with_cd     = 0;
    long long n_particles_total    = 0;
    long long n_selected_particles = 0;
    long long n_particles_with_hit = 0;
    long long n_matched_hits       = 0;
    long long n_unmatched_hits     = 0;
    long long n_used_hits          = 0;
    long long n_bad_gid            = 0;

    for (Long64_t i = 0; i < N; ++i)
    {
        s->GetEntry(i);
        n_events_processed++;

        MiPTD* ptd = Eve->getPTD();
        if (!ptd) {
            if ((i + 1) % 100000 == 0) {
                std::cout << "Processed " << (i + 1) << " / " << N
                          << "  PTD=" << n_events_with_ptd
                          << "  CD=" << n_events_with_cd
                          << "  particles=" << n_particles_total
                          << "  selected=" << n_selected_particles
                          << "  withHit=" << n_particles_with_hit
                          << "  matched=" << n_matched_hits
                          << "  unmatched=" << n_unmatched_hits
                          << "  usedHits=" << n_used_hits
                          << "  badGID=" << n_bad_gid
                          << "  nOM=" << spectraPerOM.size() << "\n";
            }
            continue;
        }
        n_events_with_ptd++;

        MiCD* cd = Eve->getCD();
        if (!cd) {
            if ((i + 1) % 100000 == 0) {
                std::cout << "Processed " << (i + 1) << " / " << N
                          << "  PTD=" << n_events_with_ptd
                          << "  CD=" << n_events_with_cd
                          << "  particles=" << n_particles_total
                          << "  selected=" << n_selected_particles
                          << "  withHit=" << n_particles_with_hit
                          << "  matched=" << n_matched_hits
                          << "  unmatched=" << n_unmatched_hits
                          << "  usedHits=" << n_used_hits
                          << "  badGID=" << n_bad_gid
                          << "  nOM=" << spectraPerOM.size() << "\n";
            }
            continue;
        }
        n_events_with_cd++;

        std::vector<MiCDParticle>* partv = ptd->getpartv();
        if (!partv) {
            if ((i + 1) % 100000 == 0) {
                std::cout << "Processed " << (i + 1) << " / " << N
                          << "  PTD=" << n_events_with_ptd
                          << "  CD=" << n_events_with_cd
                          << "  particles=" << n_particles_total
                          << "  selected=" << n_selected_particles
                          << "  withHit=" << n_particles_with_hit
                          << "  matched=" << n_matched_hits
                          << "  unmatched=" << n_unmatched_hits
                          << "  usedHits=" << n_used_hits
                          << "  badGID=" << n_bad_gid
                          << "  nOM=" << spectraPerOM.size() << "\n";
            }
            continue;
        }

        const int npart = (int)partv->size();
        n_particles_total += npart;

        std::vector<bool> cdUsed(cd->getnoofcaloh(), false);

        for (int ip = 0; ip < npart; ++ip)
        {
            MiCDParticle* particle = ptd->getpart(ip);
            if (!particle) continue;

            // your working PTD selection
            if (particle->getcharge() != 1000) continue;
            n_selected_particles++;

            std::vector<MiCDCaloHit>* calov = particle->getcalohitv();
            if (!calov || calov->empty()) continue;
            n_particles_with_hit++;

            MiCDCaloHit* ptdHit = particle->getcalohit(0);
            if (!ptdHit) continue;

            const double E_keV = ptdHit->getE();
            if (!std::isfinite(E_keV)) continue;

            MiCDCaloHit* cdHit = match_ptd_hit_to_cd_hit(ptdHit, cd, cdUsed);
            if (!cdHit) {
                n_unmatched_hits++;
                continue;
            }
            n_matched_hits++;

            MiGID* gid = cdHit->getGID();
            if (!gid) {
                n_bad_gid++;
                continue;
            }

            const int omnum = gid_to_global_omnum(gid);
            if (omnum < 0 || omnum > 711) {
                n_bad_gid++;
                continue;
            }

            const std::string omKey = make_om_key(gid);
            omnumToKey[omnum] = omKey;

            const long long binIdx = (long long)std::floor(E_keV / BIN_KEV);
            spectraPerOM[omnum][binIdx]++;
            n_used_hits++;
        }

        if ((i + 1) % 100000 == 0) {
            std::cout << "Processed " << (i + 1) << " / " << N
                      << "  PTD=" << n_events_with_ptd
                      << "  CD=" << n_events_with_cd
                      << "  particles=" << n_particles_total
                      << "  selected=" << n_selected_particles
                      << "  withHit=" << n_particles_with_hit
                      << "  matched=" << n_matched_hits
                      << "  unmatched=" << n_unmatched_hits
                      << "  usedHits=" << n_used_hits
                      << "  badGID=" << n_bad_gid
                      << "  nOM=" << spectraPerOM.size() << "\n";
        }
    }

    std::cout << "\nFinished event loop.\n";
    std::cout << "Total processed events: " << n_events_processed << "\n";
    std::cout << "Events with PTD: " << n_events_with_ptd << "\n";
    std::cout << "Events with CD: " << n_events_with_cd << "\n";
    std::cout << "Total PTD particles seen: " << n_particles_total << "\n";
    std::cout << "Selected particles: " << n_selected_particles << "\n";
    std::cout << "Selected particles with PTD calo hit: " << n_particles_with_hit << "\n";
    std::cout << "Matched PTD->CD hits: " << n_matched_hits << "\n";
    std::cout << "Unmatched PTD hits: " << n_unmatched_hits << "\n";
    std::cout << "Bad/unusable GID: " << n_bad_gid << "\n";
    std::cout << "Hits written to spectra: " << n_used_hits << "\n";
    std::cout << "OM spectra built: " << spectraPerOM.size() << "\n";

    // mapping file
    {
        std::ofstream mapout((OUTPUT_DIR + "OM_index_map_merged_week_2025_X-BOTH.txt").c_str());
        if (mapout) {
            mapout << "# OM_num  OM_key\n";
            for (const auto& kv : omnumToKey) {
                mapout << kv.first << " " << kv.second << "\n";
            }
            mapout.close();
        }
    }

    // one txt per OM number
    for (const auto& omEntry : spectraPerOM)
    {
        const int omnum = omEntry.first;
        const auto& binCounts = omEntry.second;

        std::ostringstream fname;
        fname << OUTPUT_DIR
              << "OM_"
              << omnum
              << "-merged_week_2025_X-BOTH.txt";

        std::ofstream out(fname.str().c_str());
        if (!out) {
            std::cerr << "Cannot open output file " << fname.str() << "\n";
            continue;
        }

        for (const auto& kv : binCounts)
        {
            const long long binIdx = kv.first;
            const long long count  = kv.second;
            const double energy_keV_center = (binIdx + 0.5) * BIN_KEV;

            out << count << " "
                << std::fixed << std::setprecision(3)
                << energy_keV_center << "\n";
        }

        out.close();

        std::cout << "Saved spectrum: OM_" << omnum
                  << "  key=" << omnumToKey[omnum]
                  << "  bins=" << binCounts.size()
                  << "  file=" << fname.str() << "\n";
    }

    f->Close();
    delete f;
    delete Eve;
}
