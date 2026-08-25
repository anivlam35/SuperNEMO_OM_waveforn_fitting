// overlay_with_without_carrier.C
//
// Usage:
//   root -l -b -q 'overlay_with_without_carrier.C()'

#include "/sps/nemo/scratch/ykozina/Falaise/tutorial/MiModule/include/MiEvent.h"

#include <TCanvas.h>
#include <TH1D.h>
#include <TFile.h>
#include <TTree.h>
#include <TLegend.h>
#include <TStyle.h>

#include <iostream>
#include <string>
#include <vector>
#include <cmath>

R__LOAD_LIBRARY(/sps/nemo/scratch/ykozina/Falaise/tutorial/MiModule/lib/libMiModule.so);

void overlay_with_without_carrier()
{
    // ============================================================
    const std::string INPUT_ROOT =
        "/sps/nemo/scratch/ykozina/Falaise/calibration/calibration-CORRECTIONS/"
        "SIMU/brios/SNCUTS/NEW_s_positions/ROOT/merged_simu.root";

    const std::string OUTPUT_PNG  = "overlay_with_without_carrier.png";
    const std::string OUTPUT_ROOT = "overlay_with_without_carrier.root";

    const double BIN_KEV        = 10.0;
    const double X_MIN          = 0.0;
    const double X_MAX          = 1400.0;

    // energy cut ? ?????????????, ????? ?????????: E_carrier > 0
    // const double LOSS_THRESHOLD = 35.0;   // keV

    // ---- DRAW STYLE: uncomment ONE option ----
    // Option 1: step histogram (currently active)
    const char* DRAW_STYLE = "HIST";
    // Option 2: error bars / crosses ? comment line above and uncomment below
    // const char* DRAW_STYLE = "E";
    // ------------------------------------------

    const Long64_t MAX_EVENTS = 0;
    // ============================================================

    TFile* f = TFile::Open(INPUT_ROOT.c_str(), "READ");
    if(!f||f->IsZombie()){ std::cerr<<"ERROR: cannot open input\n"; return; }
    TTree* tree = dynamic_cast<TTree*>(f->Get("Event"));
    if(!tree){ std::cerr<<"ERROR: no Event tree\n"; f->Close(); return; }

    const Long64_t N = (MAX_EVENTS>0)
                     ? std::min(MAX_EVENTS,tree->GetEntries())
                     : tree->GetEntries();
    std::cout<<"Entries: "<<N<<"\n";

    MiEvent* Eve = new MiEvent();
    tree->SetBranchAddress("Eventdata", &Eve);

    const int nbins = (int)((X_MAX-X_MIN)/BIN_KEV+0.5);

    TH1D* hAll  = new TH1D("hAll",
        " ;Energy [keV];Counts",
        nbins, X_MIN, X_MAX);
    TH1D* hLoss = new TH1D("hLoss","",nbins,X_MIN,X_MAX);
    hAll->Sumw2(); hLoss->Sumw2();

    Long64_t n_all=0, n_loss=0;

    for(Long64_t i=0;i<N;++i){
        tree->GetEntry(i);
        if((i+1)%100000==0) std::cout<<"  "<<(i+1)<<"/"<<N<<"\n";

        MiPTD* ptd = Eve->getPTD();
        if(!ptd) continue;
        std::vector<MiCDParticle>* partv = ptd->getpartv();
        if(!partv||partv->empty()) continue;

        double E_OM = -1.0;
        for(int ip=0;ip<(int)partv->size();++ip){
            MiCDParticle* part = ptd->getpart(ip);
            if(!part||part->getcharge()!=1000) continue;
            std::vector<MiCDCaloHit>* cv = part->getcalohitv();
            if(!cv||cv->empty()) continue;
            MiCDCaloHit* hit = part->getcalohit(0);
            if(hit){ E_OM=hit->getE(); break; }
        }
        if(E_OM<0||!std::isfinite(E_OM)) continue;

        MiSD* sd = Eve->getSD();
        double E_carrier = 0.0;
        if(sd){
            std::vector<MiSDCaloHit>* ch = sd->getcalohitv();
            if(ch) for(auto& hit:*ch)
                if(hit.getcategory()=="bi207_calib_source"
                   && hit.getname()=="e-"
                   && hit.getGID()->gettype()=="1066"
                   // GID 1111 ?? 1112 ? ????? ????, ?????????????
                   // || hit.getGID()->gettype()=="1111"
                   // || hit.getGID()->gettype()=="1112"
                   )
                    E_carrier += hit.getE();
        }

        hAll->Fill(E_OM);
        ++n_all;

        // if(E_carrier >= LOSS_THRESHOLD){
        if(E_carrier > 0.0){
            hLoss->Fill(E_OM);
            ++n_loss;
        }
    }

    std::cout<<"All electrons         : "<<n_all<<"\n";
    std::cout<<"With GID 1066 deposit : "<<n_loss
             <<"  ("<<(n_all>0?100.0*n_loss/n_all:0)<<"%)\n";

   // const double norm = hAll->Integral();
   // if(norm > 0){
   //     hAll ->Scale(1.0 / norm);
   //     hLoss->Scale(1.0 / norm);
   // }

    gStyle->SetOptStat(0);

    TCanvas* c = new TCanvas("c","Overlay: all vs GID 1066 loss", 1100, 550);
    c->SetLeftMargin(0.10);
    c->SetRightMargin(0.04);
    c->SetBottomMargin(0.13);
    c->SetTopMargin(0.08);

    hAll->SetLineColor(TColor::GetColor("#7021dd"));
    hAll->SetLineWidth(2);
    hAll->SetFillStyle(0);
    hAll->SetOption(DRAW_STYLE);

    hLoss->SetLineColor(TColor::GetColor("#e42536"));
    hLoss->SetLineWidth(2);
    hLoss->SetFillColor(TColor::GetColor("#e42536")); 
    hLoss->SetFillStyle(3004);
    hLoss->SetOption(DRAW_STYLE);

    hAll->GetXaxis()->SetRangeUser(X_MIN, X_MAX);
    hAll->GetXaxis()->SetTitleSize(0.05);
    hAll->GetXaxis()->SetLabelSize(0.04);
    hAll->GetYaxis()->SetTitleSize(0.05);
    hAll->GetYaxis()->SetLabelSize(0.04);
    hAll->GetYaxis()->SetTitleOffset(0.85);

    hAll->Draw(DRAW_STYLE);
    hLoss->Draw("HIST SAME");
    if(std::string(DRAW_STYLE) == "E")
        hLoss->Draw("E SAME");


    TLegend* leg = new TLegend(0.15, 0.75, 0.40, 0.88);

    leg->SetBorderSize(0);
    leg->SetFillStyle(0);
    leg->SetTextSize(0.040);
    leg->AddEntry(hAll,  "All e^{-}", "l");
    leg->AddEntry(hLoss, "Envelope e^{-}", "lf");
    leg->Draw();

    c->SaveAs(OUTPUT_PNG.c_str());

    TFile fout(OUTPUT_ROOT.c_str(),"RECREATE");
    hAll->Write(); hLoss->Write();
    fout.Close();

    f->Close(); delete f; delete Eve;
    std::cout<<"Saved: "<<OUTPUT_PNG<<"  "<<OUTPUT_ROOT<<"\n";
}
