// Utils.cpp

#include "ParamsUtils.h"

namespace fs = std::filesystem;

namespace PU {

    

    
    void LockFile(int fd) {
#ifdef _WIN32
        
        
        _lseek(fd, 0, SEEK_SET);
        _locking(fd, _LK_LOCK, 1);
        _lseek(fd, 0, SEEK_END);
#else
        
        flock(fd, LOCK_EX);
#endif
    }

    
    void UnlockFile(int fd) {
#ifdef _WIN32
        _lseek(fd, 0, SEEK_SET);
        _locking(fd, _LK_UNLCK, 1);
#else
        flock(fd, LOCK_UN);
#endif
    }

    
    long long GetFileSize(int fd) {
#ifdef _WIN32
        return _filelength(fd);
#else
        struct stat st;
        if (fstat(fd, &st) == 0) return st.st_size;
        return 0;
#endif
    }

    
    void WriteToFd(int fd, const std::string& content) {
#ifdef _WIN32
        _write(fd, content.c_str(), static_cast<unsigned int>(content.size()));
#else
        write(fd, content.c_str(), content.size());
#endif
    }

    
    void CloseFd(int fd) {
#ifdef _WIN32
        _close(fd);
#else
        close(fd);
#endif
    }

    // ----------------------------------------------------------------------
    
    // ----------------------------------------------------------------------
    std::vector<std::string> splitString(const std::string& s, char delimiter) {
        std::vector<std::string> tokens;
        std::string token;
        std::istringstream tokenStream(s);

        while (std::getline(tokenStream, token, delimiter)) {
            token.erase(std::remove_if(token.begin(), token.end(), ::isspace), token.end());
            if (!token.empty()) {
                tokens.push_back(token);
            }
        }
        return tokens;
    }

    // ----------------------------------------------------------------------
    
    // ----------------------------------------------------------------------
    std::vector<int> readLineOfInts(const std::string& line, char delimiter) {
        std::vector<int> values;
        std::vector<std::string> words = splitString(line, delimiter);
        for (const std::string& word : words) {
            try {
                values.push_back(std::stoi(word));
            }
            catch (const std::exception& e) {
                std::cerr << "Error converting string to int: " << word << " in line: " << line << std::endl;
            }
        }
        return values;
    }

    // ----------------------------------------------------------------------
    
    // ----------------------------------------------------------------------
    unsigned int generateRandomSeed() {
        return static_cast<unsigned int>(std::chrono::high_resolution_clock::now().time_since_epoch().count());
    }

    // ----------------------------------------------------------------------
    
    // ----------------------------------------------------------------------
    std::vector<std::string> findInstanceFiles(const std::string& directoryPath) {
        std::vector<std::string> txtFiles;

        try {
            
            for (const auto& entry : fs::recursive_directory_iterator(directoryPath)) {
                if (entry.is_regular_file() && entry.path().extension() == ".txt") {
                    txtFiles.push_back(entry.path().string());
                }
            }
        }
        catch (const std::exception& e) {
            std::cerr << "Error scanning directory " << directoryPath << ": " << e.what() << std::endl;
        }

        return txtFiles;
    }

    // ----------------------------------------------------------------------
    
