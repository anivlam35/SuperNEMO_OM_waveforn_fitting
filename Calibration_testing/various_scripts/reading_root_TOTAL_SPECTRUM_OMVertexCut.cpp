#include "/sps/nemo/scratch/ykozina/Falaise/tutorial/MiModule/include/MiEvent.h"

#include <TFile.h>
#include <TTree.h>
#include <TH1D.h>
#include <TSystem.h>

#include <iostream>
#include <cmath>
#include <string>
#include <limits>
#include <cstdlib>

R__LOAD_LIBRARY(/sps/nemo/scratch/ykozina/Falaise/tutorial/MiModule/lib/libMiModule.so);

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

// ------------------------------------------------------------
// Compute OM center (volume center) from MiGID
//
// IMPORTANT: For main wall in your MiModule:
//   side   = side
//   wall   = column
//   column = row
//   row    = sentinel
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

  // ---------------- main wall ----------------
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

  // ---------------- xcalo ----------------
  if (type == 1232) {
    if (side < 0 || wall < 0 || column < 0 || row < 0) return false;

    om_type = 1232;
    yc = (wall == 1) ? 2580.5 : -2580.5;

    if (side == 1) xc = (column == 1) ? 333.0 : 130.0;
    else           xc = (column == 1) ? -333.0 : -130.0;

    zc = (static_cast<double>(row) - 7.5) * 212.0;
    return finite3(xc, yc, zc);
  }

  // ---------------- gveto ----------------
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
// face_mode: 0 MW -> circle in YZ
//            1 XW -> circle in XZ
//            2 GV -> circle in XY
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
// 1 track / 1 electron assumed:
// take PTD particle #0 and its first calo/xcalo/gveto vertex
// ------------------------------------------------------------
static bool get_particle0_calo_vertex(MiEvent* Eve, double& xv, double& yv, double& zv)
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

  if (face_mode == 0) {        // MW: YZ plane
    R_mm = std::sqrt((yv - yw) * (yv - yw) + (zv - zw) * (zv - zw));
    normal_mm = std::fabs(xv - xw);
  } else if (face_mode == 1) { // XW: XZ plane
    R_mm = std::sqrt((xv - xw) * (xv - xw) + (zv - zw) * (zv - zw));
    normal_mm = std::fabs(yv - yw);
  } else if (face_mode == 2) { // GV: XY plane
    R_mm = std::sqrt((xv - xw) * (xv - xw) + (yv - yw) * (yv - yw));
    normal_mm = std::fabs(zv - zw);
  } else {
    return false;
  }

  return std::isfinite(R_mm) && (R_mm <= Rmax_mm);
}

// ------------------------------------------------------------
// Main: same total-spectrum logic as before,
// but only for events passing the face-circle cut
// ------------------------------------------------------------
void reading_root_TOTAL_SPECTRUM_OMVertexCut(
    const char* input_root  = "merged_week_2025_X-BOTH.root",
    const char* output_root = "TOTAL_energy_spectrum-faceCircle5cm-BOTH.root",
    double Rmax_mm = 50.0)
{
  TFile* f = TFile::Open(input_root, "READ");
  if (!f || f->IsZombie()) {
    std::cerr << "ERROR: cannot open input file: " << input_root << "\n";
    if (f) { f->Close(); delete f; }
    return;
  }

  TTree* s = dynamic_cast<TTree*>(f->Get("Event"));
  if (!s) {
    std::cerr << "ERROR: Tree 'Event' not found in file\n";
    f->Close();
    delete f;
    return;
  }

  MiEvent* Eve = new MiEvent();
  s->SetBranchAddress("Eventdata", &Eve);

  const Long64_t N = s->GetEntries();

  TH1D* h_energy_sum = new TH1D(
      "h_energy_sum",
      "Total energy spectrum (CD hits), events passing face-circle cut;Energy [keV];Counts",
      1000, 0.0, 5000.0);
  h_energy_sum->SetDirectory(nullptr);

  TH1D* h_R = new TH1D("h_R", "R on OM inner face;R [mm];Counts", 400, 0.0, 400.0);
  TH1D* h_normal = new TH1D("h_normal", "Residual along face normal;|dn| [mm];Counts", 400, 0.0, 200.0);
  h_R->SetDirectory(nullptr);
  h_normal->SetDirectory(nullptr);

  long long n_pass = 0;
  long long n_fail = 0;

  std::cout << "Reading tree 'Event' from: " << input_root << "\n";
  std::cout << "Total entries: " << N << "\n";
  std::cout << "Cut: R <= " << Rmax_mm << " mm on OM inner face\n";

  for (Long64_t i = 0; i < N; ++i) {
    s->GetEntry(i);

    double R_mm = -1.0;
    double normal_mm = -1.0;
    bool pass = event_passes_face_circle(Eve, Rmax_mm, R_mm, normal_mm);

    if (R_mm >= 0.0) h_R->Fill(R_mm);
    if (normal_mm >= 0.0) h_normal->Fill(normal_mm);

    if (!pass) {
      n_fail++;
      continue;
    }

    n_pass++;

    MiCD* cd = Eve->getCD();
    if (!cd) continue;

    const int ncalo = cd->getnoofcaloh();
    for (int ih = 0; ih < ncalo; ++ih) {
      MiCDCaloHit* hitp = cd->getcalohit(ih);
      if (!hitp) continue;

      const double E_keV = hitp->getE();
      if (!std::isfinite(E_keV)) continue;

      h_energy_sum->Fill(E_keV);
    }

    if ((i + 1) % 100000 == 0) {
      std::cout << "Processed " << (i + 1) << " / " << N
                << "  pass=" << n_pass
                << "  fail=" << n_fail << "\n";
    }
  }

  TFile* fout = TFile::Open(output_root, "RECREATE");
  if (!fout || fout->IsZombie()) {
    std::cerr << "ERROR: cannot create output ROOT file: " << output_root << "\n";
    f->Close();
    delete f;
    delete Eve;
    delete h_energy_sum;
    delete h_R;
    delete h_normal;
    return;
  }

  h_energy_sum->Write();
  h_R->Write();
  h_normal->Write();

  fout->Close();
  f->Close();

  std::cout << "Done. Output: " << output_root << "\n";
  std::cout << "Passed events: " << n_pass << "\n";
  std::cout << "Failed events: " << n_fail << "\n";
  std::cout << "Histogram entries: " << h_energy_sum->GetEntries() << "\n";

  delete fout;
  delete f;
  delete Eve;
  delete h_energy_sum;
  delete h_R;
  delete h_normal;
}
