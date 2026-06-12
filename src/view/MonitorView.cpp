#include "MonitorView.h"
#include <iostream>
#include <iomanip>
#include <limits>
#include <cstdio>
#include <string>

static std::string progressBar(double pct) {
    constexpr int BAR = 16;
    int filled = static_cast<int>(pct / 100.0 * BAR + 0.5);
    if (filled > BAR) filled = BAR;
    return "[" + std::string(filled, '#') + std::string(BAR - filled, '-') + "]";
}

MonitorView::MonitorView(MonitorService& monitorService,
                         ProductionService& productionService)
    : m_monitorService(monitorService)
    , m_productionService(productionService) {}

void MonitorView::run() {
    int choice = -1;
    while (choice != 0) {
        std::cout << "\n============================================================\n";
        std::cout << "  [6] 모니터링\n";
        std::cout << "------------------------------------------------------------\n";
        std::cout << "  [1] 대시보드 조회    [0] 위로\n";
        std::cout << "  선택 > ";

        if (!(std::cin >> choice)) {
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            choice = -1;
            continue;
        }
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

        switch (choice) {
            case 1: showDashboard(); break;
            case 0: break;
            default: std::cout << "  잘못된 입력입니다.\n";
        }
    }
}

void MonitorView::showDashboard() {
    auto summary = m_monitorService.orderSummary();
    auto stocks  = m_monitorService.stockStatus();
    auto prods   = m_productionService.list();

    int total = summary.reserved + summary.producing + summary.confirmed
              + summary.released + summary.rejected;

    // ── 주문 현황 ──
    std::cout << "\n============================================================\n";
    std::cout << "  주문 현황\n";
    std::cout << "------------------------------------------------------------\n";
    std::cout << "  전체: " << total << "건\n\n";
    std::cout << "  " << std::left
              << std::setw(14) << "RESERVED"
              << std::setw(14) << "PRODUCING"
              << std::setw(14) << "CONFIRMED"
              << std::setw(14) << "RELEASED"
              << "REJECTED\n";
    std::cout << "  "
              << std::setw(14) << summary.reserved
              << std::setw(14) << summary.producing
              << std::setw(14) << summary.confirmed
              << std::setw(14) << summary.released
              << summary.rejected << "\n";

    // ── 시료 재고 현황 ──
    std::cout << "\n------------------------------------------------------------\n";
    std::cout << "  시료 재고 현황\n";
    std::cout << "------------------------------------------------------------\n";
    if (stocks.empty()) {
        std::cout << "  (등록된 시료 없음)\n";
    } else {
        std::cout << "  " << std::left
                  << std::setw(10) << "시료ID"
                  << std::setw(20) << "시료명"
                  << std::setw(12) << "현재재고"
                  << "생산중\n";
        std::cout << "  " << std::string(54, '-') << "\n";
        for (const auto& s : stocks) {
            char inProd[16] = "-";
            if (s.inProductionQty > 0)
                std::snprintf(inProd, sizeof(inProd), "+%d ea", s.inProductionQty);
            std::cout << "  " << std::left
                      << std::setw(10) << s.id
                      << std::setw(20) << s.name
                      << std::setw(12) << (std::to_string(s.stock) + " ea")
                      << inProd << "\n";
        }
    }

    // ── 생산 현황 ──
    std::cout << "\n------------------------------------------------------------\n";
    std::cout << "  생산 현황\n";
    std::cout << "------------------------------------------------------------\n";
    if (prods.empty()) {
        std::cout << "  (진행 중인 생산 없음)\n";
    } else {
        std::cout << "  " << std::left
                  << std::setw(22) << "주문번호"
                  << std::setw(16) << "시료"
                  << std::setw(10) << "생산수량"
                  << "진행률\n";
        std::cout << "  " << std::string(70, '-') << "\n";
        for (const auto& p : prods) {
            char pct[8];
            std::snprintf(pct, sizeof(pct), "%5.1f%%", p.progressPct);
            std::cout << "  " << std::left
                      << std::setw(22) << p.orderId
                      << std::setw(16) << p.sampleName
                      << std::setw(10) << p.actualProductQty
                      << progressBar(p.progressPct) << " " << pct << "\n";
        }
    }
    std::cout << "============================================================\n";
}
