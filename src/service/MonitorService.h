#pragma once
#include <string>
#include <vector>
#include "../repository/JsonRepository.h"

class MonitorService {
public:
    struct OrderSummary {
        int reserved  = 0;
        int producing = 0;
        int confirmed = 0;
        int released  = 0;
        int rejected  = 0;
    };

    struct StockInfo {
        std::string id;
        std::string name;
        int         stock;
        int         inProductionQty;  // 현재 생산 중인 수량 (actualProductQty 합계)
    };

    explicit MonitorService(JsonRepository& repo);

    OrderSummary           orderSummary() const;
    std::vector<StockInfo> stockStatus()  const;

private:
    JsonRepository& m_repo;
};
