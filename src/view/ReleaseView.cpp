#include "ReleaseView.h"
#include <iostream>
#include <iomanip>
#include <limits>
#include <cctype>
#include <cstdio>

ReleaseView::ReleaseView(ReleaseService& releaseService, SampleService& sampleService)
    : m_releaseService(releaseService), m_sampleService(sampleService) {}

void ReleaseView::run() {
    int choice = -1;
    while (choice != 0) {
        std::cout << "\n============================================================\n";
        std::cout << "  [5] 출고 처리\n";
        std::cout << "------------------------------------------------------------\n";
        std::cout << "  [1] 출고 처리    [0] 위로\n";
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

void ReleaseView::showList() {
    auto confirmed = m_releaseService.confirmedOrders();
    if (confirmed.empty()) {
        std::cout << "\n  (출고 대기 중인 주문이 없습니다)\n";
        return;
    }

    std::cout << "\n  출고 대기 목록  (CONFIRMED)\n\n";
    std::cout << "  " << std::left
              << std::setw(6)  << "번호"
              << std::setw(22) << "주문번호"
              << std::setw(14) << "고객"
              << std::setw(18) << "시료"
              << "수량\n";
    std::cout << "  " << std::string(70, '-') << "\n";

    for (int i = 0; i < static_cast<int>(confirmed.size()); ++i) {
        const Order* o = confirmed[i];
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

    std::cout << "\n  출고 처리할 번호를 선택하세요 (0=위로) > ";
    int sel;
    if (!(std::cin >> sel) || sel < 0 || sel > static_cast<int>(confirmed.size())) {
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        std::cout << "  잘못된 입력입니다.\n";
        return;
    }
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    if (sel == 0) return;

    const Order* target = confirmed[sel - 1];
    const Sample* s = m_sampleService.findById(target->sampleId);

    std::cout << "\n  주문번호  " << target->orderId << "\n";
    std::cout << "  고객      " << target->customerName << "\n";
    std::cout << "  시료      " << (s ? s->name : target->sampleId) << "\n";
    std::cout << "  수량      " << target->quantity << " ea\n\n";
    std::cout << "  출고 처리하시겠습니까? [Y/N] > ";

    char confirm;
    std::cin >> confirm;
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    if (std::toupper(static_cast<unsigned char>(confirm)) != 'Y') {
        std::cout << "  취소되었습니다.\n";
        return;
    }

    try {
        m_releaseService.release(target->orderId);
        std::cout << "\n  출고 완료.\n";
        std::cout << "  상태 변경   CONFIRMED  →  RELEASED\n";
        std::cout << "  주문번호    " << target->orderId << "\n";
    } catch (const std::exception& e) {
        std::cout << "  [오류] " << e.what() << "\n";
    }
}
