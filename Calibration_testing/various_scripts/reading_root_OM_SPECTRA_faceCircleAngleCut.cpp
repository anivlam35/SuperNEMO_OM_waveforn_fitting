// reading_root_OM_SPECTRA_faceCircleAngleCut.cpp
//
// Run exactly like this:
//   root reading_root_OM_SPECTRA_faceCircleAngleCut.cpp
//
// To change cuts, edit the constants below:
//   INPUT_ROOT
//   OUTPUT_DIR
//   RMAX_MM
//   ANGLE_MIN_DEG
//   ANGLE_MAX_DEG
//   ANGLE_TOL_DEG
//   BIN_KEV
//
// Convention:
//   if ANGLE_MAX_DEG < 0  -> single-angle mode
//      keep |theta - ANGLE_MIN_DEG| <= ANGLE_TOL_DEG
//
//   if ANGLE_MAX_DEG >= 0 -> range mode
//      keep ANGLE_MIN_DEG <= theta < ANGLE_MAX_DEG
//      except if ANGLE_MAX_DEG == 90 -> keep ANGLE_MIN_DEG <= theta <= 90
//
// Angle definition:
//   0 deg   = perpendicular to OM face
//   90 deg  = parallel to OM face

#include "/sps/nemo/scratch/ykozina/Falaise/tutorial/MiModule/include/MiEvent.h"

#include <TFile.h>
#include <TTree.h>
#include <TH1D.h>
#include <TString.h>

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

// ------------------------------------------------------------
// USER SETTINGS: edit only these if needed
// ------------------------------------------------------------
const std::string INPUT_ROOT =
  "/sps/nemo/scratch/ykozina/Falaise/calibration/calibration-CORRECTIONS/BOTH/calibrated_brio/SNCUTS/ROOT/merged_week_2025_X-BOTH.root";

const std::string OUTPUT_DIR =
  "/sps/nemo/scratch/ykozina/Falaise/calibration/calibration-CORRECTIONS/BOTH/calibrated_brio/SNCUTS/ROOT/txt_angleCut/60-90_within_circle";

const double RMAX_MM       = 50.0;
const double ANGLE_MIN_DEG = 60;
const double ANGLE_MAX_DEG = 90;   // <0 => single-angle mode; >=0 => range mode
const double ANGLE_TOL_DEG = 0.5;    // used only in single-angle mode
const double BIN_KEV       = 5.0;

// ------------------------------------------------------------
// OM dimensions (mm)
// ------------------------------------------------------------
static const double mw_sizex = 194.0;
static const double xw_sizey = 150.0;
static const double gv_sizez = 150.0;

// ------------------------------------------------------------
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

// sanitize for ROOT object names if needed
static std::string make_root_safe_name(const std::string& s)
{
  std::string out = s;
  for (char& c : out) {
    if (!(std::isalnum((unsigned char)c) || c == '_')) c = '_';
  }
  return out;
}

// ------------------------------------------------------------
// Compute OM center (volume center) from MiGID
// IMPORTANT mapping for MW in your MiModule:
//   side = side, wall = column, column = row
// ------------------------------------------------------------
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

