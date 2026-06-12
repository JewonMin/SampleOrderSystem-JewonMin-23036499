#include "MonitorService.h"

MonitorService::MonitorService(JsonRepository& repo) : m_repo(repo) {}

MonitorService::OrderSummary MonitorService::orderSummary() const {
    OrderSummary s;
    for (const auto& o : m_repo.orders()) {
        switch (o.status) {
            case OrderStatus::RESERVED:  ++s.reserved;  break;
            case OrderStatus::PRODUCING: ++s.producing; break;
            case OrderStatus::CONFIRMED: ++s.confirmed; break;
            case OrderStatus::RELEASED:  ++s.released;  break;
            case OrderStatus::REJECTED:  ++s.rejected;  break;
        }
    }
    return s;
}

std::vector<MonitorService::StockInfo> MonitorService::stockStatus() const {
    std::vector<StockInfo> result;
    for (const auto& sample : m_repo.samples()) {
        int inProd = 0;
        for (const auto& entry : m_repo.productionQueue())
            if (entry.sampleId == sample.id)
                inProd += entry.actualProductQty;

        result.push_back({sample.id, sample.name, sample.stock, inProd});
    }
    return result;
}
