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
#include <algorithm>

R__LOAD_LIBRARY(/sps/nemo/scratch/ykozina/Falaise/tutorial/MiModule/lib/libMiModule.so);

// ============================================================
// SETTINGS
// ============================================================
const std::string INPUT_ROOT =
    "/sps/nemo/scratch/ykozina/Falaise/calibration/calibration-CORRECTIONS/BOTH/calibrated_brio/SNCUTS/15-40/ROOT/merged_week_2025_X.root";

const std::string OUTPUT_TXT =
    "/sps/nemo/scratch/ykozina/Falaise/calibration/calibration-CORRECTIONS/BOTH/calibrated_brio/SNCUTS/15-40/ROOT/tot_spectra/txt_angle/energy_spectrum_angleCut_60to90deg.txt";

const double ANGLE_MIN_DEG = 60.0;
const double ANGLE_MAX_DEG = 90.0;  // <0 => single-angle mode
const double ANGLE_TOL_DEG = 0.5;   // used only if ANGLE_MAX_DEG < 0
const double BIN_KEV       = 5.0;

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

static bool finite3(double x, double y, double z)
{
    return std::isfinite(x) && std::isfinite(y) && std::isfinite(z);
}

static double clamp1(double x)
{
    if (x < -1.0) return -1.0;
    if (x >  1.0) return  1.0;
    return x;
}

// ============================================================
// Geometry from GID
// ============================================================
static bool om_window_center_from_gid(MiGID* gid, double& xw, double& yw, double& zw, int& face_mode)
{
    if (!gid) return false;

    const int type   = to_int_safe(gid->gettype());
    const int side   = to_int_safe(gid->getside());
    const int wall   = to_int_safe(gid->getwall());
    const int column = to_int_safe(gid->getcolumn());
    const int row    = to_int_safe(gid->getrow());

    face_mode = -1;

    static const double mw_sizex = 194.0;
    static const double xw_sizey = 150.0;
    static const double gv_sizez = 150.0;

    if (type == 1302) {
        if (side < 0 || wall < 0 || column < 0) return false;
        const int mw_col = wall;
        const int mw_row = column;
        double xc = (side == 1) ?  532.0 : -532.0;
        double yc = (static_cast<double>(mw_col) - 9.5) * 259.0;
        double zc = (static_cast<double>(mw_row) - 6.0) * 259.0;
        if (!finite3(xc, yc, zc)) return false;
        face_mode = 0;
        xw = (side == 1) ? (xc - mw_sizex / 2.0) : (xc + mw_sizex / 2.0);
        yw = yc;
        zw = zc;
        return true;
    }

    if (type == 1232) {
        if (side < 0 || wall < 0 || column < 0 || row < 0) return false;
        double xc = (side == 1) ? ((column == 1) ?  333.0 :  130.0)
                                : ((column == 1) ? -333.0 : -130.0);
        double yc = (wall == 1) ? 2580.5 : -2580.5;
        double zc = (static_cast<double>(row) - 7.5) * 212.0;
        if (!finite3(xc, yc, zc)) return false;
        face_mode = 1;
        xw = xc;
        yw = (wall == 1) ? (yc - xw_sizey / 2.0) : (yc + xw_sizey / 2.0);
        zw = zc;
        return true;
    }

    if (type == 1252) {
        if (side < 0 || wall < 0 || column < 0) return false;
        double xc = (side == 1) ?  213.5 : -213.5;
        double yc = (column > 7) ? (161.0  + (static_cast<double>(column) - 8.0) * 311.5)
                                 : (-161.0 + (static_cast<double>(column) - 7.0) * 311.5);
        double zc = (wall == 1) ? 1625.0 : -1625.0;
        if (!finite3(xc, yc, zc)) return false;
        face_mode = 2;
        xw = xc;
        yw = yc;
        zw = (wall == 1) ? (zc - gv_sizez / 2.0) : (zc + gv_sizez / 2.0);
        return true;
    }

    return false;
}

// ============================================================
// PTD particle vertex helpers
// ============================================================
static bool get_particle_reference_source_plane_vertex(MiEvent* Eve, int ip,
                                                       double& xs, double& ys, double& zs)
{
    if (!Eve) return false;
    if (ip < 0 || ip >= Eve->getPTDNoPart()) return false;
    const int nVert = Eve->getPTDNoVert(ip);
    for (int iv = 0; iv < nVert; ++iv) {
        std::string where = Eve->getPTDVertpos(ip, iv);
        if (where == "reference source plane") {
            xs = Eve->getPTDverX(ip, iv);
            ys = Eve->getPTDverY(ip, iv);
            zs = Eve->getPTDverZ(ip, iv);
            return finite3(xs, ys, zs);
        }
    }
    return false;
}