// ------------------------------------------------------------
// Center of INNER FACE (entrance window)
// face_mode: 0 MW -> face plane YZ, normal along X
//            1 XW -> face plane XZ, normal along Y
//            2 GV -> face plane XY, normal along Z
// ------------------------------------------------------------
static bool om_window_center_from_gid(MiGID* gid, double& xw, double& yw, double& zw, int& face_mode)
{
  double xc = 0.0, yc = 0.0, zc = 0.0;
  int om_type = -1;
  if (!om_center_from_gid(gid, xc, yc, zc, om_type)) return false;

  const int side = to_int_safe(gid->getside());
  const int wall = to_int_safe(gid->getwall());

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

// ------------------------------------------------------------
// particle #0 : reference source plane vertex
// ------------------------------------------------------------
static bool get_particle0_reference_source_plane_vertex(MiEvent* Eve,
                                                        double& xs, double& ys, double& zs)
{
  if (!Eve) return false;
  if (Eve->getPTDNoPart() <= 0) return false;

  const int ip = 0;
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

// ------------------------------------------------------------
// particle #0 : first calo/xcalo/gveto vertex
// ------------------------------------------------------------
static bool get_particle0_calo_vertex(MiEvent* Eve,
                                      double& xv, double& yv, double& zv)
{
  if (!Eve) return false;
  if (Eve->getPTDNoPart() <= 0) return false;

  const int ip = 0;
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

// ------------------------------------------------------------
// Event passes if PTD calo vertex lies inside circle of radius Rmax
// on the inner face of the OM associated to CD hit #0
// ------------------------------------------------------------
static bool event_passes_face_circle(MiEvent* Eve, double Rmax_mm, double& R_mm, double& normal_mm)
{
  R_mm = -1.0;
  normal_mm = -1.0;

  if (!Eve) return false;

  MiCD* cd = Eve->getCD();
  if (!cd) return false;
  if (cd->getnoofcaloh() <= 0) return false;

  MiCDCaloHit* hit = cd->getcalohit(0);
  if (!hit) return false;

  MiGID* gid = hit->getGID();
  if (!gid) return false;

  double xw = 0.0, yw = 0.0, zw = 0.0;
  int face_mode = -1;
  if (!om_window_center_from_gid(gid, xw, yw, zw, face_mode)) return false;

  double xv = 0.0, yv = 0.0, zv = 0.0;
  if (!get_particle0_calo_vertex(Eve, xv, yv, zv)) return false;

  if (face_mode == 0) {
    R_mm = std::sqrt((yv - yw) * (yv - yw) + (zv - zw) * (zv - zw));
    normal_mm = std::fabs(xv - xw);
  } else if (face_mode == 1) {
    R_mm = std::sqrt((xv - xw) * (xv - xw) + (zv - zw) * (zv - zw));
    normal_mm = std::fabs(yv - yw);
  } else if (face_mode == 2) {
    R_mm = std::sqrt((xv - xw) * (xv - xw) + (yv - yw) * (yv - yw));
    normal_mm = std::fabs(zv - zw);
  } else {
    return false;
  }

  return std::isfinite(R_mm) && (R_mm <= Rmax_mm);
}

// ------------------------------------------------------------
// Compute impact angle in degrees
// ------------------------------------------------------------
static bool event_compute_impact_angle_deg(MiEvent* Eve, double& theta_deg)
{
  theta_deg = -1.0;

  if (!Eve) return false;

  MiCD* cd = Eve->getCD();
  if (!cd) return false;
  if (cd->getnoofcaloh() <= 0) return false;

  MiCDCaloHit* hit = cd->getcalohit(0);
  if (!hit) return false;

  MiGID* gid = hit->getGID();
  if (!gid) return false;

  int face_mode = -1;
  double xw = 0.0, yw = 0.0, zw = 0.0;
  if (!om_window_center_from_gid(gid, xw, yw, zw, face_mode)) return false;

  double xs = 0.0, ys = 0.0, zs = 0.0;
  if (!get_particle0_reference_source_plane_vertex(Eve, xs, ys, zs)) return false;

  double xv = 0.0, yv = 0.0, zv = 0.0;
  if (!get_particle0_calo_vertex(Eve, xv, yv, zv)) return false;

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

// ------------------------------------------------------------
// Angle cut
// ------------------------------------------------------------
static bool event_passes_angle_cut(MiEvent* Eve,
                                   double angle_min_deg,
                                   double angle_max_deg,
                                   double angle_tol_deg,
                                   double& theta_deg)
{
  theta_deg = -1.0;

  if (!event_compute_impact_angle_deg(Eve, theta_deg)) return false;

  if (angle_max_deg < 0.0) {
    return std::fabs(theta_deg - angle_min_deg) <= angle_tol_deg;
  }

  const bool include_upper_90 = (std::fabs(angle_max_deg - 90.0) < 1e-12);
  if (include_upper_90) {
    return (theta_deg >= angle_min_deg && theta_deg <= angle_max_deg);
  }

  return (theta_deg >= angle_min_deg && theta_deg < angle_max_deg);
}

// ------------------------------------------------------------
// OM key
// ------------------------------------------------------------
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

// ------------------------------------------------------------
// Worker with explicit arguments
// ------------------------------------------------------------
static void run_faceCircleAngleCut_impl(const char* input_root,
                                        double Rmax_mm,
                                        double angle_min_deg,
                                        double angle_max_deg,
                                        double angle_tol_deg,
                                        double BIN_KEV)
{
  std::cout << "Input: " << input_root << "\n";

  TFile* f = TFile::Open(input_root, "READ");
  if (!f || f->IsZombie()) {
    std::cerr << "ERROR: cannot open input file: " << input_root << "\n";
    if (f) { f->Close(); delete f; }
    return;
  }

  TTree* s = (TTree*) f->Get("Event");
  if (!s) {
    std::cerr << "ERROR: Tree 'Event' not found\n";
    f->Close();
    delete f;
    return;
  }

  const Long64_t N = s->GetEntries();

  std::map< std::string, std::map<long long,long long> > spectraPerOM;

  MiEvent* Eve = new MiEvent();
  s->SetBranchAddress("Eventdata", &Eve);

  long long n_pass_total   = 0;
  long long n_fail_radius  = 0;
  long long n_fail_angle   = 0;
  long long n_bad_angle    = 0;

  std::cout << "Entries: " << N << "\n";
  std::cout << "Radius cut: R <= " << Rmax_mm << " mm\n";

  if (angle_max_deg < 0.0) {
    std::cout << "Angle cut mode: single-angle\n";
    std::cout << "Condition: |theta - " << angle_min_deg
              << "| <= " << angle_tol_deg << " deg\n";
  } else {
    std::cout << "Angle cut mode: range\n";
    std::cout << "Condition: " << angle_min_deg << " <= theta";
    if (std::fabs(angle_max_deg - 90.0) < 1e-12) std::cout << " <= ";
    else                                          std::cout << " < ";
    std::cout << angle_max_deg << " deg\n";
  }

  std::cout << "Bin width: " << BIN_KEV << " keV\n";
  std::cout << "Output dir: " << OUTPUT_DIR << "\n";

  for (Long64_t i = 0; i < N; ++i) {
    s->GetEntry(i);

    double R_mm = -1.0;
    double normal_mm = -1.0;
    const bool pass_radius = event_passes_face_circle(Eve, Rmax_mm, R_mm, normal_mm);
    if (!pass_radius) {
      n_fail_radius++;
      continue;
    }

    double theta_deg = -1.0;
    const bool pass_angle = event_passes_angle_cut(Eve,
                                                   angle_min_deg,
                                                   angle_max_deg,
                                                   angle_tol_deg,
                                                   theta_deg);
    if (!pass_angle) {
      if (!std::isfinite(theta_deg) || theta_deg < 0.0) n_bad_angle++;
      n_fail_angle++;
      continue;
    }

    n_pass_total++;

    MiCD* cd = Eve->getCD();
    if (!cd) continue;

    const int ncalo = cd->getnoofcaloh();
    for (int ih = 0; ih < ncalo; ++ih) {
      MiCDCaloHit* hitp = cd->getcalohit(ih);
      if (!hitp) continue;

      const double E_keV = hitp->getE();
      if (!std::isfinite(E_keV)) continue;

      MiGID* gid = hitp->getGID();
      if (!gid) continue;

      const std::string omKey = make_om_key(gid);
      const long long binIdx = (long long) std::floor(E_keV / BIN_KEV);

      spectraPerOM[omKey][binIdx]++;
    }

    if ((i + 1) % 100000 == 0) {
      std::cout << "Processed " << (i + 1) << " / " << N
                << "  pass=" << n_pass_total
                << "  failR=" << n_fail_radius
                << "  failA=" << n_fail_angle
                << "  badAngle=" << n_bad_angle
                << "  nOM=" << spectraPerOM.size() << "\n";
    }
  }

  std::ostringstream cutTag;
  cutTag << "faceCircle" << (int)std::round(Rmax_mm) << "mm-";

  if (angle_max_deg < 0.0) {
    cutTag << "angle" << format_double_1(angle_min_deg)
           << "pm"    << format_double_1(angle_tol_deg)
           << "deg";
  } else {
    cutTag << "angle" << format_double_1(angle_min_deg)
           << "to"    << format_double_1(angle_max_deg)
           << "deg";
  }

  for (const auto& omEntry : spectraPerOM) {
    const std::string& omKey     = omEntry.first;
    const auto&        binCounts = omEntry.second;

    std::ostringstream fname;
    fname << OUTPUT_DIR
          << omKey
          << "-"
          << cutTag.str()
          << ".txt";

    std::ofstream out(fname.str());
    if (!out) {
      std::cerr << "Cannot open output file " << fname.str() << "\n";
      continue;
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

    std::cout << omKey
              << ": " << binCounts.size()
              << " bins written to " << fname.str()
              << " (bin=" << BIN_KEV << " keV)\n";
  }

  {
    std::ostringstream rootname;
    rootname << OUTPUT_DIR << "OM_spectra_" << cutTag.str() << ".root";

    TFile* fout = TFile::Open(rootname.str().c_str(), "RECREATE");
    if (!fout || fout->IsZombie()) {
      std::cerr << "Cannot create ROOT output file " << rootname.str() << "\n";
    } else {
      TH1D* hOMTotalCounts = new TH1D("hOMTotalCounts",
                                      "Total counts per OM;OM index;Counts",
                                      std::max(1, (int)spectraPerOM.size()),
                                      0.5, spectraPerOM.size() + 0.5);

      int iom = 0;
      for (const auto& omEntry : spectraPerOM) {
        ++iom;
        const std::string& omKey     = omEntry.first;
        const auto&        binCounts = omEntry.second;

        long long maxBinIdx = -1;
        long long totalCounts = 0;
        for (const auto& kv : binCounts) {
          if (kv.first > maxBinIdx) maxBinIdx = kv.first;
          totalCounts += kv.second;
        }

        const int nBinsHist = std::max(1LL, maxBinIdx + 1);
        const double xmax = nBinsHist * BIN_KEV;

        const std::string hkey = make_root_safe_name("h_" + omKey);

        std::ostringstream htitle;
        htitle << omKey << ";Energy [keV];Counts";

        TH1D* h = new TH1D(hkey.c_str(),
                           htitle.str().c_str(),
                           nBinsHist, 0.0, xmax);

        for (const auto& kv : binCounts) {
          const long long binIdx = kv.first;
          const long long count  = kv.second;
          const int rootBin = static_cast<int>(binIdx + 1);
          if (rootBin >= 1 && rootBin <= h->GetNbinsX()) {
            h->SetBinContent(rootBin, static_cast<double>(count));
          }
        }

        h->Write();
        hOMTotalCounts->SetBinContent(iom, static_cast<double>(totalCounts));
      }

      hOMTotalCounts->Write();
      fout->Close();

      std::cout << "ROOT browser file written to " << rootname.str() << "\n";
    }

    delete fout;
  }

  std::cout << "Done.\n";
  std::cout << "Passed events: " << n_pass_total << "\n";
  std::cout << "Failed radius cut: " << n_fail_radius << "\n";
  std::cout << "Failed angle cut: " << n_fail_angle << "\n";
  std::cout << "Bad/unavailable angle events: " << n_bad_angle << "\n";
  std::cout << "OM files written: " << spectraPerOM.size() << "\n";

  f->Close();
  delete f;
  delete Eve;
}

// ------------------------------------------------------------
// ROOT named macro entry point: NO ARGUMENTS
// so you can run simply:
//   root reading_root_OM_SPECTRA_faceCircleAngleCut.cpp
// ------------------------------------------------------------
void reading_root_OM_SPECTRA_faceCircleAngleCut()
{
  run_faceCircleAngleCut_impl(INPUT_ROOT.c_str(),
                              RMAX_MM,
                              ANGLE_MIN_DEG,
                              ANGLE_MAX_DEG,
                              ANGLE_TOL_DEG,
                              BIN_KEV);
}
