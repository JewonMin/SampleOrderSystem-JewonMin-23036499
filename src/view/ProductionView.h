#pragma once
#include "../service/ProductionService.h"

class ProductionView {
public:
    explicit ProductionView(ProductionService& service);
    void run();

private:
    ProductionService& m_service;
};
