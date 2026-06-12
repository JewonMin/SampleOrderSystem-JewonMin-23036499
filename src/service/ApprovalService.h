#pragma once
#include <string>
#include <vector>
#include <stdexcept>
#include "../repository/JsonRepository.h"

class ApprovalService {
public:
    struct ApprovalResult {
        enum class Decision { CONFIRMED, PRODUCING };
        Decision decision;
        int      stockAtApproval;      // 승인 시점 유효 재고
        int      shortage;             // 부족분 (PRODUCING일 때만 >0)
        int      actualProductQty;     // 실생산량
        double   totalProductionTime;  // 총 생산시간 (min)
    };

    explicit ApprovalService(JsonRepository& repo);

    std::vector<const Order*> reservedOrders() const;
    ApprovalResult            approve(const std::string& orderId);
    void                      reject(const std::string& orderId);

    // 유효 재고: sample.stock + 진행 중인 생산 기여분(경과시간 비례)
    int effectiveStock(const std::string& sampleId) const;

    static int    calcActualProductQty(int shortage, double yield);
    static double calcTotalProductionTime(int actualQty, double avgProductionTime);

private:
    JsonRepository& m_repo;

    Order*  findOrder(const std::string& orderId);
    Sample* findSample(const std::string& sampleId);
};
