// list_carrier_gids.C
//
// Collects all unique GID type numbers from bi207_calib_source hits.
// Algorithm exactly as requested:
//   1. empty set to hold GID types
//   2. loop all events
//   3. for each calohit: if GID type not in set ? add it, else skip
//   4. at end print the list + count + mean/max E per type
//
// Usage:
//   root -l -b -q 'list_carrier_gids.C()'

#include "/sps/nemo/scratch/ykozina/Falaise/tutorial/MiModule/include/MiEvent.h"

#include <TFile.h>
#include <TTree.h>

#include <iostream>
#include <string>
#include <set>
#include <map>
#include <vector>
#include <iomanip>
#include <cmath>

R__LOAD_LIBRARY(/sps/nemo/scratch/ykozina/Falaise/tutorial/MiModule/lib/libMiModule.so);

void list_carrier_gids()
{
    const std::string INPUT_ROOT =
        "/sps/nemo/scratch/ykozina/Falaise/calibration/calibration-CORRECTIONS/"
        "SIMU/brios/SNCUTS/NEW_s_positions/ROOT/merged_simu.root";

    TFile* f = TFile::Open(INPUT_ROOT.c_str(), "READ");
    if(!f||f->IsZombie()){ std::cerr<<"ERROR: cannot open input\n"; return; }
    TTree* tree = dynamic_cast<TTree*>(f->Get("Event"));
    if(!tree){ std::cerr<<"ERROR: no Event tree\n"; f->Close(); return; }

    const Long64_t N = tree->GetEntries();
    std::cout<<"Total events: "<<N<<"\n";

    MiEvent* Eve = new MiEvent();
    tree->SetBranchAddress("Eventdata", &Eve);

    // step 1: empty set of known GID types
    std::set<std::string> knownGIDs;

    // for statistics per GID type
    struct Stat { long long count=0; double sumE=0; double maxE=0; };
    std::map<std::string, Stat> stats;

    // step 2: loop all events
    for(Long64_t i=0; i<N; ++i){
        tree->GetEntry(i);
        if((i+1)%200000==0)
            std::cout<<"  processed "<<(i+1)<<"/"<<N
                     <<"  unique GIDs so far: "<<knownGIDs.size()<<"\n";

        MiSD* sd = Eve->getSD();
        if(!sd) continue;
        std::vector<MiSDCaloHit>* ch = sd->getcalohitv();
        if(!ch) continue;

        // step 3: loop all calohits
        for(auto& hit : *ch){
            if(hit.getcategory() != "bi207_calib_source") continue;

            const std::string gtype = hit.getGID()->gettype();

            // if not in list ? add it
            if(knownGIDs.find(gtype) == knownGIDs.end()){
                knownGIDs.insert(gtype);
                std::cout<<"  [NEW GID FOUND] type = "<<gtype
                         <<" at event "<<i<<"\n";
            }

            // accumulate stats regardless
            auto& st = stats[gtype];
            st.count++;
            st.sumE += hit.getE();
            if(hit.getE() > st.maxE) st.maxE = hit.getE();
        }
        // step 4: repeat until all events done
    }

    // print final list
    std::cout<<"\n========== RESULT ==========\n";
    std::cout<<"Unique GID types in bi207_calib_source: "
             <<knownGIDs.size()<<"\n\n";
    std::cout<<std::left
             <<std::setw(12)<<"GID type"
             <<std::setw(14)<<"Hits total"
             <<std::setw(16)<<"Mean E [keV]"
             <<std::setw(16)<<"Max E [keV]"
             <<"\n"
             <<std::string(58,'-')<<"\n";

    for(const auto& gtype : knownGIDs){
        auto& st = stats[gtype];
        std::cout<<std::left
                 <<std::setw(12)<<gtype
                 <<std::setw(14)<<st.count
                 <<std::setw(16)<<std::fixed<<std::setprecision(3)
                 <<(st.count>0 ? st.sumE/st.count : 0.0)
                 <<std::setw(16)<<st.maxE
                 <<"\n";
    }
    std::cout<<"============================\n";

    f->Close(); delete f; delete Eve;
}
