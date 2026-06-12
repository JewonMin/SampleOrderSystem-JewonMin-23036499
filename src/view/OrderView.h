#pragma once
#include "../service/OrderService.h"
#include "../service/SampleService.h"

class OrderView {
public:
    OrderView(OrderService& orderService, SampleService& sampleService);
    void run();

private:
    void showCreate();

    OrderService&  m_orderService;
    SampleService& m_sampleService;
};
