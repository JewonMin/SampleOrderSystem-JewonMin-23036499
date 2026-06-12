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
        clearScreen();
        std::cout << boxTop();
        std::cout << boxRowA(
            std::string(Color::BOLD) + "  [4] 모니터링" + Color::RESET +
            "    " + Color::DIM + currentTimeStr() + Color::RESET, 14 + 4 + 19);
        std::cout << boxDiv();
        std::cout << boxRow("  [1] 주문량 확인    [2] 재고량 확인    [0] 위로");
        std::cout << boxDiv();
        std::cout << "\xe2\x95\x91 " << Color::BOLD << "  선택 ▶ " << Color::RESET << std::flush;

        if (!(std::cin >> choice)) {
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            choice = -1; continue;
        }
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

        switch (choice) {
            case 1: showOrderSummary(); break;
            case 2: showStockStatus();  break;
            case 0: break;
            default:
                std::cout << "\n";
                std::cout << boxRowA(std::string(Color::BRED) + "  잘못된 입력입니다." + Color::RESET, 14);
                std::cout << boxBot(); Sleep(600);
        }
    }
}

void MonitorView::showOrderSummary() {
    clearScreen();
    auto s = m_monitorService.orderSummary();
    int total = s.reserved + s.producing + s.confirmed + s.released;

    std::cout << boxTop();
    std::cout << boxRowA(std::string(Color::BOLD) + "  주문량 현황" + Color::RESET, 13);
    std::cout << boxDiv();
    std::cout << boxEmpty();

    auto printRow = [](const char* label, int cnt, const char* col, const char* barCol) {
        std::string bar = progressBar(cnt > 0 ? 100.0 : 0.0, 0);  // just count
        int pct = cnt;
        // Visual bar based on count (scale: max ~20 chars, relative)
        std::cout << boxRowA(
            "  " + std::string(col) + padRight(label, 12) + Color::RESET +
            "  " + std::string(barCol) + std::to_string(cnt) + "건" + Color::RESET,
            14 + 2 + (int)std::to_string(cnt).size() + 1);
        (void)pct; (void)bar;
    };

    // Draw bars relative to total
    auto makeBar = [&](int cnt) -> std::string {
        if (total == 0) return std::string(Color::DIM) + progressBar(0, 16) + Color::RESET;
        double pct = static_cast<double>(cnt) / total * 100.0;
        return std::string(Color::BGREEN) + progressBar(pct, 16) + Color::RESET;
    };

    auto printSummaryRow = [&](const char* label, int cnt, const char* col) {
        char cntBuf[8]; std::snprintf(cntBuf, sizeof(cntBuf), "%3d건", cnt);
        std::string row = "  " + std::string(col) + padRight(label, 12) + Color::RESET
                        + "  " + makeBar(cnt)
                        + "  " + std::string(col) + cntBuf + Color::RESET;
        std::cout << boxRowA(row, 14 + 2 + 16 + 2 + 5);
        std::cout << boxEmpty();
    };

    printSummaryRow("RESERVED",  s.reserved,  Color::BCYAN);
    printSummaryRow("PRODUCING", s.producing, Color::BYELLOW);
    printSummaryRow("CONFIRMED", s.confirmed, Color::BGREEN);
    printSummaryRow("RELEASED",  s.released,  Color::BBLUE);

    std::cout << boxRow("  " + sline(INNER_W - 2));
    char totalBuf[16]; std::snprintf(totalBuf, sizeof(totalBuf), "%d건", total);
    std::cout << boxRowA(
        "  " + std::string(Color::BWHITE) + "합계" + Color::RESET +
        "                              " + Color::BOLD + totalBuf + Color::RESET,
        4 + 30 + (int)std::strlen(totalBuf));
    std::cout << boxEmpty();
    std::cout << boxDiv();
    std::cout << boxRow(std::string(Color::DIM) + "  임의 키를 눌러 돌아가세요" + Color::RESET);
    std::cout << boxBot();
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
}

void MonitorView::showStockStatus() {
    clearScreen();
    auto stocks = m_monitorService.stockStatus();

    std::cout << boxTop();
    std::cout << boxRowA(std::string(Color::BOLD) + "  재고량 현황" + Color::RESET, 13);
    std::cout << boxDiv();

    if (stocks.empty()) {
        std::cout << boxEmpty();
        std::cout << boxRow(centerText("등록된 시료가 없습니다", INNER_W));
        std::cout << boxEmpty();
    } else {
        std::string hdr = padRight("  시료ID", 8) + padRight("시료명", 20)
                        + padRight("현재재고", 12) + padRight("생산중", 12) + "상태";
        std::cout << boxEmpty();
        std::cout << boxRow(hdr);
        std::cout << boxRow("  " + sline(INNER_W - 2));

        for (const auto& s : stocks) {
            const char* stCol  = Color::BGREEN;
            const char* stText = "여유";
            if      (s.status == MonitorService::StockStatus::DEPLETED) { stCol = Color::BRED;    stText = "고갈"; }
            else if (s.status == MonitorService::StockStatus::SHORTAGE) { stCol = Color::BYELLOW; stText = "부족"; }

            char inProd[16] = "-";
            if (s.inProductionQty > 0)
                std::snprintf(inProd, sizeof(inProd), "+%d ea", s.inProductionQty);

            std::string stockStr = std::to_string(s.stock) + " ea";
            std::string row = padRight("  " + s.id, 8)
                            + padRight(s.name, 20)
                            + padRight(stockStr, 12)
                            + padRight(inProd, 12)
                            + std::string(stCol) + stText + Color::RESET;
            std::cout << boxRowA(row, 8 + 20 + 12 + 12 + 2);
        }
        std::cout << boxEmpty();
    }

    std::cout << boxDiv();
    std::cout << boxRow(std::string(Color::DIM) + "  임의 키를 눌러 돌아가세요" + Color::RESET);
    std::cout << boxBot();
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
}