    // ----------------------------------------------------------------------
    std::vector<std::string> getFilesToProcess(const ExperimentParams& params) {

        std::vector<std::string> filesToProcess;
        const std::string& path = params.InstancePath;

        
        if (!fs::exists(path)) {
            std::cerr << "Error: Path does not exist: " << path << std::endl;
            return filesToProcess;
        }

        
        if (fs::is_directory(path)) {
            //std::cout << "Detected directory input. Recursively scanning: " << path << std::endl;
            filesToProcess = findInstanceFiles(path); 
            std::cout << "Found " << filesToProcess.size() << " instance files in directory tree.\n";
        }
        
        else if (fs::is_regular_file(path)) {
            if (fs::path(path).extension() == ".txt") {
                //std::cout << "Detected single file input: " << path << std::endl;
                filesToProcess.push_back(path);
            }
            else {
                std::cerr << "Warning: File is not a .txt file, skipping: " << path << std::endl;
            }
        }
        else {
            std::cerr << "Error: Input is neither a valid file nor a directory.\n";
        }

        return filesToProcess;
    }

    
    std::string GetKeyFromPath(const std::string& filePath) {
        fs::path p(filePath);

        
        std::string filename = p.filename().string();

        
        std::string sizeStr = p.parent_path().filename().string();

        
        std::string typeStr = p.parent_path().parent_path().filename().string();

        
        
        std::string indexStr = "0";
        size_t underscorePos = filename.find('_');
        if (underscorePos != std::string::npos) {
            std::string numPart = filename.substr(0, underscorePos);
            try {
                int idx = std::stoi(numPart);
                indexStr = std::to_string(idx); 
            }
            catch (...) {}
        }

        
        return typeStr + "_" + sizeStr + "_" + indexStr;
    }

    std::map<std::string, BestKnownInfo> LoadBestKnownResults(const std::string& csvFilePath) {

        std::map<std::string, BestKnownInfo> results; 
        std::ifstream file(csvFilePath);

        if (!file.is_open()) {
            std::cerr << "Warning: Cannot open BKR CSV: " << csvFilePath << std::endl;
            return results;
        }

        std::string line;
        
        std::getline(file, line);

        while (std::getline(file, line)) {
            
            auto parts = splitString(line, ',');

            if (parts.size() >= 4) {
                try {
                    BestKnownInfo info;

                    
                    info.Type = parts[0];
                    info.Size = std::stoi(parts[1]);
                    info.Index = std::stoi(parts[2]);
                    info.Value = std::stoi(parts[3]);

                    
                    std::string key = info.Type + "_" + std::to_string(info.Size) + "_" + std::to_string(info.Index);

                    
                    results[key] = info;

                }
                catch (...) {
                    continue; 
                }
            }
        }
        return results;
    }


    // ----------------------------------------------------------------------
    
    // ----------------------------------------------------------------------
    std::string GetCurrentTimestamp() {
        auto now = std::chrono::system_clock::now();
        std::time_t now_c = std::chrono::system_clock::to_time_t(now);
        std::tm tm_struct;
#ifdef _WIN32
        if (localtime_s(&tm_struct, &now_c) != 0) {
            return "TimestampError";
        }
#else
        if (localtime_r(&now_c, &tm_struct) == nullptr) {
            return "TimestampError";
        }
#endif

        std::stringstream ss;
        ss << std::put_time(&tm_struct, "%Y%m%d_%H%M%S");
        return ss.str();
    }

    // ----------------------------------------------------------------------
    
    // ----------------------------------------------------------------------
    std::string CreateResultsFolder(const std::string& baseResultsPath) {

        std::string timestamp = GetCurrentTimestamp();
        fs::path fullPath = fs::path(baseResultsPath) / timestamp;

        try {
            if (fs::create_directories(fullPath)) {
                //std::cout << "Created results directory: " << fullPath.string() << "\n";
            }
        }
        catch (const fs::filesystem_error& e) {
            std::cerr << "Error creating directory: " << e.what() << "\n";
            throw std::runtime_error("Failed to create results folder.");
        }

        return fullPath.string();
    }

    // ----------------------------------------------------------------------
    
