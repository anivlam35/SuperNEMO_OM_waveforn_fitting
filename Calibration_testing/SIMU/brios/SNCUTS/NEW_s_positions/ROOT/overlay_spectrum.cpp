// overlay_with_profile.C
//
// ??? ?????? ?? ?????? canvas:
//   ??????: E_OM ?????? ???? ?????????? (?????)
//   ?????:  ??????? ?????? <E_carrier> vs E_OM (TProfile, ????????)
//           ? ?????? ????? ?? ???????? ??????? ????? GID 1111 ??? 1112
//
// Usage:
//   root -l -b -q 'overlay_with_profile.C()'

#include "/sps/nemo/scratch/ykozina/Falaise/tutorial/MiModule/include/MiEvent.h"

#include <TCanvas.h>
#include <TH1D.h>
#include <TProfile.h>
#include <TFile.h>
#include <TTree.h>
#include <TLegend.h>
#include <TPad.h>
#include <TStyle.h>
#include <TPaveText.h>

#include <iostream>
#include <string>
#include <vector>
#include <cmath>

R__LOAD_LIBRARY(/sps/nemo/scratch/ykozina/Falaise/tutorial/MiModule/lib/libMiModule.so);

void overlay_spectrum()
{
    // ============================================================
    const std::string INPUT_ROOT =
        "/sps/nemo/scratch/ykozina/Falaise/calibration/calibration-CORRECTIONS/"
        "SIMU/brios/SNCUTS/NEW_s_positions/ROOT/merged_simu.root";

    const std::string OUTPUT_PNG  = "overlay_with_profile.png";
    const std::string OUTPUT_ROOT = "overlay_with_profile.root";

    const double BIN_KEV  = 5.0;
    const double X_MIN    = 0.0;
    const double X_MAX    = 1400.0;

    const Long64_t MAX_EVENTS = 0;   // 0 = ???
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

    // ?????? ?????? ? ?????? ??????
    TH1D* hAll = new TH1D("hAll",
        ";E_{OM} [keV];Normalized counts",
        nbins, X_MIN, X_MAX);
    hAll->Sumw2();

    // ????? ?????? ? ??????? ?????? ?? ??????? E_OM
    TProfile* hProf = new TProfile("hProf",
        ";E_{OM} [keV];<E_{carrier}> [keV]",
        nbins, X_MIN, X_MAX, 0, 500);

    Long64_t n_all=0, n_carrier=0;

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

        // ??????? e- ??? ? ????? GID 1111 ??? 1112
        MiSD* sd = Eve->getSD();
        double E_carrier = 0.0;
        bool   hit_carrier = false;

        if(sd){
            std::vector<MiSDCaloHit>* ch = sd->getcalohitv();
            if(ch){
                for(auto& hit : *ch){
                    if(hit.getcategory() == "bi207_calib_source"
                       && hit.getname()  == "e-"
                       && (hit.getGID()->gettype() == "1111"
                        || hit.getGID()->gettype() == "1112"))
                    {
                        E_carrier  += hit.getE();
                        hit_carrier = true;
                    }
                }
            }
        }

        hAll->Fill(E_OM);
        ++n_all;

        if(hit_carrier){
            hProf->Fill(E_OM, E_carrier);   // ??????? E_carrier ? ??????? ???? E_OM
            ++n_carrier;
        }
    }

    std::cout<<"All electrons        : "<<n_all<<"\n";
    std::cout<<"With carrier hit     : "<<n_carrier<<"\n";
    std::cout<<"Fraction             : "
             <<(n_all>0?100.0*n_carrier/n_all:0)<<" %\n";

    if(hAll->Integral()>0) hAll->Scale(1.0/hAll->Integral());

    // -------- ????????? --------
    gStyle->SetOptStat(0);

    TCanvas* c = new TCanvas("c","Spectrum + carrier loss profile",900,800);

    // ?????? ?????? ? 60% ??????
    TPad* pad1 = new TPad("pad1","",0,0.38,1,1);
    pad1->SetBottomMargin(0.02);
    pad1->SetLeftMargin(0.12);
    pad1->Draw();

    // ????? ?????? ? 40% ??????
    TPad* pad2 = new TPad("pad2","",0,0,1,0.38);
    pad2->SetTopMargin(0.02);
    pad2->SetBottomMargin(0.18);
    pad2->SetLeftMargin(0.12);
    pad2->Draw();

    // --- ?????? ---
    pad1->cd();
    hAll->SetLineColor(kBlue+1);
    hAll->SetLineWidth(2);
    hAll->GetXaxis()->SetLabelSize(0);   // ??????? ??????? ??? X
    hAll->GetXaxis()->SetRangeUser(X_MIN, X_MAX);
    hAll->GetYaxis()->SetTitleSize(0.06);
    hAll->GetYaxis()->SetTitleOffset(0.9);
    hAll->Draw("HIST");

    TPaveText* pt = new TPaveText(0.55,0.75,0.88,0.88,"NDC");
    pt->SetBorderSize(1); pt->SetFillColor(0); pt->SetTextAlign(12);
    pt->SetTextSize(0.05);
    pt->AddText(Form("All e^{-}: %lld", n_all));
    pt->Draw();

    TLegend* leg1 = new TLegend(0.55,0.60,0.88,0.74);
    leg1->SetBorderSize(1); leg1->SetFillColor(0); leg1->SetTextSize(0.05);
    leg1->AddEntry(hAll,"All electrons (E_{OM})","l");
    leg1->Draw();

    // --- ????? ---
    pad2->cd();
    hProf->SetLineColor(kRed+1);
    hProf->SetMarkerColor(kRed+1);
    hProf->SetMarkerStyle(20);
    hProf->SetMarkerSize(0.4);
    hProf->SetLineWidth(1);
    hProf->GetXaxis()->SetRangeUser(X_MIN, X_MAX);
    hProf->GetXaxis()->SetTitleSize(0.09);
    hProf->GetXaxis()->SetLabelSize(0.07);
    hProf->GetYaxis()->SetTitleSize(0.08);
    hProf->GetYaxis()->SetTitleOffset(0.7);
    hProf->GetYaxis()->SetLabelSize(0.07);
    hProf->Draw("E1");

    TLegend* leg2 = new TLegend(0.55,0.75,0.88,0.92);
    leg2->SetBorderSize(1); leg2->SetFillColor(0); leg2->SetTextSize(0.07);
    leg2->AddEntry(hProf,
        Form("<E_{carrier}> GID 1111/1112  (%lld evt)",n_carrier),"lp");
    leg2->Draw();

    c->SaveAs(OUTPUT_PNG.c_str());

    TFile fout(OUTPUT_ROOT.c_str(),"RECREATE");
    hAll->Write(); hProf->Write();
    fout.Close();

    f->Close(); delete f; delete Eve;
    std::cout<<"Saved: "<<OUTPUT_PNG<<"  "<<OUTPUT_ROOT<<"\n";
}
