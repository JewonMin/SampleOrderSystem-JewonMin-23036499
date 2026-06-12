#include "ApprovalView.h"
#include "ViewUtils.h"
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
        clearScreen();
        std::cout << boxTop();
        std::cout << boxRowA(std::string(Color::BOLD) + "  [3] 주문 승인/거절" + Color::RESET, 20);
        std::cout << boxDiv();
        std::cout << boxRow("  [1] 승인/거절 처리    [0] 위로");
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
                std::cout << "\n"; std::cout << boxRowA(std::string(Color::BRED) + "  잘못된 입력입니다." + Color::RESET, 14);
                std::cout << boxBot(); Sleep(600);
        }
    }
}

void ApprovalView::showList() {
    clearScreen();
    auto reserved = m_approvalService.reservedOrders();

    std::cout << boxTop();
    std::cout << boxRowA(
        "  " + std::string(Color::BOLD) + "승인 대기 목록  " + Color::RESET +
        Color::BCYAN + "(RESERVED)" + Color::RESET, 18 + 10);

    if (reserved.empty()) {
        std::cout << boxDiv();
        std::cout << boxEmpty();
        std::cout << boxRow(centerText("승인 대기 중인 주문이 없습니다", INNER_W));
        std::cout << boxEmpty();
        std::cout << boxBot();
        Sleep(800); return;
    }

    std::cout << boxDiv();
    std::string hdr = padRight("  번호", 6) + padRight("주문번호", 22)
                    + padRight("고객", 14) + padRight("시료", 16) + "수량";
    std::cout << boxRow(hdr);
    std::cout << boxRow("  " + sline(INNER_W - 2));

    for (int i = 0; i < (int)reserved.size(); ++i) {
        const Order* o = reserved[i];
        const Sample* s = m_sampleService.findById(o->sampleId);
        char qty[16]; std::snprintf(qty, sizeof(qty), "%d ea", o->quantity);
        std::string row = padRight("  [" + std::to_string(i+1) + "]", 6)
                        + padRight(o->orderId, 22)
                        + padRight(o->customerName, 14)
                        + padRight(s ? s->name : o->sampleId, 16)
                        + qty;
        std::cout << boxRow(row);
    }

    std::cout << boxDiv();
    std::cout << "\xe2\x95\x91 " << Color::BOLD << "  처리할 번호 (0=위로) ▶ " << Color::RESET << std::flush;

    int sel;
    if (!(std::cin >> sel) || sel < 0 || sel > (int)reserved.size()) {
        std::cin.clear(); std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        std::cout << "\n"; std::cout << boxRowA(std::string(Color::BRED) + "  잘못된 입력입니다." + Color::RESET, 14);
        std::cout << boxBot(); Sleep(600); return;
    }
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    if (sel == 0) { std::cout << boxBot(); return; }
    processOrder(reserved[sel - 1]->orderId);
}

