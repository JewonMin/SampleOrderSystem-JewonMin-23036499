#include "OrderService.h"
#include <ctime>
#include <cstdio>
#include <algorithm>

OrderService::OrderService(JsonRepository& repo) : m_repo(repo) {}

Order OrderService::create(const std::string& sampleId,
                            const std::string& customerName,
                            int quantity) {
    // 등록된 시료인지 확인
    bool found = false;
    for (const auto& s : m_repo.samples())
        if (s.id == sampleId) { found = true; break; }
    if (!found)
        throw std::invalid_argument("등록되지 않은 시료 ID입니다: " + sampleId);

    if (customerName.empty())
        throw std::invalid_argument("고객명이 비어 있습니다.");
    if (quantity <= 0)
        throw std::invalid_argument("주문 수량은 1 이상이어야 합니다.");

    Order o{generateOrderId(), sampleId, customerName, quantity,
            OrderStatus::RESERVED, currentTimestamp()};
    m_repo.orders().push_back(o);
    return o;
}

const std::vector<Order>& OrderService::all() const {
    return m_repo.orders();
}

std::string OrderService::generateOrderId() const {
    std::time_t now = std::time(nullptr);
    struct tm t{};
    localtime_s(&t, &now);

    char dateBuf[16];
    std::strftime(dateBuf, sizeof(dateBuf), "%Y%m%d", &t);
    std::string prefix = std::string("ORD-") + dateBuf + "-";

    int maxSeq = 0;
    for (const auto& o : m_repo.orders()) {
        if (o.orderId.rfind(prefix, 0) == 0) {
            try {
                int seq = std::stoi(o.orderId.substr(prefix.size()));
                if (seq > maxSeq) maxSeq = seq;
            } catch (...) {}
        }
    }

    char buf[32];
    std::snprintf(buf, sizeof(buf), "ORD-%s-%04d", dateBuf, maxSeq + 1);
    return buf;
}

std::string OrderService::currentTimestamp() const {
    std::time_t now = std::time(nullptr);
    struct tm t{};
    localtime_s(&t, &now);

    char buf[32];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &t);
    return buf;
}
