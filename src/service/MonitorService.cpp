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
            case OrderStatus::REJECTED:  break;  // 모니터링 제외
        }
    }
    return s;
}

std::vector<MonitorService::StockInfo> MonitorService::stockStatus() const {
    std::vector<StockInfo> result;
    for (const auto& sample : m_repo.samples()) {
        int inProd   = 0;
        int demandQty = 0;

        for (const auto& entry : m_repo.productionQueue())
            if (entry.sampleId == sample.id)
                inProd += entry.actualProductQty;

        // 부족 판단: RESERVED + PRODUCING 미처리 주문 수량 합
        for (const auto& o : m_repo.orders()) {
            if (o.sampleId == sample.id &&
                (o.status == OrderStatus::RESERVED || o.status == OrderStatus::PRODUCING))
                demandQty += o.quantity;
        }

        StockStatus st;
        if (sample.stock == 0)             st = StockStatus::DEPLETED;
        else if (sample.stock < demandQty) st = StockStatus::SHORTAGE;
        else                               st = StockStatus::SUFFICIENT;

        result.push_back({sample.id, sample.name, sample.stock, inProd, demandQty, st});
    }
    return result;
}