static bool get_particle_calo_vertex(MiEvent* Eve, int ip,
                                     double& xv, double& yv, double& zv)
{
    if (!Eve) return false;
    if (ip < 0 || ip >= Eve->getPTDNoPart()) return false;
    const int nVert = Eve->getPTDNoVert(ip);
    for (int iv = 0; iv < nVert; ++iv) {
        std::string where = Eve->getPTDVertpos(ip, iv);
        if (where == "calo" || where == "xcalo" || where == "gveto") {
            xv = Eve->getPTDverX(ip, iv);
            yv = Eve->getPTDverY(ip, iv);
            zv = Eve->getPTDverZ(ip, iv);
            return finite3(xv, yv, zv);
        }
    }
    return false;
}

// ============================================================
// Compute impact angle
// ============================================================
static bool particle_compute_impact_angle_deg(MiEvent* Eve, int ip,
                                              MiCDCaloHit* cdHit, double& theta_deg)
{
    theta_deg = -1.0;
    if (!Eve || !cdHit) return false;

    MiGID* gid = cdHit->getGID();
    if (!gid) return false;

    int face_mode = -1;
    double xw = 0.0, yw = 0.0, zw = 0.0;
    if (!om_window_center_from_gid(gid, xw, yw, zw, face_mode)) return false;

    double xs = 0.0, ys = 0.0, zs = 0.0;
    if (!get_particle_reference_source_plane_vertex(Eve, ip, xs, ys, zs)) return false;

    double xv = 0.0, yv = 0.0, zv = 0.0;
    if (!get_particle_calo_vertex(Eve, ip, xv, yv, zv)) return false;

    const double vx = xv - xs;
    const double vy = yv - ys;
    const double vz = zv - zs;
    const double vnorm = std::sqrt(vx*vx + vy*vy + vz*vz);
    if (!(vnorm > 0.0) || !std::isfinite(vnorm)) return false;

    double cos_theta = 0.0;
    if      (face_mode == 0) cos_theta = std::fabs(vx) / vnorm;
    else if (face_mode == 1) cos_theta = std::fabs(vy) / vnorm;
    else if (face_mode == 2) cos_theta = std::fabs(vz) / vnorm;
    else return false;

    theta_deg = std::acos(clamp1(cos_theta)) * 180.0 / M_PI;
    return std::isfinite(theta_deg);
}

// ============================================================
// Angle cut
// ============================================================
static bool particle_passes_angle_cut(MiEvent* Eve, int ip, MiCDCaloHit* cdHit,
                                      double& theta_deg)
{
    theta_deg = -1.0;
    if (!particle_compute_impact_angle_deg(Eve, ip, cdHit, theta_deg)) return false;

    if (ANGLE_MAX_DEG < 0.0)
        return std::fabs(theta_deg - ANGLE_MIN_DEG) <= ANGLE_TOL_DEG;

    const bool include_upper_90 = (std::fabs(ANGLE_MAX_DEG - 90.0) < 1e-12);
    if (include_upper_90)
        return (theta_deg >= ANGLE_MIN_DEG && theta_deg <= ANGLE_MAX_DEG);

    return (theta_deg >= ANGLE_MIN_DEG && theta_deg < ANGLE_MAX_DEG);
}

// ============================================================
// Match PTD hit -> CD hit
// ============================================================
static MiCDCaloHit* match_ptd_hit_to_cd_hit(MiCDCaloHit* ptdHit, MiCD* cd,
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
        const double dE = std::fabs(cdHit->getE() - e_ptd);
        const double dT = std::fabs(cdHit->gett() - t_ptd);
        if (dE > MATCH_E_TOL_KEV || dT > MATCH_T_TOL_NS) continue;
        const double score = dE + dT;
        if (score < bestScore) { bestScore = score; bestIdx = ih; }
    }

    if (bestIdx < 0) return nullptr;
    cdUsed[bestIdx] = true;
    return cd->getcalohit(bestIdx);
}

