#include "ReleaseView.h"
#include "ViewUtils.h"
#include <iostream>
#include <iomanip>
#include <limits>
#include <cctype>
#include <cstdio>
#include <ctime>

ReleaseView::ReleaseView(ReleaseService& releaseService, SampleService& sampleService)
    : m_releaseService(releaseService), m_sampleService(sampleService) {}

void ReleaseView::run() {
    int choice = -1;
    while (choice != 0) {
        std::cout << "\n============================================================\n";
        std::cout << Color::BOLD << "  [6] 출고 처리" << Color::RESET << "\n";
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

    std::cout << "\n  " << Color::BOLD << "출고 가능 주문  (CONFIRMED)" << Color::RESET << "\n\n";
    std::cout << "  "
              << padRight("번호", 6)
              << padRight("주문번호", 22)
              << padRight("고객", 14)
              << padRight("시료", 18)
              << "수량\n";
    std::cout << "  " << std::string(70, '-') << "\n";

    for (int i = 0; i < static_cast<int>(confirmed.size()); ++i) {
        const Order* o = confirmed[i];
        const Sample* s = m_sampleService.findById(o->sampleId);
        std::string sampleName = s ? s->name : o->sampleId;

        char qtyBuf[16];
        std::snprintf(qtyBuf, sizeof(qtyBuf), "%d ea", o->quantity);

        std::cout << "  [" << std::left << std::setw(3) << (i + 1) << "] "
                  << padRight(o->orderId,      22)
                  << padRight(o->customerName, 14)
                  << padRight(sampleName,      18)
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

    std::cout << "\n  주문번호  " << Color::CYAN << target->orderId << Color::RESET << "\n";
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
        std::cout << "\n  " << Color::BGREEN << "출고 완료." << Color::RESET << "\n";
        std::cout << "  주문번호    " << target->orderId << "\n";
        std::cout << "  출고수량    " << target->quantity << " ea\n";
        std::cout << "  처리일시    " << currentTimeStr() << "\n";
        std::cout << "  상태 변경   "
                  << Color::BGREEN  << "CONFIRMED" << Color::RESET << "  →  "
                  << Color::BLUE    << "RELEASED"  << Color::RESET << "\n";
    } catch (const std::exception& e) {
        std::cout << "  " << Color::BRED << "[오류] " << e.what() << Color::RESET << "\n";
    }
}
