#include "Instances.h"

Instances::Instances(const std::string& FileName, const std::map<std::string, BestKnownInfo> bestKnownMap)
    : mNumItems(0), mNumForfiets(0), mCapacity(0), mOptimal(0)
{
    
    
    if (!Load(FileName, bestKnownMap)) {
        throw std::runtime_error("Instances construction failed: Unable to load data from file: " + FileName);
    }
}

bool Instances::Load(const std::string& FileName, const std::map<std::string, BestKnownInfo>& bestKnownMap) {

    std::ifstream file(FileName);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open file " << FileName << std::endl;
        return false;
    }

    
    size_t lastSlash = FileName.find_last_of("/\\");
    size_t lastDot = FileName.find_last_of(".");
    std::string name = (lastSlash == std::string::npos) ? FileName : FileName.substr(lastSlash + 1);
    if (lastDot != std::string::npos && lastDot > lastSlash) {
        mInstanceName = name.substr(0, name.find_last_of("."));
    }
    else {
        mInstanceName = name;
    }

    
    
    mInstanceKey = PU::GetKeyFromPath(FileName);

    
    auto it = bestKnownMap.find(mInstanceKey); 
    if (it != bestKnownMap.end()) {
        
        

        const BestKnownInfo& info = it->second;
        mOptimal = info.Value;

        
        // std::cout << "Loaded BKR: " << info.Type << " " << info.Size << " (Val=" << mOptimal << ")\n";

    }
    else {
        mOptimal = 2147483647; 
        // std::cout << "Warning: Best Known NOT found for key: " << key << "\n";
    }

    std::string line;
    std::vector<std::string> words;
    int lineCount = 0;

    try {
        // --- 1. StartInfo: NumItems NumForfiets Capacity (N F C) ---
        if (!std::getline(file, line)) throw std::runtime_error("Missing StartInfo line.");
        words = PU::splitString(line, ' ');
        if (words.size() < 3) throw std::runtime_error("Missing StartInfo fields.");

        mNumItems = std::stoi(words[0]);
        mNumForfiets = std::stoi(words[1]);
        mCapacity = std::stoi(words[2]);
        lineCount++;

        // --- 2. Profits: ItemValues (p_1 p_2 ... p_N) ---
        if (!std::getline(file, line)) throw std::runtime_error("Missing Profits line.");
        mItemValues = PU::readLineOfInts(line, ' ');
        if (mItemValues.size() != mNumItems) throw std::runtime_error("ItemValues size mismatch.");
        lineCount++;

        // --- 3. Weights: ItemWeights (w_1 w_2 ... w_N) ---
        if (!std::getline(file, line)) throw std::runtime_error("Missing Weights line.");
        mItemWeights = PU::readLineOfInts(line, ' ');
        if (mItemWeights.size() != mNumItems) throw std::runtime_error("ItemWeights size mismatch.");
        lineCount++;

        // --- 4. Forfiets: Cost and Items (F times) ---
        mForfiets.clear();
        mForfiets.reserve(mNumForfiets);
        for (int i = 0; i < mNumForfiets; ++i) {
            Forfeit tempForfeit;

            // a. Cost: temp1 forfeit_cost temp2
            if (!std::getline(file, line)) throw std::runtime_error("Missing Forfeit Cost line.");
            words = PU::splitString(line, ' ');
            if (words.size() < 2) throw std::runtime_error("Missing Forfeit Cost value.");
            tempForfeit.mCost = std::stoi(words[1]);
            lineCount++;

            // b. Items: item1 item2
            if (!std::getline(file, line)) throw std::runtime_error("Missing Forfeit Items line.");
            words = PU::splitString(line, ' ');
            if (words.size() < 2) throw std::runtime_error("Missing Forfeit Items values.");
            tempForfeit.mItem1 = std::stoi(words[0]);
            tempForfeit.mItem2 = std::stoi(words[1]);
            lineCount++;

            mForfiets.push_back(tempForfeit);
        }

    }
    catch (const std::exception& e) {
        std::cerr << "File reading error at line " << lineCount + 1 << ": " << e.what() << std::endl;
        file.close();
        return false;
    }

    file.close();
    return true;
}