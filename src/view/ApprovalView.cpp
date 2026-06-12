#include "ApprovalView.h"
#include <iostream>
#include <iomanip>
#include <limits>
#include <cctype>
#include <cstdio>

ApprovalView::ApprovalView(ApprovalService& approvalService, SampleService& sampleService)
    : m_approvalService(approvalService), m_sampleService(sampleService) {}

void ApprovalView::run() {
    int choice = -1;
    while (choice != 0) {
        std::cout << "\n============================================================\n";
        std::cout << "  [3] 주문 승인/거절\n";
        std::cout << "------------------------------------------------------------\n";
        std::cout << "  [1] 승인/거절 처리    [0] 위로\n";
        std::cout << "  선택 > ";

        if (!(std::cin >> choice)) {
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            choice = -1;
            continue;
        }
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

        switch (choice) {
            case 1: showList(); break;
            case 0: break;
            default: std::cout << "  잘못된 입력입니다.\n";
        }
    }
}

void ApprovalView::showList() {
    auto reserved = m_approvalService.reservedOrders();
    if (reserved.empty()) {
        std::cout << "\n  (승인 대기 중인 주문이 없습니다)\n";
        return;
    }

    std::cout << "\n  승인 대기 중인 예약 목록  (RESERVED)\n\n";
    std::cout << "  " << std::left
              << std::setw(6)  << "번호"
              << std::setw(22) << "주문번호"
              << std::setw(14) << "고객"
              << std::setw(18) << "시료"
              << "수량\n";
    std::cout << "  " << std::string(70, '-') << "\n";

    for (int i = 0; i < static_cast<int>(reserved.size()); ++i) {
        const Order* o = reserved[i];
        const Sample* s = m_sampleService.findById(o->sampleId);
        std::string sampleName = s ? s->name : o->sampleId;

        char qtyBuf[16];
        std::snprintf(qtyBuf, sizeof(qtyBuf), "%d ea", o->quantity);

        std::cout << "  [" << std::left << std::setw(3) << (i + 1) << "] "
                  << std::setw(22) << o->orderId
                  << std::setw(14) << o->customerName
                  << std::setw(18) << sampleName
                  << qtyBuf << "\n";
    }

    std::cout << "\n  처리할 번호를 선택하세요 (0=위로) > ";
    int sel;
    if (!(std::cin >> sel) || sel < 0 || sel > static_cast<int>(reserved.size())) {
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        std::cout << "  잘못된 입력입니다.\n";
        return;
    }
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

    if (sel == 0) return;
    processOrder(reserved[sel - 1]->orderId);
}

void ApprovalView::processOrder(const std::string& orderId) {
    // 재고 확인
    const Order* order = nullptr;
    for (const auto& o : m_approvalService.reservedOrders())
        if (o->orderId == orderId) { order = o; break; }
    if (!order) { std::cout << "  [오류] 주문을 찾을 수 없습니다.\n"; return; }

    const Sample* sample = m_sampleService.findById(order->sampleId);
    if (!sample) { std::cout << "  [오류] 시료를 찾을 수 없습니다.\n"; return; }

    int effStock = m_approvalService.effectiveStock(order->sampleId);
    bool sufficient = (effStock >= order->quantity);
    int shortage = sufficient ? 0 : (order->quantity - effStock);

    std::cout << "\n  재고 확인 중...\n\n";
    std::cout << "  시료       " << sample->name
              << "    현재 재고  " << effStock << " ea\n";
    std::cout << "  주문 수량  " << order->quantity << " ea";

    if (!sufficient) {
        int actualQty  = ApprovalService::calcActualProductQty(shortage, sample->yield);
        double totTime = ApprovalService::calcTotalProductionTime(actualQty, sample->avgProductionTime);
        std::cout << "    부족분  " << shortage << " ea\n\n";
        char timeBuf[32];
        std::snprintf(timeBuf, sizeof(timeBuf), "%.1f", totTime);
        std::cout << "  재고 부족.  (실생산량: " << actualQty << " ea / "
                  << timeBuf << " min)\n";
    } else {
        std::cout << "\n\n  재고 충분.  즉시 처리 가능.\n";
    }

    std::cout << "\n  [Y] 승인    [N] 거절\n  선택 > ";
    char confirm;
    std::cin >> confirm;
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    confirm = static_cast<char>(std::toupper(static_cast<unsigned char>(confirm)));

    if (confirm == 'Y') {
        try {
            auto result = m_approvalService.approve(orderId);
            std::cout << "\n  승인 완료.\n";
            if (result.decision == ApprovalService::ApprovalResult::Decision::CONFIRMED)
                std::cout << "  상태 변경   RESERVED  →  CONFIRMED\n";
            else
                std::cout << "  상태 변경   RESERVED  →  PRODUCING\n";
            std::cout << "  주문번호    " << orderId << "\n";
        } catch (const std::exception& e) {
            std::cout << "  [오류] " << e.what() << "\n";
        }
    } else if (confirm == 'N') {
        try {
            m_approvalService.reject(orderId);
            std::cout << "\n  거절 처리 완료.\n";
            std::cout << "  상태 변경   RESERVED  →  REJECTED\n";
            std::cout << "  주문번호    " << orderId << "\n";
        } catch (const std::exception& e) {
            std::cout << "  [오류] " << e.what() << "\n";
        }
    } else {
        std::cout << "  취소되었습니다.\n";
    }
}
