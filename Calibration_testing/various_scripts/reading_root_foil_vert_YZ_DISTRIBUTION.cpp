#include "/sps/nemo/scratch/ykozina/Falaise/tutorial/MiModule/include/MiEvent.h"

#include <iostream>
#include <cmath>
#include <string>

#include "TFile.h"
#include "TTree.h"
#include "TH2D.h"
#include "TH1D.h"
#include "TCanvas.h"
#include "TStyle.h"
#include "TPaletteAxis.h"

R__LOAD_LIBRARY(/sps/nemo/scratch/ykozina/Falaise/tutorial/MiModule/lib/libMiModule.so);

void reading_root_foil_vert_YZ_DISTRIBUTION()
{
  // -------- Style (presentation-ready) --------
  gStyle->SetOptStat(0);              
  gStyle->SetNumberContours(255);     
  gStyle->SetImageScaling(3.0);       
  gStyle->SetTitleFont(42, "XYZ");
  gStyle->SetLabelFont(42, "XYZ");
  gStyle->SetTitleSize(0.045, "XYZ");
  gStyle->SetLabelSize(0.038, "XYZ");
  gStyle->SetTitleOffset(1.15, "X");
  gStyle->SetTitleOffset(1.20, "Y");
  gStyle->SetTitleOffset(1.10, "Z");

  TFile* f = new TFile("merged_week_2025_X_BOTH_NEWSNC.root","READ");
  if (!f || f->IsZombie()) { std::cerr << "Cannot open Default.root\n"; return; }

  TTree* s = (TTree*) f->Get("Event");
  if (!s) { std::cerr << "Tree 'Event' not found\n"; return; }

  MiEvent* Eve = new MiEvent();
  s->SetBranchAddress("Eventdata", &Eve);

  // 2D
  TH2D* hYZ = new TH2D("hFoilYZ",
                       "Foil vertices;Y [mm];Z [mm]",
                       4000, -2500, 2500,
                       4000, -2000, 2000);

  TH1D* hY  = new TH1D("hFoilY",
                       "Foil vertex Y;Y [mm];Entries",
                       400, -2500, 2500);

  TH1D* hZ  = new TH1D("hFoilZ",
                       "Foil vertex Z;Z [mm];Entries",
                       400, -2500, 2500);

  long long totVert = 0, totFoil = 0;

  Long64_t N = s->GetEntries();
  for (Long64_t i = 0; i < N; ++i) {
    s->GetEntry(i);
    // --- ??????? ---
    if (i % 100000 == 0)
      std::cout << "Processing event " << i << " / " << N
                << "  foil_verts_so_far=" << totFoil << std::endl;

    int nPart = Eve->getPTDNoPart();
    for (int ip = 0; ip < nPart; ++ip) {

      int nVert = Eve->getPTDNoVert(ip);
      for (int iv = 0; iv < nVert; ++iv) {
        totVert++;

        std::string where = Eve->getPTDVertpos(ip, iv);

        // debug
        if (totVert < 20) {
          std::cout << "Example vertex label: '" << where << "'\n";
        }

        if (where == "reference source plane") {
          double y = Eve->getPTDverY(ip, iv);
          double z = Eve->getPTDverZ(ip, iv);
          if (!std::isfinite(y) || !std::isfinite(z)) continue;

          hYZ->Fill(y, z);
          hY->Fill(y);
          hZ->Fill(z);

          totFoil++;
        }
      }
    }
  }

  TFile* out = new TFile("FoilYZ-calibsource_NEWpos.root", "RECREATE");
  hYZ->Write();
  hY->Write();
  hZ->Write();
  out->Close();

  // -------- Canvas + pretty colorbar + HiRes PNG --------
  TCanvas* c2 = new TCanvas("cFoilYZ","Foil YZ",1800,1400);
  c2->SetLeftMargin(0.12);
  c2->SetBottomMargin(0.12);
  c2->SetTopMargin(0.08);
  c2->SetRightMargin(0.10); 

  hYZ->SetMinimum(1);
  hYZ->GetXaxis()->CenterTitle(true);
  hYZ->GetYaxis()->CenterTitle(true);
  hYZ->Draw("COLZ");



  c2->Update();


  if (auto pal = (TPaletteAxis*)hYZ->GetListOfFunctions()->FindObject("palette")) {
    pal->SetX1NDC(0.915);  
    pal->SetX2NDC(0.945);  
    pal->SetY1NDC(0.15);
    pal->SetY2NDC(0.90);
    pal->SetLabelSize(0.032);
  }

  c2->Modified();
  c2->Update();


  c2->SaveAs("FoilYZ-calibsource_NEWpos.png");  


  TCanvas* cY = new TCanvas("cFoilY","Foil Y",1400,900);
  cY->SetLeftMargin(0.12); cY->SetBottomMargin(0.12);
  hY->Draw("HIST");

  TCanvas* cZ = new TCanvas("cFoilZ","Foil Z",1400,900);
  cZ->SetLeftMargin(0.12); cZ->SetBottomMargin(0.12);
  hZ->Draw("HIST");

  std::cout << "Filled foil vertices: " << totFoil
            << " (hist entries: " << hYZ->GetEntries() << ")\n";

  f->Close();
}

