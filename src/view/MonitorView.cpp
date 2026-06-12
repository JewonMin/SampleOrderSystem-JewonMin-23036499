#include "MonitorView.h"
#include "ViewUtils.h"
#include <iostream>
#include <iomanip>
#include <limits>
#include <cstdio>

MonitorView::MonitorView(MonitorService& monitorService,
                         ProductionService& productionService)
    : m_monitorService(monitorService)
    , m_productionService(productionService) {}

void MonitorView::run() {
    int choice = -1;
    while (choice != 0) {
        std::cout << "\n============================================================\n";
        std::cout << Color::BOLD << "  [4] 모니터링" << Color::RESET
                  << "    " << currentTimeStr() << "\n";
        std::cout << "------------------------------------------------------------\n";
        std::cout << "  [1] 주문량 확인    [2] 재고량 확인    [0] 위로\n";
        std::cout << "  선택 > ";

        if (!(std::cin >> choice)) {
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            choice = -1;
            continue;
        }
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

        switch (choice) {
            case 1: showOrderSummary(); break;
            case 2: showStockStatus();  break;
            case 0: break;
            default: std::cout << "  잘못된 입력입니다.\n";
        }
    }
}

void MonitorView::showOrderSummary() {
    auto s = m_monitorService.orderSummary();
    int total = s.reserved + s.producing + s.confirmed + s.released;

    std::cout << "\n  상태별 주문 현황  (총 " << total << "건)\n";
    std::cout << "  " << std::string(40, '-') << "\n";

    auto row = [](const char* label, int cnt, const char* col) {
        std::cout << "  " << col << padRight(label, 14) << Color::RESET
                  << cnt << "건\n";
    };
    row("RESERVED",  s.reserved,  Color::CYAN);
    row("PRODUCING", s.producing, Color::BYELLOW);
    row("CONFIRMED", s.confirmed, Color::BGREEN);
    row("RELEASED",  s.released,  Color::BLUE);
    std::cout << "  " << std::string(40, '-') << "\n";
}

void MonitorView::showStockStatus() {
    auto stocks = m_monitorService.stockStatus();

    std::cout << "\n  재고 현황\n";
    std::cout << "  " << std::string(68, '-') << "\n";
    std::cout << "  "
              << padRight("시료ID",   8)
              << padRight("시료명",   22)
              << padRight("현재재고", 12)
              << padRight("생산중",   10)
              << "상태\n";
    std::cout << "  " << std::string(68, '-') << "\n";

    if (stocks.empty()) {
        std::cout << "  (등록된 시료 없음)\n";
    } else {
        for (const auto& s : stocks) {
            const char* stCol  = Color::BGREEN;
            const char* stText = "여유";
            if      (s.status == MonitorService::StockStatus::DEPLETED) { stCol = Color::BRED;    stText = "고갈"; }
            else if (s.status == MonitorService::StockStatus::SHORTAGE) { stCol = Color::BYELLOW; stText = "부족"; }

            char inProd[16] = "-";
            if (s.inProductionQty > 0)
                std::snprintf(inProd, sizeof(inProd), "+%d ea", s.inProductionQty);

            std::cout << "  "
                      << padRight(s.id,   8)
                      << padRight(s.name, 22)
                      << padRight(std::to_string(s.stock) + " ea", 12)
                      << padRight(inProd, 10)
                      << stCol << stText << Color::RESET << "\n";
        }
    }
    std::cout << "  " << std::string(68, '-') << "\n";
}
