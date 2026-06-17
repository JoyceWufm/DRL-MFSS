

#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <map>
#include <cmath>
#include <algorithm>
#include <chrono>
#include <iomanip>
#include <filesystem>


#include <ilcplex/ilocplex.h>
#include "Instances.h"
#include "ParamsUtils.h" 

namespace fs = std::filesystem;


const std::string REPORT_FILENAME = "Verification_Report.csv";


struct RowData {
    std::string key;        // Type_Size_Index
    double expectedObj;     // BestFoundObj
    std::vector<int> solution; 
    std::string seed;
    double inputTime; // time from input CSV (Time(s))
    std::string originalLine; 
    bool valid = false;
};

// =========================================================

// =========================================================
std::map<std::string, std::string> IndexInstanceFiles(const std::string& dirPath) {
    std::map<std::string, std::string> fileMap;
    std::cout << ">>> Indexing instances in: " << dirPath << " ..." << std::endl;

    if (!fs::exists(dirPath)) {
        std::cerr << "Error: Directory not found: " << dirPath << std::endl;
        return fileMap;
    }

    int count = 0;
    for (const auto& entry : fs::recursive_directory_iterator(dirPath)) {
        if (entry.is_regular_file()) {
            std::string path = entry.path().string();
            
            std::string key = PU::GetKeyFromPath(path);
            if (!key.empty()) {
                fileMap[key] = path;
                count++;
            }
        }
    }
    std::cout << ">>> Indexed " << count << " instances." << std::endl;
    return fileMap;
}

// =========================================================

// =========================================================
RowData ParseCsvLine(const std::string& line) {
    // Deprecated: this function is kept for backward compatibility but not used
    RowData data;
    (void)line;
    return data;
}


RowData ParseCsvLineByIndices(const std::string& line,
    const std::map<std::string,int>& idxMap,
    int itemStartIdx) {

    RowData data;
    if (line.empty()) return data;
    auto cols = PU::splitString(line, ',');

    
    if (cols.size() <= 3) return data;

    // Key: prefer fullpath/key column if available
    if (idxMap.count("fullpath")) {
        std::string val = cols[idxMap.at("fullpath")];
        // if looks like a path or filename, try to extract key via PU::GetKeyFromPath
        if (val.find('/') != std::string::npos || val.find('\\') != std::string::npos || val.find(".txt") != std::string::npos) {
            data.key = PU::GetKeyFromPath(val);
        } else if (val.find('_') != std::string::npos) {
            // maybe already Type_Size_Index
            data.key = val;
        } else {
            // fallback: treat as filename
            data.key = PU::GetKeyFromPath(val);
        }
    } else {
        std::string type = (idxMap.count("type") ? cols[idxMap.at("type")] : "");
        std::string size = (idxMap.count("size") ? cols[idxMap.at("size")] : "");
        std::string index = (idxMap.count("index") ? cols[idxMap.at("index")] : "0");
        data.key = type + "_" + size + "_" + index;
    }

    
    if (idxMap.count("bestfound")) {
        try { data.expectedObj = std::stod(cols[idxMap.at("bestfound")]); } catch (...) { data.expectedObj = 0.0; }
    }

    // Seed (optional)
    if (idxMap.count("seed")) {
        data.seed = cols[idxMap.at("seed")];
    } else data.seed = "";

    // Input Time (Time(s))
    if (idxMap.count("time")) {
        try { data.inputTime = std::stod(cols[idxMap.at("time")]); } catch (...) { data.inputTime = 0.0; }
    } else data.inputTime = 0.0;

    // Items: from itemStartIdx onward, but only those named Item_*
    if (itemStartIdx >= 0 && itemStartIdx < (int)cols.size()) {
        for (int i = itemStartIdx; i < (int)cols.size(); ++i) {
            std::string s = cols[i];
            s.erase(remove_if(s.begin(), s.end(), isspace), s.end());
            if (s == "0") data.solution.push_back(0);
            else if (s == "1") data.solution.push_back(1);
            else {
                // try parse int (0/1)
                try {
                    int v = std::stoi(s);
                    data.solution.push_back(v==0?0:1);
                } catch (...) {
                    // stop at first non-binary value
                }
            }
        }
    }

    data.valid = true;
    data.originalLine = line;
    return data;
}

// =========================================================

// =========================================================
struct VerifyResult {
    std::string status;       // Optimal, Infeasible...
    double calcObj;
    bool isFeasible;
    bool isObjMatch;
    double time;
};

