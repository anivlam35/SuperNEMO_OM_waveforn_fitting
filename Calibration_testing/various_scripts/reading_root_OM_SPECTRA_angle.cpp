// reading_root_OM_SPECTRA_PTD_CD_angleCut_OMNUM.C

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

const std::string OUTPUT_DIR =
"/sps/nemo/scratch/ykozina/Falaise/calibration/calibration-CORRECTIONS/BOTH/calibrated_brio/SNCUTS/15-40/ROOT/txt_anglecut/40-50/";

const double ANGLE_MIN_DEG = 40.0;
const double ANGLE_MAX_DEG = 50.0;   // <0 => single-angle mode
const double ANGLE_TOL_DEG = 0.5;    // used only if ANGLE_MAX_DEG < 0
const double BIN_KEV       = 5.0;

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

static std::string format_double_1(double x)
{
    std::ostringstream os;
    os << std::fixed << std::setprecision(1) << x;
    return os.str();
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

// ============================================================
// GID -> global OM number
// ============================================================
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

// ============================================================
// Geometry from GID
// IMPORTANT mapping for MW in your MiModule:
//   side = side, wall = column, column = row
// ============================================================
static bool om_center_from_gid(MiGID* gid, double& xc, double& yc, double& zc, int& om_type)
{
    if (!gid) return false;

    const int type   = to_int_safe(gid->gettype());
    const int side   = to_int_safe(gid->getside());
    const int wall   = to_int_safe(gid->getwall());
    const int column = to_int_safe(gid->getcolumn());
    const int row    = to_int_safe(gid->getrow());

    xc = yc = zc = std::numeric_limits<double>::quiet_NaN();
    om_type = -1;

    if (type == 1302) {
        if (side < 0 || wall < 0 || column < 0) return false;

        const int mw_col = wall;
        const int mw_row = column;

        om_type = 1302;
        xc = (side == 1) ?  532.0 : -532.0;
        yc = (static_cast<double>(mw_col) - 9.5) * 259.0;
        zc = (static_cast<double>(mw_row) - 6.0) * 259.0;
        return finite3(xc, yc, zc);
    }

    if (type == 1232) {
        if (side < 0 || wall < 0 || column < 0 || row < 0) return false;

        om_type = 1232;
        yc = (wall == 1) ? 2580.5 : -2580.5;

        if (side == 1) xc = (column == 1) ? 333.0 : 130.0;
        else           xc = (column == 1) ? -333.0 : -130.0;

        zc = (static_cast<double>(row) - 7.5) * 212.0;
        return finite3(xc, yc, zc);
    }

    if (type == 1252) {
        if (side < 0 || wall < 0 || column < 0) return false;

        om_type = 1252;
        xc = (side == 1) ?  213.5 : -213.5;
        zc = (wall == 1) ? 1625.0 : -1625.0;

        if (column > 7) yc = 161.0 + (static_cast<double>(column) - 8.0) * 311.5;
        else            yc = -161.0 + (static_cast<double>(column) - 7.0) * 311.5;

        return finite3(xc, yc, zc);
    }

    return false;
}

// ============================================================
// Window center from GID
// ============================================================
static bool om_window_center_from_gid(MiGID* gid, double& xw, double& yw, double& zw, int& face_mode)
{
    double xc = 0.0, yc = 0.0, zc = 0.0;
    int om_type = -1;
    if (!om_center_from_gid(gid, xc, yc, zc, om_type)) return false;

    const int side = to_int_safe(gid->getside());
    const int wall = to_int_safe(gid->getwall());

    static const double mw_sizex = 194.0;
    static const double xw_sizey = 150.0;
    static const double gv_sizez = 150.0;

    xw = xc;
    yw = yc;
    zw = zc;
    face_mode = -1;

    if (om_type == 1302) {
        face_mode = 0;
        xw = (side == 1) ? (xc - mw_sizex / 2.0) : (xc + mw_sizex / 2.0);
        return true;
    }

    if (om_type == 1232) {
        face_mode = 1;
        yw = (wall == 1) ? (yc - xw_sizey / 2.0) : (yc + xw_sizey / 2.0);
        return true;
    }

    if (om_type == 1252) {
        face_mode = 2;
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
// Compute impact angle for a given particle and matched CD hit
// ============================================================
static bool particle_compute_impact_angle_deg(MiEvent* Eve,
                                              int ip,
                                              MiCDCaloHit* cdHit,
                                              double& theta_deg)
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

    if (face_mode == 0)      cos_theta = std::fabs(vx) / vnorm;
    else if (face_mode == 1) cos_theta = std::fabs(vy) / vnorm;
    else if (face_mode == 2) cos_theta = std::fabs(vz) / vnorm;
    else return false;

    cos_theta = clamp1(cos_theta);
    theta_deg = std::acos(cos_theta) * 180.0 / M_PI;

    return std::isfinite(theta_deg);
}

// ============================================================
// Angle cut
// ============================================================
static bool particle_passes_angle_cut(MiEvent* Eve,
                                      int ip,
                                      MiCDCaloHit* cdHit,
                                      double angle_min_deg,
                                      double angle_max_deg,
                                      double angle_tol_deg,
                                      double& theta_deg)
{
    theta_deg = -1.0;

    if (!particle_compute_impact_angle_deg(Eve, ip, cdHit, theta_deg)) return false;

    if (angle_max_deg < 0.0) {
        return std::fabs(theta_deg - angle_min_deg) <= angle_tol_deg;
    }

    const bool include_upper_90 = (std::fabs(angle_max_deg - 90.0) < 1e-12);
    if (include_upper_90) {
        return (theta_deg >= angle_min_deg && theta_deg <= angle_max_deg);
    }

    return (theta_deg >= angle_min_deg && theta_deg < angle_max_deg);
}

// ============================================================
// Match PTD hit -> CD hit
// ============================================================
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
void reading_root_OM_SPECTRA_angle()
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

    if (ANGLE_MAX_DEG < 0.0) {
        std::cout << "Angle cut mode: single-angle\n";
        std::cout << "Condition: |theta - " << ANGLE_MIN_DEG
                  << "| <= " << ANGLE_TOL_DEG << " deg\n";
    } else {
        std::cout << "Angle cut mode: range\n";
        std::cout << "Condition: " << ANGLE_MIN_DEG << " <= theta";
        if (std::fabs(ANGLE_MAX_DEG - 90.0) < 1e-12) std::cout << " <= ";
        else                                         std::cout << " < ";
        std::cout << ANGLE_MAX_DEG << " deg\n";
    }

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
    long long n_bad_gid            = 0;
    long long n_pass_angle         = 0;
    long long n_fail_angle         = 0;
    long long n_bad_angle          = 0;
    long long n_used_hits          = 0;

    for (Long64_t i = 0; i < N; ++i)
    {
        s->GetEntry(i);
        n_events_processed++;

        MiPTD* ptd = Eve->getPTD();
        if (!ptd) {
            if ((i + 1) % 100000 == 0) {
                std::cout << "Processed " << (i + 1) << " / " << N
                          << "  selected=" << n_selected_particles
                          << "  matched=" << n_matched_hits
                          << "  passA=" << n_pass_angle
                          << "  failA=" << n_fail_angle
                          << "  badA=" << n_bad_angle
                          << "  usedHits=" << n_used_hits
                          << "  nOM=" << spectraPerOM.size() << "\n";
            }
            continue;
        }
        n_events_with_ptd++;

        MiCD* cd = Eve->getCD();
        if (!cd) {
            if ((i + 1) % 100000 == 0) {
                std::cout << "Processed " << (i + 1) << " / " << N
                          << "  selected=" << n_selected_particles
                          << "  matched=" << n_matched_hits
                          << "  passA=" << n_pass_angle
                          << "  failA=" << n_fail_angle
                          << "  badA=" << n_bad_angle
                          << "  usedHits=" << n_used_hits
                          << "  nOM=" << spectraPerOM.size() << "\n";
            }
            continue;
        }
        n_events_with_cd++;

        std::vector<MiCDParticle>* partv = ptd->getpartv();
        if (!partv) {
            if ((i + 1) % 100000 == 0) {
                std::cout << "Processed " << (i + 1) << " / " << N
                          << "  selected=" << n_selected_particles
                          << "  matched=" << n_matched_hits
                          << "  passA=" << n_pass_angle
                          << "  failA=" << n_fail_angle
                          << "  badA=" << n_bad_angle
                          << "  usedHits=" << n_used_hits
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

            // PTD selection
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

            double theta_deg = -1.0;
            const bool pass_angle = particle_passes_angle_cut(
                Eve, ip, cdHit,
                ANGLE_MIN_DEG,
                ANGLE_MAX_DEG,
                ANGLE_TOL_DEG,
                theta_deg
            );

            if (!pass_angle) {
                if (!std::isfinite(theta_deg) || theta_deg < 0.0) n_bad_angle++;
                n_fail_angle++;
                continue;
            }

            n_pass_angle++;

            const std::string omKey = make_om_key(gid);
            omnumToKey[omnum] = omKey;

            const long long binIdx = (long long)std::floor(E_keV / BIN_KEV);
            spectraPerOM[omnum][binIdx]++;
            n_used_hits++;
        }

        if ((i + 1) % 100000 == 0) {
            std::cout << "Processed " << (i + 1) << " / " << N
                      << "  selected=" << n_selected_particles
                      << "  matched=" << n_matched_hits
                      << "  passA=" << n_pass_angle
                      << "  failA=" << n_fail_angle
                      << "  badA=" << n_bad_angle
                      << "  usedHits=" << n_used_hits
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
    std::cout << "Passed angle cut: " << n_pass_angle << "\n";
    std::cout << "Failed angle cut: " << n_fail_angle << "\n";
    std::cout << "Bad/unavailable angle: " << n_bad_angle << "\n";
    std::cout << "Hits written to spectra: " << n_used_hits << "\n";
    std::cout << "OM spectra built: " << spectraPerOM.size() << "\n";

    std::ostringstream cutTag;
    cutTag << "angleCutOnly-";
    if (ANGLE_MAX_DEG < 0.0) {
        cutTag << "angle" << format_double_1(ANGLE_MIN_DEG)
               << "pm"    << format_double_1(ANGLE_TOL_DEG)
               << "deg";
    } else {
        cutTag << "angle" << format_double_1(ANGLE_MIN_DEG)
               << "to"    << format_double_1(ANGLE_MAX_DEG)
               << "deg";
    }

    // mapping file
    {
        std::ofstream mapout((OUTPUT_DIR + "OM_index_map_" + cutTag.str() + ".txt").c_str());
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
              << "-"
              << cutTag.str()
              << ".txt";

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
