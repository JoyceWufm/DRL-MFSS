// Utils.h

#pragma once

#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <stdexcept>
#include <random> 
#include <chrono>
#include <filesystem>
#include <cctype> // for ::isspace
#include <iomanip> // for std::put_time
#include <numeric>
#include "cxxopts.hpp"


#ifdef _WIN32
    
#include <io.h>       // _access, _locking, _write, _close
#include <sys/locking.h>
#include <direct.h>   // _mkdir
#include <fcntl.h>    // _O_RDWR, _O_CREAT
#include <sys/stat.h> // _S_IREAD
#include <sys/types.h>
#include <share.h>    // _sopen_s
#else
    
#include <sys/file.h> // flock
#include <unistd.h>   // close, write, access
#include <fcntl.h>    // open, O_RDWR
#include <sys/stat.h> // mkdir, stat
#include <sys/types.h>
#endif


namespace fs = std::filesystem;




enum class ItemStatus {
    Using,
    NotUsing,
    Unknown
};


struct Forfeit {
    int mItem1;
    int mItem2;
    int mCost;
};


struct BestKnownInfo {
    std::string Type;
    int Size;
    int Index;
    int Value;
};


struct BestSolutionRecord {
    int obj;
    int iteration;
    double time;
    int availableCap;
    std::vector<ItemStatus> itemStatuses;
};




const std::string DEFAULT_INSTANCE_PATH = "Instances/O/";

const std::string BASE_RESULTS_PATH = "Results";




const int DEFAULT_FIXED_SEED_VALUE = -1;

const int RANDOM_SEED_FLAG = -1;

const int DEFAULT_RCL_SIZE = 10;

const int DEFAULT_POPULATION_SIZE = 200;

const int DEFAULT_K_REF_SOL = 5;

const int DEFAULT_MAX_ITERATIONS = 5000;

const int DEFAULT_MAX_BIN_VAR = 1000;

const int DEFAULT_MAX_STAGNATION = 50;

const int DEFAULT_CPLEX_INIT_TIME_MS = 100;

const int DEFAULT_TOTAL_TIME_LIMIT_S = 600; 

const double DEFAULT_MAX_CPLEX_SUB_TIME_S = 3.3;

const double DEFAULT_CPLEX_TIME_MULTIPLIER = 2.0;

struct ExperimentParams {

    
    std::string InstancePath;
    
    std::string ResultsFolderName;

    
    int PopulationSize;
    int SeedInput; 
    int RCLsize;
    int K;
    int MaxIterations;
    int MaxBinVar;
    int MaxStag;
    int InitialCplexTimeLimitMs;
    int TotalTimeLimitS;    
    double MaxCplexSubTimeS;      
    double CplexTimeMultiplier;   

    
    ExperimentParams()
        : 
        InstancePath(DEFAULT_INSTANCE_PATH),
        ResultsFolderName(""),
        PopulationSize(DEFAULT_POPULATION_SIZE),
        SeedInput(DEFAULT_FIXED_SEED_VALUE),
        RCLsize(DEFAULT_RCL_SIZE),
        K(DEFAULT_K_REF_SOL),
        MaxIterations(DEFAULT_MAX_ITERATIONS),
        MaxBinVar(DEFAULT_MAX_BIN_VAR),
        MaxStag(DEFAULT_MAX_STAGNATION),
        InitialCplexTimeLimitMs(DEFAULT_CPLEX_INIT_TIME_MS),
        TotalTimeLimitS(DEFAULT_TOTAL_TIME_LIMIT_S),
        MaxCplexSubTimeS(DEFAULT_MAX_CPLEX_SUB_TIME_S),
        CplexTimeMultiplier(DEFAULT_CPLEX_TIME_MULTIPLIER)
    {
        fs::create_directories(BASE_RESULTS_PATH);
    }
};



namespace PU {
    
    ExperimentParams parseArgsWithCxxopts(int argc, char* argv[]);

    
    std::map<std::string, BestKnownInfo> LoadBestKnownResults(const std::string& csvFilePath);

    
    std::string GetKeyFromPath(const std::string& filePath);

    
    std::vector<std::string> splitString(const std::string& s, char delimiter);

    
    std::vector<int> readLineOfInts(const std::string& line, char delimiter = ' ');

    
    unsigned int generateRandomSeed();

    
    std::vector<std::string> findInstanceFiles(const std::string& directoryPath);

    
    std::vector<std::string> getFilesToProcess(const ExperimentParams& params);

    
    std::string GetCurrentTimestamp();

    
    std::string CreateResultsFolder(const std::string& baseResultsPath);

    
    void SaveParamsToFile(const ExperimentParams& params, const std::string& filename = "config_summary.txt");

    
    void SaveRecordsToCSV(const std::string& folder,
        const std::string& instanceName,
        const std::vector<BestSolutionRecord>& records,
        const std::string& terminationReason);

    
    void LogExperimentDetail(const std::string& resultsFolder,
        const BestKnownInfo& info,
        const BestSolutionRecord& bestSol);

    // ----------------------------------------------------------------------
    // Simple Timer Class
    // ----------------------------------------------------------------------
    class Timer {
    public:
        // Start the timer automatically upon creation
        Timer() : mStart(std::chrono::high_resolution_clock::now()) {}

        // Return elapsed time in seconds
        double elapsed() const {
            
            return std::chrono::duration<double>(
                std::chrono::high_resolution_clock::now() - mStart
            ).count();
        }

        // Return elapsed time in seconds and reset the timer
        double reset() {
            const auto now = std::chrono::high_resolution_clock::now();
            const double elapsed = std::chrono::duration<double>(now - mStart).count();
            mStart = now; // Reset start point
            return elapsed;
        }

    private:
        std::chrono::time_point<std::chrono::high_resolution_clock> mStart;
    };

    
    template <class Scalar = double>
    struct plus_assign_op {
        void operator()(Scalar& lhs, const Scalar& rhs) { lhs += rhs; }
    };

    

        
    class ScopedTimer {
    public:
        
        explicit ScopedTimer(double& dest) : dest_(dest) {}

        
        ~ScopedTimer() {
            
            dest_ = timer_.elapsed();
        }

    private:
        double& dest_;
        
        Timer timer_;
    };

    
    class ScopedTimerAdd {
    public:
        explicit ScopedTimerAdd(double& dest) : dest_(dest) {}

        
        ~ScopedTimerAdd() {
            dest_ += timer_.elapsed();
        }

    private:
        double& dest_;
        
        Timer timer_;
    };
}