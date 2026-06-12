#pragma once
#include "../service/ProductionService.h"

class ProductionView {
public:
    explicit ProductionView(ProductionService& service);
    void run();

private:
    void showQueue();
    void completeProduction();

    ProductionService& m_service;
};
