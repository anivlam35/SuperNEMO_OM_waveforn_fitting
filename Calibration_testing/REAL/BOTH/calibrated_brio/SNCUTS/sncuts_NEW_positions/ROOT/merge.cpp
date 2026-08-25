// merge.C
// Recursively finds all TTrees in input ROOT files and merges them by full path
// using TChain::Merge (robust for TBranchElement/object branches).
//
// Usage:
//   root -l -b -q 'merge.cpp("merged_week_2025_X.root","filelist.txt")'

#include <TFile.h>
#include <TKey.h>
#include <TDirectory.h>
#include <TTree.h>
#include <TClass.h>
#include <TChain.h>

#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <vector>

struct FileCheckResult {
  bool ok = false;
  bool hasTree = false;
  bool hasNonEmptyTree = false;
  Long64_t totalEntries = 0;
  std::vector<std::string> treePaths;
  std::string reason;
};

static std::vector<std::string> read_filelist(const char* filelist_txt)
{
  std::vector<std::string> files;
  std::ifstream in(filelist_txt);
  if (!in) return files;

  std::string line;
  while (std::getline(in, line)) {
    while (!line.empty() &&
           (line.back()=='\r' || line.back()=='\n' || line.back()==' ' || line.back()=='\t'))
      line.pop_back();

    size_t i = 0;
    while (i < line.size() && (line[i]==' ' || line[i]=='\t')) ++i;
    if (i) line = line.substr(i);

    if (line.empty() || line[0]=='#') continue;
    files.push_back(line);
  }
  return files;
}

static TDirectory* ensure_outdir(TFile* fout, const std::string& path)
{
  if (path.empty()) return fout;

  TDirectory* cur = fout;
  size_t start = 0;

  while (start < path.size()) {
    size_t slash = path.find('/', start);
    std::string part = (slash == std::string::npos)
                     ? path.substr(start)
                     : path.substr(start, slash - start);

    if (!part.empty()) {
      TDirectory* next = cur->GetDirectory(part.c_str());
      if (!next) next = cur->mkdir(part.c_str());
      cur = next;
    }

    if (slash == std::string::npos) break;
    start = slash + 1;
  }

  return cur;
}

// Collect full tree paths and entry statistics from one opened file/directory
static void collect_trees_recursive(TDirectory* dir,
                                    const std::string& curPath,
                                    std::vector<std::string>& treePaths,
                                    bool& hasTree,
                                    bool& hasNonEmptyTree,
                                    Long64_t& totalEntries)
{
  TIter nextKey(dir->GetListOfKeys());
  TKey* key = nullptr;

  while ((key = (TKey*)nextKey())) {
    TClass* cl = TClass::GetClass(key->GetClassName());
    if (!cl) continue;

    if (cl->InheritsFrom("TDirectory")) {
      TDirectory* sub = (TDirectory*)key->ReadObj();
      if (!sub) continue;

      std::string subPath = curPath.empty()
                          ? std::string(sub->GetName())
                          : (curPath + "/" + sub->GetName());

      collect_trees_recursive(sub, subPath, treePaths, hasTree, hasNonEmptyTree, totalEntries);
      delete sub;
    }
    else if (cl->InheritsFrom("TTree")) {
      TTree* tr = (TTree*)key->ReadObj();
      if (!tr) continue;

      hasTree = true;

      std::string name = key->GetName();
      std::string full = curPath.empty() ? name : (curPath + "/" + name);
      treePaths.push_back(full);

      Long64_t nent = tr->GetEntries();
      totalEntries += nent;
      if (nent > 0) hasNonEmptyTree = true;

      delete tr;
    }
  }
}

static FileCheckResult inspect_file(const std::string& fname)
{
  FileCheckResult res;

  TFile fin(fname.c_str(), "READ");
  if (fin.IsZombie()) {
    res.reason = "zombie / cannot open";
    return res;
  }

  if (fin.TestBit(TFile::kRecovered)) {
    res.reason = "recovered ROOT file";
    fin.Close();
    return res;
  }

  collect_trees_recursive(&fin, "", res.treePaths,
                          res.hasTree, res.hasNonEmptyTree, res.totalEntries);

  fin.Close();

  if (!res.hasTree) {
    res.reason = "no TTrees found";
    return res;
  }

  if (!res.hasNonEmptyTree) {
    res.reason = "all TTrees have 0 entries";
    return res;
  }

  res.ok = true;
  res.reason = "ok";
  return res;
}

