#pragma once
#include <string>
#include <vector>
#include <stdexcept>
#include "../repository/JsonRepository.h"

class OrderService {
public:
    explicit OrderService(JsonRepository& repo);

    // 주문 생성: 시료 존재 확인 + RESERVED 상태로 저장
    Order create(const std::string& sampleId,
                 const std::string& customerName,
                 int quantity);

    const std::vector<Order>& all() const;

private:
    JsonRepository& m_repo;

    std::string generateOrderId() const;
    std::string currentTimestamp() const;
};