VerifyResult VerifyInstance(const std::string& path, const RowData& rowData) {
    VerifyResult res;
    res.calcObj = 0.0;
    res.isFeasible = false;
    res.isObjMatch = false;
    res.status = "Error";
    res.time = 0.0;

    IloEnv env;
    try {
        
        std::map<std::string, BestKnownInfo> dummy;
        Instances instance(path, dummy);

        IloModel model(env);
        IloCplex cplex(model);

        
        cplex.setParam(IloCplex::Param::Threads, 1);
        cplex.setParam(IloCplex::Param::TimeLimit, 10); 
        cplex.setOut(env.getNullStream());
        cplex.setWarning(env.getNullStream());

        
        int numItems = instance.NumItems();
        int numForfeits = instance.NumForfiets();
        IloBoolVarArray x(env, numItems); 
        IloBoolVarArray z(env, numForfeits); 

        
        IloExpr weightExpr(env);
        for (int i = 0; i < numItems; ++i) weightExpr += x[i] * (IloInt)instance.ItemWeights()[i];
        model.add(weightExpr <= (IloInt)instance.Capacity());
        weightExpr.end();

        
        const auto& forfeits = instance.Forfiets();
        for (int k = 0; k < numForfeits; ++k) {
            model.add(z[k] >= x[forfeits[k].mItem1] + x[forfeits[k].mItem2] - 1);
        }

        
        IloExpr objExpr(env);
        for (int i = 0; i < numItems; ++i) objExpr += x[i] * (IloInt)instance.ItemValues()[i];
        for (int k = 0; k < numForfeits; ++k) objExpr -= z[k] * (IloInt)forfeits[k].mCost;
        model.add(IloMaximize(env, objExpr));
        objExpr.end();

        
        
        for (int i = 0; i < numItems; ++i) {
            int val = 0;
            if (i < (int)rowData.solution.size()) {
                val = rowData.solution[i];
            }
            x[i].setBounds(val, val);
        }

        
        auto start = std::chrono::high_resolution_clock::now();
        cplex.solve();
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> elapsed = end - start;
        res.time = elapsed.count();

        
        auto status = cplex.getStatus();
        if (status == IloAlgorithm::Optimal || status == IloAlgorithm::Feasible) {
            res.status = (status == IloAlgorithm::Optimal) ? "Optimal(Fixed)" : "Feasible(Fixed)";
            res.isFeasible = true;
            res.calcObj = cplex.getObjValue();

            
            if (std::abs(res.calcObj - rowData.expectedObj) < 1e-4) {
                res.isObjMatch = true;
            }
        } else if (status == IloAlgorithm::Infeasible) {
            res.status = "Infeasible";
            res.isFeasible = false;
        } else {
            res.status = "Unknown";
        }

    } catch (IloException& e) {
        res.status = "CplexError";
        std::cerr << "CPLEX Error: " << e.getMessage() << std::endl;
    } catch (...) {
        res.status = "Error";
    }

    env.end();
    return res;
}

// =========================================================

