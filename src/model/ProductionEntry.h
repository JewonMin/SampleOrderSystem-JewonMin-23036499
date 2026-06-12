#pragma once
#include <string>
#include <nlohmann/json.hpp>

struct ProductionEntry {
    std::string orderId;
    std::string sampleId;
    int         shortageQty;          // 부족분 (주문수량 - 승인시점재고)
    int         actualProductQty;     // 실생산량 = ceil(shortage / (yield * 0.9))
    double      totalProductionTime;  // 총 생산시간 (min)
    long long   startedAtEpoch;       // 생산 시작 Unix timestamp
    std::string startedAt;            // 생산 시작 표시 문자열 "YYYY-MM-DD HH:MM:SS"
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(ProductionEntry,
    orderId, sampleId, shortageQty, actualProductQty,
    totalProductionTime, startedAtEpoch, startedAt)
