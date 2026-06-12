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
        clearScreen();
        std::cout << boxTop();
        std::cout << boxRowA(std::string(Color::BOLD) + "  [6] 출고 처리" + Color::RESET, 15);
        std::cout << boxDiv();
        std::cout << boxRow("  [1] 출고 처리    [0] 위로");
        std::cout << boxDiv();
        std::cout << "\xe2\x95\x91 " << Color::BOLD << "  선택 ▶ " << Color::RESET << std::flush;

        if (!(std::cin >> choice)) {
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            choice = -1; continue;
        }
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

        switch (choice) {
            case 1: showList(); break;
            case 0: break;
            default:
                std::cout << boxRowA(std::string(Color::BRED) + "  잘못된 입력입니다." + Color::RESET, 14);
                std::cout << boxBot(); Sleep(600);
        }
    }
}

void ReleaseView::showList() {
    clearScreen();
    auto confirmed = m_releaseService.confirmedOrders();

    std::cout << boxTop();
    std::cout << boxRowA(
        std::string(Color::BOLD) + "  출고 가능 주문  " + Color::RESET +
        Color::BGREEN + "(CONFIRMED)" + Color::RESET, 18 + 11);

    if (confirmed.empty()) {
        std::cout << boxDiv();
        std::cout << boxEmpty();
        std::cout << boxRow(centerText("출고 대기 중인 주문이 없습니다", INNER_W));
        std::cout << boxEmpty();
        std::cout << boxBot();
        Sleep(800); return;
    }

    std::cout << boxDiv();
    std::string hdr = padRight("  번호", 6) + padRight("주문번호", 22)
                    + padRight("고객", 14) + padRight("시료", 16) + "수량";
    std::cout << boxRow(hdr);
    std::cout << boxRow("  " + sline(INNER_W - 2));

    for (int i = 0; i < static_cast<int>(confirmed.size()); ++i) {
        const Order* o = confirmed[i];
        const Sample* s = m_sampleService.findById(o->sampleId);
        char qtyBuf[16]; std::snprintf(qtyBuf, sizeof(qtyBuf), "%d ea", o->quantity);
        std::string row = padRight("  [" + std::to_string(i+1) + "]", 6)
                        + padRight(o->orderId,        22)
                        + padRight(o->customerName,   14)
                        + padRight(s ? s->name : o->sampleId, 16)
                        + qtyBuf;
        std::cout << boxRow(row);
    }

    std::cout << boxDiv();
    std::cout << "\xe2\x95\x91 " << Color::BOLD << "  처리할 번호 (0=위로) ▶ " << Color::RESET << std::flush;

    int sel;
    if (!(std::cin >> sel) || sel < 0 || sel > static_cast<int>(confirmed.size())) {
        std::cin.clear(); std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        std::cout << "\n";
        std::cout << boxRowA(std::string(Color::BRED) + "  잘못된 입력입니다." + Color::RESET, 14);
        std::cout << boxBot(); Sleep(600); return;
    }
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    if (sel == 0) { std::cout << boxBot(); return; }

    const Order* target = confirmed[sel - 1];
    const Sample* s = m_sampleService.findById(target->sampleId);

    std::cout << boxEmpty();
    std::cout << boxRow("  주문번호  " + target->orderId);
    std::cout << boxRow("  고객      " + target->customerName);
    std::cout << boxRow("  시료      " + (s ? s->name : target->sampleId));
    std::cout << boxRow("  수량      " + std::to_string(target->quantity) + " ea");
    std::cout << boxEmpty();
    std::cout << boxDiv();
    std::cout << "\xe2\x95\x91 " << Color::BOLD << "  출고 처리하시겠습니까? [Y/N] ▶ " << Color::RESET << std::flush;

    char confirm; std::cin >> confirm;
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    std::cout << "\n";

    if (std::toupper(static_cast<unsigned char>(confirm)) != 'Y') {
        std::cout << boxRow(std::string(Color::DIM) + "  취소되었습니다." + Color::RESET);
        std::cout << boxBot(); Sleep(600); return;
    }

    try {
        m_releaseService.release(target->orderId);
        std::cout << boxEmpty();
        std::cout << boxRowA(std::string(Color::BGREEN) + "  ✓ 출고 완료" + Color::RESET, 12);
        std::cout << boxEmpty();
        std::cout << boxRow("  주문번호    " + target->orderId);
        std::cout << boxRow("  출고수량    " + std::to_string(target->quantity) + " ea");
        std::cout << boxRow("  처리일시    " + currentTimeStr());
        std::cout << boxRowA(
            "  상태 변경   " + std::string(Color::BGREEN) + "CONFIRMED" + Color::RESET
            + "  →  " + std::string(Color::BBLUE) + "RELEASED" + Color::RESET,
            14 + 9 + 5 + 8);
        std::cout << boxEmpty();
    } catch (const std::exception& e) {
        std::cout << boxRowA(std::string(Color::BRED) + "  [오류] " + e.what() + Color::RESET,
            8 + (int)std::strlen(e.what()));
    }
    std::cout << boxBot();
    Sleep(1200);
}
