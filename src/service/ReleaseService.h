#pragma once
#include <string>
#include <vector>
#include "../repository/JsonRepository.h"

class ReleaseService {
public:
    explicit ReleaseService(JsonRepository& repo);

    std::vector<const Order*> confirmedOrders() const;
    void release(const std::string& orderId);

private:
    JsonRepository& m_repo;
};