// ============================================================
// MAIN
// ============================================================
void reading_root_totE_angle()
{
    TFile* f = TFile::Open(INPUT_ROOT.c_str(), "READ");
    if (!f || f->IsZombie()) {
        std::cerr << "ERROR: cannot open input ROOT file: " << INPUT_ROOT << "\n";
        if (f) { f->Close(); delete f; }
        return;
    }

    TTree* s = dynamic_cast<TTree*>(f->Get("Event"));
    if (!s) {
        std::cerr << "ERROR: Tree 'Event' not found\n";
        f->Close(); delete f;
        return;
    }

    MiEvent* Eve = new MiEvent();
    s->SetBranchAddress("Eventdata", &Eve);

    const Long64_t N = s->GetEntries();

    std::cout << "Input:     " << INPUT_ROOT << "\n";
    std::cout << "Output:    " << OUTPUT_TXT << "\n";
    std::cout << "Entries:   " << N << "\n";
    std::cout << "Bin width: " << BIN_KEV << " keV\n";
    if (ANGLE_MAX_DEG < 0.0)
        std::cout << "Angle cut: |theta - " << ANGLE_MIN_DEG << "| <= " << ANGLE_TOL_DEG << " deg\n";
    else
        std::cout << "Angle cut: " << ANGLE_MIN_DEG << " <= theta < " << ANGLE_MAX_DEG << " deg\n";

    std::map<long long, long long> binCounts;

    long long n_events_processed   = 0;
    long long n_events_with_ptd    = 0;
    long long n_selected_particles = 0;
    long long n_matched_hits       = 0;
    long long n_pass_angle         = 0;
    long long n_fail_angle         = 0;
    long long n_bad_angle          = 0;

    for (Long64_t i = 0; i < N; ++i) {
        s->GetEntry(i);
        n_events_processed++;

        MiPTD* ptd = Eve->getPTD();
        if (!ptd) continue;
        n_events_with_ptd++;

        MiCD* cd = Eve->getCD();
        if (!cd) continue;

        std::vector<MiCDParticle>* partv = ptd->getpartv();
        if (!partv) continue;

        const int npart = (int)partv->size();
        std::vector<bool> cdUsed(cd->getnoofcaloh(), false);

        for (int ip = 0; ip < npart; ++ip) {
            MiCDParticle* particle = ptd->getpart(ip);
            if (!particle) continue;
            if (particle->getcharge() != 1000) continue;
            n_selected_particles++;

            std::vector<MiCDCaloHit>* calov = particle->getcalohitv();
            if (!calov || calov->empty()) continue;

            MiCDCaloHit* ptdHit = particle->getcalohit(0);
            if (!ptdHit) continue;

            const double E_keV = ptdHit->getE();
            if (!std::isfinite(E_keV)) continue;

            MiCDCaloHit* cdHit = match_ptd_hit_to_cd_hit(ptdHit, cd, cdUsed);
            if (!cdHit) continue;
            n_matched_hits++;

            double theta_deg = -1.0;
            if (!particle_passes_angle_cut(Eve, ip, cdHit, theta_deg)) {
                if (!std::isfinite(theta_deg) || theta_deg < 0.0) n_bad_angle++;
                else n_fail_angle++;
                continue;
            }
            n_pass_angle++;

            const long long binIdx = (long long)std::floor(E_keV / BIN_KEV);
            binCounts[binIdx]++;
        }

        if ((i + 1) % 100000 == 0) {
            std::cout << "Processed " << (i + 1) << " / " << N
                      << "  selected=" << n_selected_particles
                      << "  matched=" << n_matched_hits
                      << "  passAngle=" << n_pass_angle
                      << "  failAngle=" << n_fail_angle
                      << "  badAngle=" << n_bad_angle
                      << "  bins=" << binCounts.size() << "\n";
        }
    }

    std::cout << "\nFinished event loop.\n";
    std::cout << "Total processed events:  " << n_events_processed   << "\n";
    std::cout << "Events with PTD:         " << n_events_with_ptd    << "\n";
    std::cout << "Selected particles:      " << n_selected_particles  << "\n";
    std::cout << "Matched PTD->CD hits:    " << n_matched_hits        << "\n";
    std::cout << "Passed angle cut:        " << n_pass_angle          << "\n";
    std::cout << "Failed angle cut:        " << n_fail_angle          << "\n";
    std::cout << "Bad/unavailable angle:   " << n_bad_angle           << "\n";
    std::cout << "Filled bins:             " << binCounts.size()      << "\n";

    std::ofstream out(OUTPUT_TXT.c_str());
    if (!out) {
        std::cerr << "ERROR: cannot open output txt file: " << OUTPUT_TXT << "\n";
        f->Close(); delete f; delete Eve;
        return;
    }

    for (const auto& kv : binCounts) {
        const double energy_keV_center = (kv.first + 0.5) * BIN_KEV;
        out << kv.second << " "
            << std::fixed << std::setprecision(3)
            << energy_keV_center << "\n";
    }

    out.close();
    std::cout << "Saved spectrum to: " << OUTPUT_TXT << "\n";

    f->Close();
    delete f;
    delete Eve;
}
