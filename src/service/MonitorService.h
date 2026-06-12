#pragma once
#include <string>
#include <vector>
#include "../repository/JsonRepository.h"

class MonitorService {
public:
    // REJECTED는 모니터링 대상 제외 (order-status.md)
    struct OrderSummary {
        int reserved  = 0;
        int producing = 0;
        int confirmed = 0;
        int released  = 0;
    };

    enum class StockStatus { SUFFICIENT, SHORTAGE, DEPLETED };

    struct StockInfo {
        std::string id;
        std::string name;
        int         stock;
        int         inProductionQty;  // 현재 생산 중인 수량 (actualProductQty 합계)
        int         demandQty;        // RESERVED+PRODUCING 주문 수량 합
        StockStatus status;
    };

    explicit MonitorService(JsonRepository& repo);

    OrderSummary           orderSummary() const;
    std::vector<StockInfo> stockStatus()  const;

private:
    JsonRepository& m_repo;
};
