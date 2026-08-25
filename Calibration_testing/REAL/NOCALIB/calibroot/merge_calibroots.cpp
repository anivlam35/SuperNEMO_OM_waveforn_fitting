// root -l -b -q 'merge_calibroots.cpp("merged.root","filelist.txt")'

#include <TFile.h>
#include <TTree.h>
#include <TChain.h>
#include <TKey.h>
#include <fstream>
#include <iostream>
#include <set>
#include <string>
#include <vector>

static std::vector<std::string> read_filelist(const char* path)
{
  std::vector<std::string> files;
  std::ifstream in(path);
  if (!in) { std::cerr << "Cannot open filelist: " << path << "\n"; return files; }
  std::string line;
  while (std::getline(in, line)) {
    while (!line.empty() && (line.back()=='\r'||line.back()=='\n'||
                              line.back()==' ' ||line.back()=='\t'))
      line.pop_back();
    if (line.empty() || line[0]=='#') continue;
    files.push_back(line);
  }
  return files;
}

static std::set<std::string> get_tree_names(const std::string& fname)
{
  std::set<std::string> names;
  TFile f(fname.c_str(), "READ");
  if (f.IsZombie()) return names;
  TIter next(f.GetListOfKeys());
  TKey* key = nullptr;
  while ((key = (TKey*)next())) {
    if (std::string(key->GetClassName()).find("TTree") != std::string::npos)
      names.insert(key->GetName());
  }
  f.Close();
  return names;
}

void merge_calibroots(const char* outFile, const char* filelist_txt)
{
  auto files = read_filelist(filelist_txt);
  if (files.empty()) { std::cerr << "ERROR: empty filelist\n"; return; }
  std::cout << "Files in list: " << files.size() << "\n";

auto numericLess = [](const std::string& a, const std::string& b) {
  return std::stoi(a) < std::stoi(b);
};
std::set<std::string, decltype(numericLess)> allTreeNames(numericLess);

  std::vector<std::string> validFiles;

  for (const auto& f : files) {
    auto names = get_tree_names(f);
    if (names.empty()) {
      std::cerr << "SKIP (no trees or zombie): " << f << "\n";
      continue;
    }
    validFiles.push_back(f);
    for (const auto& n : names) allTreeNames.insert(n);
  }

  std::cout << "Valid files   : " << validFiles.size() << "\n";
  std::cout << "Unique trees  : " << allTreeNames.size() << "\n";

  if (validFiles.empty() || allTreeNames.empty()) {
    std::cerr << "ERROR: nothing to merge\n";
    return;
  }

  TFile* fout = TFile::Open(outFile, "RECREATE");
  if (!fout || fout->IsZombie()) {
    std::cerr << "ERROR: cannot create output: " << outFile << "\n";
    return;
  }

  int mergedCount = 0;
  int skippedEmpty = 0;
  Long64_t totalEntries = 0;

  for (const auto& treeName : allTreeNames) {
    TChain ch(treeName.c_str());
    int nAdded = 0;

    for (const auto& f : validFiles) {
      int added = ch.Add((f + "/" + treeName).c_str(), -1);
      nAdded += added;
    }

    if (nAdded == 0) continue;

    // FIX: skip empty chains to avoid segfault in CloneTree
    Long64_t nEntries = ch.GetEntries();
    if (nEntries <= 0) {
      skippedEmpty++;
      continue;
    }

    fout->cd();
    TTree* outTree = ch.CloneTree(-1, "fast");

    // FIX: check for nullptr in case CloneTree still fails
    if (!outTree) {
      std::cerr << "WARNING: CloneTree returned nullptr for tree: " << treeName << "\n";
      continue;
    }

    outTree->SetName(treeName.c_str());
    outTree->SetTitle(treeName.c_str());
    outTree->Write("", TObject::kOverwrite);
    delete outTree;

    totalEntries += nEntries;
    mergedCount++;

    if (mergedCount % 50 == 0)
      std::cout << "Merged " << mergedCount << " / "
                << allTreeNames.size() << " trees...\n";
  }

  fout->Close();

  std::cout << "\n========== DONE ==========\n";
  std::cout << "Valid input files : " << validFiles.size() << "\n";
  std::cout << "Trees merged      : " << mergedCount << "\n";
  std::cout << "Trees skipped (empty): " << skippedEmpty << "\n";
  std::cout << "Total entries     : " << totalEntries << "\n";
  std::cout << "Output            : " << outFile << "\n";
  std::cout << "===========================\n";
}
