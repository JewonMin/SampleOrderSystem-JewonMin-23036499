#pragma once
#include "../service/MonitorService.h"
#include "../service/ProductionService.h"

class MonitorView {
public:
    MonitorView(MonitorService& monitorService, ProductionService& productionService);
    void run();

private:
    void showOrderSummary();
    void showStockStatus();

    MonitorService&    m_monitorService;
    ProductionService& m_productionService;
};