void merge(const char* outFile, const char* filelist_txt)
{
  auto files = read_filelist(filelist_txt);
  if (files.empty()) {
    std::cerr << "ERROR: filelist is empty or cannot be read: " << filelist_txt << "\n";
    return;
  }

  std::vector<std::string> validFiles;
  std::map<std::string, std::string> skippedFiles;
  std::set<std::string> treePathSet;

  // 1) Validate all files and collect union of tree paths
  for (const auto& f : files) {
    FileCheckResult chk = inspect_file(f);

    if (!chk.ok) {
      skippedFiles[f] = chk.reason;
      continue;
    }

    validFiles.push_back(f);
    for (const auto& tp : chk.treePaths) {
      treePathSet.insert(tp);
    }

    std::cout << "[VALID] " << f
              << " | trees=" << chk.treePaths.size()
              << " | total entries=" << chk.totalEntries
              << "\n";
  }

  if (validFiles.empty()) {
    std::cerr << "ERROR: No valid input ROOT files to merge.\n";
    std::cerr << "Total files in list: " << files.size() << "\n";
    std::cerr << "Skipped files: " << skippedFiles.size() << "\n";
    for (const auto& kv : skippedFiles) {
      std::cerr << "  SKIP: " << kv.first << " | reason: " << kv.second << "\n";
    }
    return;
  }

  std::vector<std::string> treePaths(treePathSet.begin(), treePathSet.end());

  if (treePaths.empty()) {
    std::cerr << "ERROR: No TTrees found in valid input files.\n";
    return;
  }

  TFile* fout = TFile::Open(outFile, "RECREATE");
  if (!fout || fout->IsZombie()) {
    std::cerr << "ERROR: Cannot create output file: " << outFile << "\n";
    return;
  }

  Long64_t totalMergedEntries = 0;
  int mergedTreeCount = 0;

  // 2) For each tree path, build a chain over all valid files that contain it
  for (const auto& fullPath : treePaths) {
    size_t pos = fullPath.rfind('/');
    std::string dirPath  = (pos == std::string::npos) ? "" : fullPath.substr(0, pos);
    std::string treeName = (pos == std::string::npos) ? fullPath : fullPath.substr(pos + 1);

    TChain ch(treeName.c_str());
    int nAdded = 0;

    for (const auto& f : validFiles) {
      std::string spec = f + "/" + fullPath;
      int added = ch.Add(spec.c_str(), 0);
      if (added > 0) nAdded += added;
    }

    if (nAdded == 0) continue;

    TDirectory* outDir = ensure_outdir(fout, dirPath);
    outDir->cd();

    Long64_t n = ch.Merge(fout, -1, "fast");
    if (n > 0) totalMergedEntries += n;
    if (n >= 0) mergedTreeCount++;

    std::cout << "Merged: " << fullPath
              << " | files added=" << nAdded
              << " | entries=" << n
              << "\n";
  }

  fout->Close();

  // 3) Final report
  std::cout << "\n========== MERGE SUMMARY ==========\n";
  std::cout << "Total files listed      : " << files.size() << "\n";
  std::cout << "Valid files merged from : " << validFiles.size() << "\n";
  std::cout << "Skipped files           : " << skippedFiles.size() << "\n";
  std::cout << "Merged tree paths       : " << mergedTreeCount << "\n";
  std::cout << "Total merged entries    : " << totalMergedEntries << "\n";
  std::cout << "Output file             : " << outFile << "\n";

  if (!skippedFiles.empty()) {
    std::cout << "\nSkipped files:\n";
    for (const auto& kv : skippedFiles) {
      std::cout << "  " << kv.first << " | reason: " << kv.second << "\n";
    }
  }

  std::cout << "===================================\n";
}
