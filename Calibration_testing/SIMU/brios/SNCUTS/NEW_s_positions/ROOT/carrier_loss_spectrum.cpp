// carrier_loss_spectrum.C
//
// Full spectrum of e- energy loss in source carrier (GID 1111/1112).
// Log Y scale to see the copper tail at 100-400 keV.
// BIN_KEV=2.0 to see full 0-400 keV range clearly.
//
// Usage:
//   root -l -b -q 'carrier_loss_spectrum.C()'

#include "/sps/nemo/scratch/ykozina/Falaise/tutorial/MiModule/include/MiEvent.h"

#include <TCanvas.h>
#include <TH1D.h>
#include <TFile.h>
#include <TTree.h>
#include <TPaveText.h>
#include <TStyle.h>
#include <TLine.h>

#include <iostream>
#include <string>
#include <vector>
#include <cmath>

R__LOAD_LIBRARY(/sps/nemo/scratch/ykozina/Falaise/tutorial/MiModule/lib/libMiModule.so);

void carrier_loss_spectrum()
{
    // ============================================================
    const std::string INPUT_ROOT =
        "/sps/nemo/scratch/ykozina/Falaise/calibration/calibration-CORRECTIONS/"
        "SIMU/brios/SNCUTS/NEW_s_positions/ROOT/merged_simu.root";

    const std::string OUTPUT_PNG  = "carrier_loss_spectrum-lin-bin05kev.png";
    const std::string OUTPUT_ROOT = "carrier_loss_spectrum-lin-bin05kev.root";

    const double BIN_KEV = 0.5;   // ????????? ??? ??? ?????? ????? ?? 400 keV
    const double X_MIN   = 0.0;
    const double X_MAX   = 600.0; // ?????? ???????? ??????? ? ?????

    const Long64_t MAX_EVENTS = 0;
    // ============================================================

    TFile* f = TFile::Open(INPUT_ROOT.c_str(), "READ");
    if(!f||f->IsZombie()){ std::cerr<<"ERROR: cannot open "<<INPUT_ROOT<<"\n"; return; }
    TTree* tree = dynamic_cast<TTree*>(f->Get("Event"));
    if(!tree){ std::cerr<<"ERROR: no 'Event' tree\n"; f->Close(); return; }

    const Long64_t N = (MAX_EVENTS>0)
                     ? std::min(MAX_EVENTS,tree->GetEntries())
                     : tree->GetEntries();
    std::cout<<"Entries: "<<N<<"\n";

    MiEvent* Eve = new MiEvent();
    tree->SetBranchAddress("Eventdata", &Eve);

    const int nbins = (int)((X_MAX-X_MIN)/BIN_KEV+0.5);
    TH1D* h = new TH1D("h_carrier_loss",
        "Energy loss in source carrier (e^{-} only, GID 1111/1112);"
        "E_{carrier} [keV];Normalized counts",
        nbins, X_MIN, X_MAX);
    h->Sumw2(kTRUE);

    Long64_t n_processed=0, n_filled=0, n_zero=0;

    for(Long64_t i=0;i<N;++i){
        tree->GetEntry(i);
        ++n_processed;
        if((i+1)%100000==0)
            std::cout<<"  "<<(i+1)<<"/"<<N<<"  filled="<<n_filled<<"\n";

        MiPTD* ptd = Eve->getPTD();
        if(!ptd) continue;
        std::vector<MiCDParticle>* partv = ptd->getpartv();
        if(!partv||partv->empty()) continue;

        bool has_electron = false;
        for(int ip=0;ip<(int)partv->size();++ip){
            MiCDParticle* part = ptd->getpart(ip);
            if(part&&part->getcharge()==1000){ has_electron=true; break; }
        }
        if(!has_electron) continue;

        MiSD* sd = Eve->getSD();
        if(!sd) continue;
        std::vector<MiSDCaloHit>* calohits = sd->getcalohitv();
        if(!calohits) continue;

        double E_carrier = 0.0;
        for(auto& hit : *calohits){
            if(hit.getcategory()=="bi207_calib_source"
               && hit.getname()=="e-"
               && (hit.getGID()->gettype()=="1111"
                || hit.getGID()->gettype()=="1112"))
                E_carrier += hit.getE();
        }

        if(!std::isfinite(E_carrier)) continue;
        if(E_carrier<=0.0){ ++n_zero; continue; }

        h->Fill(E_carrier);
        ++n_filled;
    }

    if(h->Integral()>0) h->Scale(1.0/h->Integral());
    h->SetEntries((double)n_filled);

    // ??????? ??????? ????? ???? ?????? ???????
    auto countAbove = [&](double thr) -> long long {
        long long cnt = 0;
        for(int ib=1;ib<=h->GetNbinsX();++ib)
            if(h->GetBinCenter(ib)>=thr) cnt += (long long)(h->GetBinContent(ib)*n_filled+0.5);
        return cnt;
    };
    std::cout<<"\nFraction above threshold:\n";
    for(double thr : {10.0,20.0,50.0,100.0,150.0,200.0})
        std::cout<<"  E_carrier > "<<thr<<" keV : "
                 <<(100.0*countAbove(thr)/n_filled)<<" %\n";

    gStyle->SetOptStat(0);
    TCanvas* c = new TCanvas("c","Carrier loss spectrum",900,600);
    //c->SetLogy();   // LOG ????? ? ??????? ??? ?????? ?????? ?????
    c->SetLeftMargin(0.12);

    h->SetLineWidth(1);
    h->SetFillStyle(0);
    h->GetXaxis()->SetRangeUser(X_MIN, X_MAX);
    h->Draw("E");

    // ???????? ??????????
    TPaveText* pt = new TPaveText(0.55,0.75,0.88,0.90,"NDC");
    pt->SetBorderSize(1); pt->SetFillColor(0); pt->SetTextAlign(12);
    pt->SetTextSize(0.033);
    pt->AddText(Form("Entries = %lld", n_filled));
    pt->AddText(Form("Mean = %.2f keV", h->GetMean()));
    pt->Draw();

    // ??????????? ?????-????????
    auto vline = [&](double x, int col){
        TLine* l = new TLine(x, h->GetMinimum()*2, x, h->GetMaximum()*0.5);
        l->SetLineColor(col); l->SetLineStyle(2); l->SetLineWidth(1);
        l->Draw();
    };
    vline(10.0,  kRed+1);    // ???? ??????? ?????????
    vline(100.0, kGreen+2);  // ??????????? ????? ??? overlay

    c->SaveAs(OUTPUT_PNG.c_str());

    TFile fout(OUTPUT_ROOT.c_str(),"RECREATE");
    h->Write();
    fout.Close();

    f->Close(); delete f; delete Eve;
    std::cout<<"\n===== DONE =====\n"
             <<"Processed : "<<n_processed<<"\n"
             <<"Filled    : "<<n_filled<<"\n"
             <<"Zero loss : "<<n_zero<<"\n"
             <<"Output    : "<<OUTPUT_PNG<<"\n"
             <<"================\n";
}
