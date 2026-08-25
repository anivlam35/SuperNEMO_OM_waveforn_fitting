// merge_fixed.C
// Merges MiModule ROOT files into a single output ROOT file.
// Fixes: correct SetMaxTreeSize placement, single-file output guarantee.
//
// Usage:
//   root -l -b -q 'merge.cpp("merged_simu.root","filelist.txt")'
//   root -l -b -q 'merge.cpp("merged_simu.root","filelist.txt", 0)'  // 0 = no entry limit

#include <TFile.h>
#include <TKey.h>
#include <TDirectory.h>
#include <TTree.h>
#include <TClass.h>
#include <TSystem.h>
#include <TFileMerger.h>

#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <vector>

// ---------- helpers ----------

static std::vector<std::string> read_filelist(const char* path)
{
    std::vector<std::string> v;
    std::ifstream in(path);
    if (!in) { std::cerr << "ERROR: cannot open filelist: " << path << "\n"; return v; }
    std::string line;
    while (std::getline(in, line)) {
        // strip \r, leading/trailing spaces
        while (!line.empty() && (line.back()=='\r'||line.back()=='\n'||
                                  line.back()==' ' ||line.back()=='\t')) line.pop_back();
        size_t i=0; while(i<line.size()&&(line[i]==' '||line[i]=='\t'))++i;
        if(i) line=line.substr(i);
        if(line.empty()||line[0]=='#') continue;
        v.push_back(line);
    }
    return v;
}

struct FileInfo { bool ok=false; Long64_t entries=0; std::string reason; };

static FileInfo inspect(const std::string& fname)
{
    FileInfo r;
    TFile f(fname.c_str(),"READ");
    if(f.IsZombie())      { r.reason="zombie";     return r; }
    if(f.TestBit(TFile::kRecovered)) { r.reason="recovered"; return r; }

    TTree* t = (TTree*)f.Get("Event");
    if(!t)                { r.reason="no 'Event' tree"; return r; }
    r.entries = t->GetEntries();
    if(r.entries==0)      { r.reason="empty tree"; return r; }
    r.ok=true; r.reason="ok";
    return r;
}

// ---------- main ----------

void merge(const char* outFile,
                 const char* filelist_txt,
                 Long64_t    maxEntries = 0)   // 0 = no limit
{
    // IMPORTANT: must be called BEFORE any TFile/TTree is opened
    TTree::SetMaxTreeSize(5000000000000LL);   // 5 TB ? prevents file splitting

    auto files = read_filelist(filelist_txt);
    if(files.empty()){ std::cerr<<"ERROR: empty filelist\n"; return; }

    std::vector<std::string> valid;
    Long64_t accumulated = 0;
    int nSkipped = 0;

    for(const auto& f : files){
        if(maxEntries>0 && accumulated>=maxEntries){
            std::cout<<"[STOP] entry limit "<<maxEntries<<" reached.\n"; break;
        }
        FileInfo info = inspect(f);
        if(!info.ok){
            std::cerr<<"[SKIP] "<<f<<" | "<<info.reason<<"\n";
            ++nSkipped; continue;
        }
        valid.push_back(f);
        accumulated += info.entries;
        std::cout<<"[OK]   "<<f<<" | entries="<<info.entries
                 <<" | total="<<accumulated<<"\n";
    }

    if(valid.empty()){ std::cerr<<"ERROR: no valid files.\n"; return; }

    // kFALSE = local merger (not parallel), keeps single output file
    TFileMerger merger(kFALSE, kFALSE);
    merger.SetFastMethod(kTRUE);

    // "RECREATE" ? always single output file, never splits
    if(!merger.OutputFile(outFile, "RECREATE")){
        std::cerr<<"ERROR: cannot create output: "<<outFile<<"\n"; return;
    }

    for(const auto& f : valid)
        if(!merger.AddFile(f.c_str()))
            std::cerr<<"WARNING: could not add "<<f<<"\n";

    std::cout<<"\nMerging "<<valid.size()<<" files ("<<accumulated<<" entries) ? "<<outFile<<"\n";

    if(!merger.Merge()){ std::cerr<<"ERROR: Merge() failed\n"; return; }

    std::cout<<"\n===== MERGE DONE =====\n"
             <<"Files merged : "<<valid.size()<<"\n"
             <<"Files skipped: "<<nSkipped<<"\n"
             <<"Entries total: "<<accumulated<<"\n"
             <<"Output       : "<<outFile<<"\n"
             <<"======================\n";
}
