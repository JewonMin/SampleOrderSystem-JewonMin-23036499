#include "ProductionView.h"
#include "ViewUtils.h"
#include <conio.h>
#include <iostream>
#include <iomanip>
#include <limits>
#include <cstdio>
#include <ctime>
#include <string>

static std::string etaStr(double remainMin) {
    if (remainMin <= 0.0) return Color::BGREEN + std::string("완료 임박") + Color::RESET;
    std::time_t eta = std::time(nullptr) + static_cast<std::time_t>(remainMin * 60.0);
    char buf[16];
    std::strftime(buf, sizeof(buf), "%H:%M", std::localtime(&eta));
    return std::string(Color::CYAN) + buf + Color::RESET;
}

ProductionView::ProductionView(ProductionService& service) : m_service(service) {}

void ProductionView::run() {
    int choice = -1;
    while (choice != 0) {
        clearScreen();
        auto items = m_service.list();
        std::string stateStr = items.empty()
            ? (std::string(Color::DIM)    + "● IDLE"    + Color::RESET)
            : (std::string(Color::BGREEN) + "● RUNNING" + Color::RESET);

        std::cout << boxTop();
        std::cout << boxRowA(
            std::string(Color::BOLD) + "  [5] 생산라인 조회" + Color::RESET +
            "  ·  FIFO  ·  " + stateStr + "  ·  " + currentTimeStr(),
            19 + 10 + (items.empty() ? 6 : 9) + 2 + 19);
        std::cout << boxDiv();
        std::cout << boxRow("  [1] 현황 (라이브 새로고침)    [2] 생산 완료 처리    [0] 위로");
        std::cout << boxDiv();
        std::cout << "\xe2\x95\x91 " << Color::BOLD << "  선택 ▶ " << Color::RESET << std::flush;

        if (!(std::cin >> choice)) {
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            choice = -1; continue;
        }
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

        switch (choice) {
            case 1: showLiveQueue();        break;
            case 2: completeProduction();   break;
            case 0: break;
            default:
                std::cout << boxRowA(std::string(Color::BRED) + "  잘못된 입력입니다." + Color::RESET, 14);
                std::cout << boxBot();
                Sleep(600);
        }
    }
}

static void drawProductionFrame(const std::vector<ProductionService::ProductionInfo>& items) {
    clearScreen();

    std::string stateStr = items.empty()
        ? (std::string(Color::DIM)    + "● IDLE"    + Color::RESET)
        : (std::string(Color::BGREEN) + "● RUNNING" + Color::RESET);

    std::cout << boxTop();
    std::cout << boxRowA(
        std::string(Color::BOLD) + "  [5] 생산라인 조회  ·  FIFO  ·  " + Color::RESET +
        stateStr + "   " + Color::DIM + currentTimeStr() + Color::RESET,
        34 + (items.empty() ? 6 : 9) + 3 + 19);
    std::cout << boxDiv();

    if (items.empty()) {
        std::cout << boxEmpty();
        std::cout << boxRow(centerText("진행 중인 생산이 없습니다", INNER_W));
        std::cout << boxEmpty();
    } else {
        // ── 현재 처리 중 (FIFO 첫 번째) ─────────────────────────────────────
        const auto& cur = items[0];
        double remainMin = cur.totalProductionTime - cur.elapsedMin;

        std::cout << boxEmpty();
        std::cout << boxRowA(
            "  " + std::string(Color::BOLD) + "현재 처리 중" + Color::RESET, 14);

        std::string oidRow = "  주문번호  " + std::string(Color::BCYAN) + cur.orderId + Color::RESET
                           + "    시료  " + Color::BWHITE + cur.sampleName + Color::RESET;
        std::cout << boxRowA(oidRow, 12 + displayWidth(cur.orderId) + 8 + displayWidth(cur.sampleName));

        std::string qtyRow = "  주문량  " + std::to_string(cur.orderQty) + " ea"
                           + "  │  부족  " + std::string(Color::YELLOW) + std::to_string(cur.shortageQty) + " ea" + Color::RESET
                           + "  │  실생산량  " + std::string(Color::BGREEN) + std::to_string(cur.actualProductQty) + " ea" + Color::RESET;
        std::cout << boxRowA(qtyRow,
            10 + (int)std::to_string(cur.orderQty).size() + 3
            + 10 + (int)std::to_string(cur.shortageQty).size() + 3
            + 14 + (int)std::to_string(cur.actualProductQty).size() + 3);

        // 진행 바 + % + ETA
        char pctBuf[16]; std::snprintf(pctBuf, sizeof(pctBuf), "  %.1f%%", cur.progressPct);
        std::string barRow = "  " + std::string(Color::BGREEN) + progressBar(cur.progressPct, 24)
                           + Color::RESET + Color::BYELLOW + pctBuf + Color::RESET
                           + "   완료 예정  " + etaStr(remainMin);
        int barDw = 2 + 24 + 7 + 14 + (remainMin <= 0 ? 6 : 5);
        std::cout << boxRowA(barRow, barDw);

        // 경과 시간
        char timeBuf[32]; std::snprintf(timeBuf, sizeof(timeBuf), "%.1f / %.1f min", cur.elapsedMin, cur.totalProductionTime);
        std::cout << boxRow("  " + std::string(Color::DIM) + "경과  " + timeBuf + Color::RESET);

        // ── 대기 큐 ──────────────────────────────────────────────────────────
        if (items.size() > 1) {
            std::cout << boxDiv();
            std::cout << boxRowA(
                "  " + std::string(Color::BOLD) + "대기 중인 주문  " + Color::RESET +
                Color::DIM + "(FIFO 순)" + Color::RESET, 18 + 7);
            std::cout << boxEmpty();

            std::string hdr = padRight("  순서", 6) + padRight("주문번호", 22)
                            + padRight("시료", 16) + padRight("주문량", 8)
                            + padRight("부족분", 8) + "실생산량";
            std::cout << boxRow(hdr);
            std::cout << boxRow("  " + sline(INNER_W - 2));

            for (size_t i = 1; i < items.size(); ++i) {
                const auto& it = items[i];
                std::string row = padRight("  " + std::to_string(i), 6)
                                + padRight(it.orderId, 22)
                                + padRight(it.sampleName, 16)
                                + padRight(std::to_string(it.orderQty)   + " ea", 8)
                                + padRight(std::to_string(it.shortageQty) + " ea", 8)
                                + std::to_string(it.actualProductQty) + " ea";
                std::cout << boxRow(row);
            }
        }
        std::cout << boxEmpty();
    }

    std::cout << boxDiv();
    std::cout << boxRow(std::string(Color::DIM) + "  임의 키를 누르면 메뉴로 돌아갑니다 (자동 새로고침 1초)" + Color::RESET);
    std::cout << boxBot();
    std::cout << std::flush;
}

