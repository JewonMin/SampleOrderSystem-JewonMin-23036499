#pragma once
#include <string>
#include <nlohmann/json.hpp>

struct Sample {
    std::string id;
    std::string name;
    double avgProductionTime;  // min/ea
    double yield;              // 0~1 (정상 시료 수 / 총 생산 시료 수)
    int stock;                 // ea
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Sample, id, name, avgProductionTime, yield, stock)
