#include "ReleaseService.h"
#include <stdexcept>
#include <algorithm>

ReleaseService::ReleaseService(JsonRepository& repo) : m_repo(repo) {}

std::vector<const Order*> ReleaseService::confirmedOrders() const {
    std::vector<const Order*> result;
    for (const auto& o : m_repo.orders())
        if (o.status == OrderStatus::CONFIRMED)
            result.push_back(&o);
    return result;
}

void ReleaseService::release(const std::string& orderId) {
    auto& orders = m_repo.orders();
    auto it = std::find_if(orders.begin(), orders.end(),
        [&](const Order& o) { return o.orderId == orderId; });

    if (it == orders.end())
        throw std::invalid_argument("Order not found: " + orderId);
    if (it->status != OrderStatus::CONFIRMED)
        throw std::logic_error("Order is not in CONFIRMED state: " + orderId);

    it->status = OrderStatus::RELEASED;
}
