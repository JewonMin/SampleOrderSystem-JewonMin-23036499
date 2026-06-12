#include "ProductionService.h"
#include <ctime>
#include <stdexcept>
#include <algorithm>

ProductionService::ProductionService(JsonRepository& repo) : m_repo(repo) {}

std::vector<ProductionService::ProductionInfo> ProductionService::list() const {
    std::vector<ProductionInfo> result;
    const long long now = static_cast<long long>(std::time(nullptr));

    for (const auto& entry : m_repo.productionQueue()) {
        ProductionInfo info;
        info.orderId             = entry.orderId;
        info.sampleId            = entry.sampleId;
        info.shortageQty         = entry.shortageQty;
        info.actualProductQty    = entry.actualProductQty;
        info.totalProductionTime = entry.totalProductionTime;
        info.startedAt           = entry.startedAt;

        const Sample* s = nullptr;
        for (const auto& sm : m_repo.samples())
            if (sm.id == entry.sampleId) { s = &sm; break; }
        info.sampleName = s ? s->name : entry.sampleId;

        info.elapsedMin = static_cast<double>(now - entry.startedAtEpoch) / 60.0;
        if (entry.totalProductionTime > 0.0) {
            info.progressPct = std::min(100.0,
                (info.elapsedMin / entry.totalProductionTime) * 100.0);
        } else {
            info.progressPct = 100.0;
        }

        result.push_back(std::move(info));
    }
    return result;
}

void ProductionService::complete(const std::string& orderId) {
    auto& queue = m_repo.productionQueue();
    auto entryIt = std::find_if(queue.begin(), queue.end(),
        [&](const ProductionEntry& e) { return e.orderId == orderId; });
    if (entryIt == queue.end())
        throw std::invalid_argument("Production entry not found: " + orderId);

    auto& orders = m_repo.orders();
    auto orderIt = std::find_if(orders.begin(), orders.end(),
        [&](const Order& o) { return o.orderId == orderId; });
    if (orderIt == orders.end())
        throw std::invalid_argument("Order not found: " + orderId);
    if (orderIt->status != OrderStatus::PRODUCING)
        throw std::logic_error("Order is not in PRODUCING state: " + orderId);

    auto& samples = m_repo.samples();
    auto sampleIt = std::find_if(samples.begin(), samples.end(),
        [&](const Sample& s) { return s.id == entryIt->sampleId; });
    if (sampleIt == samples.end())
        throw std::invalid_argument("Sample not found: " + entryIt->sampleId);

    // 생산된 수량 입고, 주문 수량 출고
    sampleIt->stock += entryIt->actualProductQty - orderIt->quantity;

    orderIt->status = OrderStatus::CONFIRMED;
    queue.erase(entryIt);
}
