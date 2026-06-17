#pragma once
#include "ParamsUtils.h"

class Instances {
    
    friend class Problem;

private:
    std::string mInstanceName = "";
    std::string mInstanceKey = "";
    int mNumItems;
    int mNumForfiets;
    int mCapacity = 0;
    int mOptimal = -1;

    std::vector<int> mItemValues;
    std::vector<int> mItemWeights;
    std::vector<Forfeit> mForfiets;

public:
    
    Instances(const std::string& FileName, const std::map<std::string, BestKnownInfo> bestKnownMap);

    
    bool Load(const std::string& FileName, const std::map<std::string, BestKnownInfo>& bestKnownMap);

    // Getters
    std::string InstanceName() const { return mInstanceName; }
    std::string InstanceKey() const { return mInstanceKey; }
    int NumItems() const { return mNumItems; }
    int NumForfiets() const { return mNumForfiets; }
    int Capacity() const { return mCapacity; }
    int Optimal() const { return mOptimal; }
    const std::vector<int>& ItemValues() const { return mItemValues; }
    const std::vector<int>& ItemWeights() const { return mItemWeights; }
    const std::vector<Forfeit>& Forfiets() const { return mForfiets; }
};