    // ----------------------------------------------------------------------
    void SaveParamsToFile(const ExperimentParams& params, const std::string& filename) {

        fs::path filePath = fs::path(params.ResultsFolderName) / filename;
        std::ofstream outfile(filePath.string());

        if (!outfile.is_open()) {
            std::cerr << "Warning: Could not open " << filePath.string() << " for writing configuration.\n";
            return;
        }

        outfile << "--- Experiment Configuration ---\n";
        outfile << "Parameter=Value\n";
        outfile << "------------------------------\n";

        
        outfile << "InstancePath=" << params.InstancePath << "\n";
        outfile << "SeedInput=" << params.SeedInput << "\n";
        outfile << "PopulationSize=" << params.PopulationSize << "\n";
        outfile << "K_RefSolutions=" << params.K << "\n";
        outfile << "MaxIterations=" << params.MaxIterations << "\n";
        outfile << "MaxBinVar=" << params.MaxBinVar << "\n";
        outfile << "MaxStagnation=" << params.MaxStag << "\n";
        outfile << "InitialCplexTimeLimitMs=" << params.InitialCplexTimeLimitMs << "\n";
        outfile << "TotalTimeLimitS=" << params.TotalTimeLimitS << "\n";

        outfile.close();
        //std::cout << "Configuration saved to " << filePath.string() << "\n";
    }



    // ----------------------------------------------------------------------
    
    // ----------------------------------------------------------------------
    void SaveRecordsToCSV(const std::string& folder,
        const std::string& instanceName,
        const std::vector<BestSolutionRecord>& records,
        const std::string& terminationReason) {

        if (records.empty()) return;

        
        std::string filePath = folder + "/" + instanceName + ".csv";

        
        
        std::ofstream outFile(filePath, std::ios::app);

        if (!outFile.is_open()) {
            std::cerr << "Error: Could not open or create file " << filePath << std::endl;
            return;
        }

        
        
        outFile.seekp(0, std::ios::end);
        if (outFile.tellp() == 0) {
            outFile << "Objective,Iteration,Time(s),AvailableCapacity";
            
            size_t numItems = records[0].itemStatuses.size();
            for (size_t i = 0; i < numItems; ++i) {
                outFile << ",Item_" << i;
            }
            outFile << "\n";
        }

        
        for (const auto& rec : records) {
            outFile << rec.obj << ","
                << rec.iteration << ","
                << rec.time << ","
                << rec.availableCap;

            
            for (const auto& status : rec.itemStatuses) {
                outFile << "," << static_cast<int>(status);
            }
            outFile << "\n";
        }

        outFile << "Termination reason: " << terminationReason << "\n";

        outFile.close();
        //std::cout << "Successfully saved " << records.size() << " records to " << filePath << std::endl;
    }

    

    void LogExperimentDetail(const std::string& resultsFolder,
        const BestKnownInfo& info,
        const BestSolutionRecord& bestSol) {

        fs::path folderPath(resultsFolder);
        
        if (!fs::exists(folderPath)) {
            fs::create_directories(folderPath);
        }

        fs::path filePath = folderPath / "All_Best_Found_Results.csv";
        std::string fullPath = filePath.string();

        int fd = -1;

        
#ifdef _WIN32
        
        
        
        errno_t err = _sopen_s(&fd, fullPath.c_str(),
            _O_RDWR | _O_CREAT | _O_APPEND | _O_TEXT,
            _SH_DENYNO,
            _S_IREAD | _S_IWRITE);
        if (err != 0) fd = -1;
#else
        // Linux
        fd = open(fullPath.c_str(), O_RDWR | O_CREAT | O_APPEND, 0666);
#endif

        if (fd == -1) {
            std::cerr << "Error: Could not open file " << fullPath << "\n";
            return;
        }

        
        LockFile(fd);

        
        
        std::ostringstream buffer;

        
        if (GetFileSize(fd) == 0) {
            buffer << "Type,Size,Index,BestKnownObj,BestFoundObj,IterationFound,Time(s)";
            for (int i = 0; i < 1000; ++i) buffer << ",Item_" << i;
            buffer << "\n";
        }

        
        buffer << info.Type << "," << info.Size << "," << info.Index << ","
            << info.Value << "," << bestSol.obj << "," << bestSol.iteration << ","
            << bestSol.time;

        for (const auto& status : bestSol.itemStatuses) {
            buffer << "," << (status == ItemStatus::Using ? 1 : 0);
        }
        buffer << "\n";

        
        WriteToFd(fd, buffer.str());

        
        UnlockFile(fd);
        CloseFd(fd);
    }