void ProductionView::showLiveQueue() {
    while (true) {
        auto items = m_service.list();
        drawProductionFrame(items);

        // 1초 동안 50ms 간격으로 키 입력 체크
        for (int t = 0; t < 20; ++t) {
            Sleep(50);
            if (_kbhit()) {
                _getch();  // consume
                return;
            }
        }
        // 1초 경과 → 자동 새로고침
    }
}

void ProductionView::completeProduction() {
    clearScreen();
    auto items = m_service.list();

    std::cout << boxTop();
    std::cout << boxRowA(
        std::string(Color::BOLD) + "  생산 완료 처리" + Color::RESET +
        "  " + Color::DIM + "(FIFO — 첫 번째 항목만 처리 가능)" + Color::RESET,
        16 + 2 + 34);

    if (items.empty()) {
        std::cout << boxDiv();
        std::cout << boxEmpty();
        std::cout << boxRow(centerText("진행 중인 생산이 없습니다", INNER_W));
        std::cout << boxEmpty();
        std::cout << boxBot();
        std::cout << "  Enter 키를 누르면 돌아갑니다 > ";
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        return;
    }

    const auto& cur = items[0];
    char pctBuf[8]; std::snprintf(pctBuf, sizeof(pctBuf), "%.1f%%", cur.progressPct);

    std::cout << boxDiv();
    std::cout << boxEmpty();
    std::cout << boxRow("  주문번호  " + cur.orderId);
    std::cout << boxRow("  시료      " + cur.sampleName);
    std::cout << boxRow("  주문량    " + std::to_string(cur.orderQty) + " ea");
    std::cout << boxRow("  실생산량  " + std::to_string(cur.actualProductQty) + " ea");

    std::string barRow = "  진행률    "
        + std::string(Color::BGREEN) + progressBar(cur.progressPct, 20) + Color::RESET
        + "  " + Color::BYELLOW + pctBuf + Color::RESET;
    std::cout << boxRowA(barRow, 12 + 20 + 2 + (int)std::strlen(pctBuf));

    std::cout << boxEmpty();
    std::cout << boxDiv();
    std::cout << "\xe2\x95\x91 " << Color::BOLD << "  생산 완료 처리하시겠습니까? [Y/N] ▶ " << Color::RESET << std::flush;

    char confirm; std::cin >> confirm;
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    std::cout << "\n";

    if (std::toupper(static_cast<unsigned char>(confirm)) != 'Y') {
        std::cout << boxRow(std::string(Color::DIM) + "  취소되었습니다." + Color::RESET);
        std::cout << boxBot(); Sleep(600); return;
    }

    try {
        m_service.complete(cur.orderId);
        std::cout << boxRowA(
            "  " + std::string(Color::BGREEN) + "✓  생산 완료 처리됨." + Color::RESET, 16);
        std::cout << boxRowA(
            "  상태 변경   " + std::string(Color::BYELLOW) + "PRODUCING" + Color::RESET
            + "  →  " + std::string(Color::BGREEN) + "CONFIRMED" + Color::RESET, 14+9+5+9);
        std::cout << boxRow("  주문번호    " + cur.orderId);
    } catch (const std::exception& e) {
        std::cout << boxRowA(std::string(Color::BRED) + "  [오류] " + e.what() + Color::RESET,
            8 + (int)std::strlen(e.what()));
    }
    std::cout << boxBot();
    Sleep(1200);
}