void ApprovalView::processOrder(const std::string& orderId) {
    const Order* order = nullptr;
    for (const auto& o : m_approvalService.reservedOrders())
        if (o->orderId == orderId) { order = o; break; }
    if (!order) { std::cout << boxRowA(std::string(Color::BRED) + "  [오류] 주문을 찾을 수 없습니다." + Color::RESET, 22); std::cout << boxBot(); Sleep(800); return; }

    const Sample* sample = m_sampleService.findById(order->sampleId);
    if (!sample) { std::cout << boxRowA(std::string(Color::BRED) + "  [오류] 시료를 찾을 수 없습니다." + Color::RESET, 22); std::cout << boxBot(); Sleep(800); return; }

    int effStock   = m_approvalService.effectiveStock(order->sampleId);
    bool sufficient = (effStock >= order->quantity);
    int shortage    = sufficient ? 0 : (order->quantity - effStock);

    // 스피너
    std::cout << "\n";
    spin("재고 확인 중...", 1200);

    clearScreen();
    std::cout << boxTop();
    std::cout << boxRowA(std::string(Color::BOLD) + "  재고 확인 결과" + Color::RESET, 16);
    std::cout << boxDiv();
    std::cout << boxEmpty();
    std::cout << boxRow("  시료       " + sample->name + "    현재 재고  " + std::to_string(effStock) + " ea");
    std::cout << boxRow("  주문 수량  " + std::to_string(order->quantity) + " ea");
    std::cout << boxEmpty();

    if (!sufficient) {
        int    actualQty = ApprovalService::calcActualProductQty(shortage, sample->yield);
        double totTime   = ApprovalService::calcTotalProductionTime(actualQty, sample->avgProductionTime);
        char timeBuf[32]; std::snprintf(timeBuf, sizeof(timeBuf), "%.1f", totTime);

        std::string stockBar = progressBar(static_cast<double>(effStock) / order->quantity * 100.0, 20);
        std::cout << boxRowA(
            "  재고  " + std::string(Color::BYELLOW) + stockBar + Color::RESET
            + "  " + std::to_string(effStock) + " / " + std::to_string(order->quantity) + " ea",
            8 + 20 + 2 + (int)std::to_string(effStock).size() + 3 + (int)std::to_string(order->quantity).size() + 3);
        std::cout << boxEmpty();
        std::cout << boxRowA(
            "  " + std::string(Color::BYELLOW) + "⚠  재고 부족" + Color::RESET +
            "   부족분  " + std::to_string(shortage) + " ea"
            + "   실생산량  " + std::string(Color::BGREEN) + std::to_string(actualQty) + " ea" + Color::RESET
            + "   " + timeBuf + " min",
            14 + 3 + (int)std::to_string(shortage).size() + 3
            + 7 + (int)std::to_string(actualQty).size() + 3 + (int)std::strlen(timeBuf) + 4);
    } else {
        std::cout << boxRowA(
            "  " + std::string(Color::BGREEN) + "✓  재고 충분  즉시 처리 가능" + Color::RESET, 26);
    }

    std::cout << boxEmpty();
    std::cout << boxDiv();
    std::cout << "\xe2\x95\x91 " << Color::BOLD << "  [Y] 승인    [N] 거절    ▶ " << Color::RESET << std::flush;

    char confirm; std::cin >> confirm;
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    confirm = static_cast<char>(std::toupper(static_cast<unsigned char>(confirm)));
    std::cout << "\n";

    if (confirm == 'Y') {
        spin("처리 중...", 800);
        try {
            auto result = m_approvalService.approve(orderId);
            bool isConf = (result.decision == ApprovalService::ApprovalResult::Decision::CONFIRMED);
            std::cout << boxRowA(std::string(Color::BGREEN) + "  ✓  승인 완료." + Color::RESET, 14);
            std::cout << boxRowA(
                "  상태 변경   " + std::string(Color::BCYAN) + "RESERVED" + Color::RESET
                + "  →  " + (isConf
                    ? std::string(Color::BGREEN)  + "CONFIRMED"
                    : std::string(Color::BYELLOW) + "PRODUCING") + Color::RESET,
                14+8+5+9);
            std::cout << boxRow("  주문번호    " + orderId);
        } catch (const std::exception& e) {
            std::cout << boxRowA(std::string(Color::BRED) + "  [오류] " + e.what() + Color::RESET, 8 + (int)std::strlen(e.what()));
        }
    } else if (confirm == 'N') {
        spin("처리 중...", 600);
        try {
            m_approvalService.reject(orderId);
            std::cout << boxRow("  거절 처리 완료.");
            std::cout << boxRowA(
                "  상태 변경   " + std::string(Color::BCYAN) + "RESERVED" + Color::RESET
                + "  →  " + std::string(Color::BRED) + "REJECTED" + Color::RESET, 14+8+5+8);
            std::cout << boxRow("  주문번호    " + orderId);
        } catch (const std::exception& e) {
            std::cout << boxRowA(std::string(Color::BRED) + "  [오류] " + e.what() + Color::RESET, 8 + (int)std::strlen(e.what()));
        }
    } else {
        std::cout << boxRow("  취소되었습니다.");
    }
    std::cout << boxBot();
    Sleep(1200);
}
