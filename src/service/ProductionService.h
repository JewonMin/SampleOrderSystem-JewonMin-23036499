#pragma once
#include <string>
#include <vector>
#include "../repository/JsonRepository.h"

class ProductionService {
public:
    struct ProductionInfo {
        std::string orderId;
        std::string sampleId;
        std::string sampleName;
        int         shortageQty;
        int         actualProductQty;
        double      totalProductionTime;  // min
        double      elapsedMin;
        double      progressPct;          // 0.0 ~ 100.0
        std::string startedAt;
    };

    explicit ProductionService(JsonRepository& repo);

    std::vector<ProductionInfo> list() const;
    void complete(const std::string& orderId);

private:
    JsonRepository& m_repo;
};