// =========================================================
int main(int argc, char* argv[]) {
    std::string instanceDir = "";
    std::string csvPath = "";

    
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--dir" && i + 1 < argc) instanceDir = argv[++i];
        else if (arg == "--csv" && i + 1 < argc) csvPath = argv[++i];
    }

    if (instanceDir.empty()) {
        std::cerr << "Usage: ./CplexBatchVerifier --dir <instances_directory> [--csv <results.csv_or_results_dir>]\n";
        return 1;
    }

    
    auto fileMap = IndexInstanceFiles(instanceDir);
    if (fileMap.empty()) {
        std::cerr << "No instances found in directory.\n";
        return 1;
    }

    
    std::string resultsDir;
    if (csvPath.empty()) resultsDir = "Final_Results";
    else {
        if (fs::is_directory(csvPath)) resultsDir = csvPath;
        else resultsDir = ""; // single file path will be used
    }

    std::vector<std::string> filesToProcess;
    if (!resultsDir.empty()) {
        for (const auto& entry : fs::directory_iterator(resultsDir)) {
            if (entry.is_regular_file()) {
                auto p = entry.path();
                if (p.extension() == ".csv") filesToProcess.push_back(p.string());
            }
        }
    } else {
        // single file provided
        filesToProcess.push_back(csvPath);
    }

    if (filesToProcess.empty()) {
        std::cerr << "No CSV files found in results location.\n";
        return 1;
    }

    
    std::string verifiedFolder = "Results_Verified";
    try { fs::create_directories(verifiedFolder); } catch(...) {}

    std::cout << ">>> Start processing " << filesToProcess.size() << " CSV files..." << std::endl;

    int fileIdx = 0;
    for (const auto& inputCsv : filesToProcess) {
        fileIdx++;
        std::ifstream inFile(inputCsv);
        if (!inFile.is_open()) {
            std::cerr << "Cannot open CSV file: " << inputCsv << std::endl;
            continue;
        }

        
        std::string fname = fs::path(inputCsv).filename().string();
        std::string rest = fname;
        const std::string prefixToRemove = "All_Best_Found_Results_";
        if (rest.rfind(prefixToRemove, 0) == 0) rest = rest.substr(prefixToRemove.size());
        std::string reportName = std::string("Verification_Report_") + rest;
        std::string outPath = (fs::path(verifiedFolder) / reportName).string();

        std::ofstream outFile(outPath);
        outFile << "Key,Seed,Status,InputObj,CalculatedObj,IsFeasible,ObjMatch,Time(s),InstancePath\n";

        std::string headerLine;
        if (!std::getline(inFile, headerLine)) {
            std::cerr << "Empty CSV or cannot read header: " << inputCsv << std::endl;
            outFile.close();
            continue;
        }

        
        if (!headerLine.empty() && headerLine.back() == '\r') {
            headerLine.pop_back();
        }
        if (headerLine.size() >= 3 && headerLine.substr(0, 3) == "\xEF\xBB\xBF") {
            headerLine = headerLine.substr(3);
        }

        
        auto headers = PU::splitString(headerLine, ',');
        std::map<std::string,int> idxMap; // keys: type,size,index,seed,bestfound,time
        int itemStartIdx = -1;
        for (size_t i = 0; i < headers.size(); ++i) {
            std::string h = headers[i];
            std::string lh = h;
            
            
            lh.erase(std::remove(lh.begin(), lh.end(), '\"'), lh.end());
            std::transform(lh.begin(), lh.end(), lh.begin(), ::tolower);

            if (lh == "type") idxMap["type"] = i;
            else if (lh == "size") idxMap["size"] = i;
            else if (lh == "index") idxMap["index"] = i;
            else if (lh.find("seed") != std::string::npos) idxMap["seed"] = i;
            else if (lh.find("bestfound") != std::string::npos || lh.find("bestfoundobj") != std::string::npos) idxMap["bestfound"] = i;
            else if (lh.find("time") != std::string::npos) idxMap["time"] = i;
            else if (lh == "key" || lh == "instance" || lh == "instancepath" || lh == "instance_path" || lh == "instancefile" || lh == "file" || lh == "filename" || lh == "path") idxMap["fullpath"] = i;

            if (lh.find("item_0") != std::string::npos) {
                itemStartIdx = (int)i;
            }
        }

        if (itemStartIdx == -1) {
            for (size_t i = 0; i < headers.size(); ++i) {
                std::string lh = headers[i];
                std::transform(lh.begin(), lh.end(), lh.begin(), ::tolower);
                if (lh.rfind("item_", 0) == 0) { itemStartIdx = (int)i; break; }
            }
        }

        std::string line;
        int lineCount = 0;
        int okCount = 0;
        int notFoundCount = 0;
        int failCount = 0;

        while (std::getline(inFile, line)) {
            if (!line.empty() && line.back() == '\r') line.pop_back();
            if (line.empty()) continue;

            RowData row = ParseCsvLineByIndices(line, idxMap, itemStartIdx);
            if (!row.valid) continue;

            lineCount++;

            
            std::string resolvedKey = row.key;
            if (resolvedKey.empty() || resolvedKey.front() == '_') {
                
                
                if (idxMap.count("size") && idxMap.count("index")) {
                    
                    auto cols = PU::splitString(row.originalLine, ',');
                    std::string sizeVal = cols[idxMap.at("size")];
                    std::string idxVal = cols[idxMap.at("index")];
                    
                    std::string suffix = std::string("_") + sizeVal + std::string("_") + idxVal;
                    for (const auto &kv : fileMap) {
                        if (kv.first.size() >= suffix.size() && kv.first.compare(kv.first.size() - suffix.size(), suffix.size(), suffix) == 0) {
                            resolvedKey = kv.first;
                            break;
                        }
                    }
                }
            }

            if (fileMap.find(resolvedKey) == fileMap.end()) {
                
                notFoundCount++;
                std::cout << "[" << fileIdx << "/" << filesToProcess.size() << "](" << lineCount << ") Verifying: " << row.key << " ... Instance File Not Found!" << std::endl;
                outFile << row.key << "," << row.seed << ",NOT_FOUND," << row.expectedObj << ",," << "False,False," << row.inputTime << "," << "" << "\n";
                continue;
            }

            std::string path = fileMap[resolvedKey];
            VerifyResult res = VerifyInstance(path, row);

            
            if (res.isFeasible && res.isObjMatch) {
                okCount++;
            } else {
                failCount++;
                std::cout << "[" << fileIdx << "/" << filesToProcess.size() << "](" << lineCount << ") Verifying: " << row.key << " ... FAIL (Feas=" << res.isFeasible << ", Match=" << res.isObjMatch << ")" << std::endl;
            }

            
            outFile << row.key << ","
                    << row.seed << ","
                    << res.status << ","
                    << std::fixed << std::setprecision(2) << row.expectedObj << ","
                    << res.calcObj << ","
                    << (res.isFeasible ? "True" : "False") << ","
                    << (res.isObjMatch ? "True" : "False") << ","
                    << std::setprecision(4) << row.inputTime << ","
                    << path << "\n";
            outFile.flush();
        }

        outFile.close();
        std::cout << ">>> Finished file " << fname << ": OK=" << okCount << ", FAIL=" << failCount << ", NOT_FOUND=" << notFoundCount << ", TOTAL=" << lineCount << std::endl;
    }

    std::cout << ">>> All done. Reports saved to " << verifiedFolder << std::endl;
    return 0;
}