    // ----------------------------------------------------------------------
    
    // ----------------------------------------------------------------------
    ExperimentParams parseArgsWithCxxopts(int argc, char* argv[]) {

        cxxopts::Options options("KPF_Solver", "Fixed Set Search for Knapsack Problem with Forfeits (KPF)");

        try {
            options.add_options()
                ("h,help", "Print usage")

                ("pop_size", "Population size for MFSS",
                    cxxopts::value<int>()->default_value(std::to_string(DEFAULT_POPULATION_SIZE)))

                ("seed", "Random seed (-1 for random time seed)",
                    cxxopts::value<int>()->default_value(std::to_string(DEFAULT_FIXED_SEED_VALUE)))

                ("path", "Instance file or directory path",
                    cxxopts::value<std::string>()->default_value(DEFAULT_INSTANCE_PATH))

                ("rcl_size", "RCL size for constructing random solutions",
                    cxxopts::value<int>()->default_value(std::to_string(DEFAULT_RCL_SIZE)))

                ("k_ref", "Number of reference solutions (K) for Fix Set learning",
                    cxxopts::value<int>()->default_value(std::to_string(DEFAULT_K_REF_SOL)))

                ("max_iter", "Maximum number of solutions to generate (MaxIterations)",
                    cxxopts::value<int>()->default_value(std::to_string(DEFAULT_MAX_ITERATIONS)))

                ("max_bin_var", "Max binary variables allowed for CPLEX subproblem",
                    cxxopts::value<int>()->default_value(std::to_string(DEFAULT_MAX_BIN_VAR)))

                ("max_stag", "Max stagnation iterations before increasing CPLEX time limit",
                    cxxopts::value<int>()->default_value(std::to_string(DEFAULT_MAX_STAGNATION)))

                ("init_cpx_time", "Initial CPLEX time limit per subproblem (milliseconds)",
                    cxxopts::value<int>()->default_value(std::to_string(DEFAULT_CPLEX_INIT_TIME_MS)))

                ("time_limit", "Total algorithm running time limit (seconds)",
                    cxxopts::value<int>()->default_value(std::to_string(DEFAULT_TOTAL_TIME_LIMIT_S)))

                ("cpx_max_time", "Hard upper limit for CPLEX subproblem time (seconds)",
                    cxxopts::value<double>()->default_value(std::to_string(DEFAULT_MAX_CPLEX_SUB_TIME_S)))

                ("cpx_multiplier", "Multiplier to increase CPLEX time limit on stagnation",
                    cxxopts::value<double>()->default_value(std::to_string(DEFAULT_CPLEX_TIME_MULTIPLIER)));

            auto result = options.parse(argc, argv);

            if (result.count("help")) {
                std::cout << options.help() << std::endl;
                throw std::runtime_error("Help requested.");
            }

            
            ExperimentParams params;
            params.PopulationSize = result["pop_size"].as<int>();
            params.SeedInput = result["seed"].as<int>();
            params.InstancePath = result["path"].as<std::string>();
            params.K = result["k_ref"].as<int>();
            params.MaxIterations = result["max_iter"].as<int>();
            params.MaxBinVar = result["max_bin_var"].as<int>();
            params.MaxStag = result["max_stag"].as<int>();
            params.InitialCplexTimeLimitMs = result["init_cpx_time"].as<int>();
            params.TotalTimeLimitS = result["time_limit"].as<int>();
            params.MaxCplexSubTimeS = result["cpx_max_time"].as<double>();
            params.CplexTimeMultiplier = result["cpx_multiplier"].as<double>();

            
            //params.ResultsFolderName = CreateResultsFolder(BASE_RESULTS_PATH);
            //SaveParamsToFile(params, "config_summary.txt");

            return params;

        }
        catch (const cxxopts::exceptions::exception& e) {
            std::cerr << "Error parsing arguments: " << e.what() << std::endl;
            throw;
        }
    }

}