#pragma once
#include "../service/ApprovalService.h"
#include "../service/SampleService.h"

class ApprovalView {
public:
    ApprovalView(ApprovalService& approvalService, SampleService& sampleService);
    void run();

private:
    void showList();
    void processOrder(const std::string& orderId);

    ApprovalService& m_approvalService;
    SampleService&   m_sampleService;
};
