#include "ProductionView.h"
#include "ViewUtils.h"
#include <conio.h>
#include <iostream>
#include <limits>
#include <cstdio>
#include <ctime>
#include <string>
#include <algorithm>

static const int WAIT_PAGE_SIZE = 4;

static std::string etaStr(double remainMin) {
    if (remainMin <= 0.0) return Color::BGREEN + std::string("완료 임박") + Color::RESET;
    std::time_t eta = std::time(nullptr) + static_cast<std::time_t>(remainMin * 60.0);
    char buf[16];
    std::strftime(buf, sizeof(buf), "%H:%M", std::localtime(&eta));
    return std::string(Color::CYAN) + buf + Color::RESET;
}

ProductionView::ProductionView(ProductionService& service) : m_service(service) {}

static void drawProductionFrame(
    const std::vector<ProductionService::ProductionInfo>& items, int waitPage)
{
    clearScreen();

    std::string stateStr = items.empty()
        ? (std::string(Color::DIM)    + "IDLE"    + Color::RESET)
        : (std::string(Color::BGREEN) + "RUNNING" + Color::RESET);

    std::cout << boxTop();
    std::cout << boxRowA(
        std::string(Color::BOLD) + "  [5] Production Line  FIFO  " + Color::RESET +
        stateStr + "   " + Color::DIM + currentTimeStr() + Color::RESET,
        29 + (items.empty() ? 4 : 7) + 3 + 19);
    std::cout << boxDiv();

    if (items.empty()) {
        std::cout << boxEmpty();
        std::cout << boxRow(centerText("No active production jobs", INNER_W));
        std::cout << boxEmpty();
    } else {
        const auto& cur = items[0];
        double remainMin = cur.totalProductionTime - cur.elapsedMin;

        std::cout << boxEmpty();
        std::cout << boxRowA(
            "  " + std::string(Color::BOLD) + "Current Job" + Color::RESET, 13);

        std::string oidRow = "  Order  " + std::string(Color::BCYAN) + cur.orderId + Color::RESET
                           + "    Sample  " + Color::BWHITE + cur.sampleName + Color::RESET;
        std::cout << boxRowA(oidRow, 9 + (int)cur.orderId.size() + 11 + displayWidth(cur.sampleName));

        std::string qtyRow = "  Qty  " + std::to_string(cur.orderQty) + " ea"
                           + "  |  Shortage  " + std::string(Color::YELLOW) + std::to_string(cur.shortageQty) + " ea" + Color::RESET
                           + "  |  Actual  " + std::string(Color::BGREEN) + std::to_string(cur.actualProductQty) + " ea" + Color::RESET;
        std::cout << boxRowA(qtyRow,
            7  + (int)std::to_string(cur.orderQty).size()      + 3
            + 14 + (int)std::to_string(cur.shortageQty).size() + 3
            + 12 + (int)std::to_string(cur.actualProductQty).size() + 3);

        char pctBuf[16]; std::snprintf(pctBuf, sizeof(pctBuf), "  %.1f%%", cur.progressPct);
        std::string barRow = "  " + std::string(Color::BGREEN) + progressBar(cur.progressPct, 24)
                           + Color::RESET + Color::BYELLOW + pctBuf + Color::RESET
                           + "   ETA  " + etaStr(remainMin);
        int barDw = 2 + 24 + 7 + 8 + (remainMin <= 0 ? 6 : 5);
        std::cout << boxRowA(barRow, barDw);

        char timeBuf[32]; std::snprintf(timeBuf, sizeof(timeBuf), "%.1f / %.1f min", cur.elapsedMin, cur.totalProductionTime);
        std::cout << boxRow("  " + std::string(Color::DIM) + "Elapsed  " + timeBuf + Color::RESET);

        if (items.size() > 1) {
            size_t waitCount  = items.size() - 1;
            size_t totalPages = (waitCount + WAIT_PAGE_SIZE - 1) / WAIT_PAGE_SIZE;
            size_t startIdx   = 1 + (size_t)waitPage * WAIT_PAGE_SIZE;
            size_t endIdx     = std::min(startIdx + WAIT_PAGE_SIZE, items.size());

            std::cout << boxDiv();

            if (totalPages > 1) {
                std::string pg = std::to_string(waitPage + 1) + "/" + std::to_string(totalPages);
                std::string waitHdr =
                    "  " + std::string(Color::BOLD) + "Queue  " + Color::RESET +
                    Color::DIM + "(FIFO)" + Color::RESET +
                    "  " + Color::BCYAN + "[" + pg + "]" + Color::RESET +
                    "  " + Color::DIM + "[N]next  [P]prev" + Color::RESET;
                int hdrDw = 9 + 6 + 2 + (int)pg.size() + 2 + 12;
                std::cout << boxRowA(waitHdr, hdrDw);
            } else {
                std::cout << boxRow("  Queue  (FIFO)");
            }

            std::cout << boxEmpty();
            std::string hdr = padRight("  #", 5) + padRight("OrderId", 22)
                            + padRight("Sample", 16) + padRight("Qty", 8)
                            + padRight("Short", 8) + "Actual";
            std::cout << boxRow(hdr);
            std::cout << boxRow("  " + sline(INNER_W - 2));

            for (size_t i = startIdx; i < endIdx; ++i) {
                const auto& it = items[i];
                std::string row = padRight("  " + std::to_string(i), 5)
                                + padRight(it.orderId, 22)
                                + padRight(it.sampleName, 16)
                                + padRight(std::to_string(it.orderQty)    + " ea", 8)
                                + padRight(std::to_string(it.shortageQty) + " ea", 8)
                                + std::to_string(it.actualProductQty) + " ea";
                std::cout << boxRow(row);
            }
        }
        std::cout << boxEmpty();
    }

    std::cout << boxDiv();
    if (items.size() > (size_t)(1 + WAIT_PAGE_SIZE)) {
        std::cout << boxRow(std::string(Color::DIM)
            + "  [N]next page  [P]prev page  |  any other key: back  (auto-refresh 1s)"
            + Color::RESET);
    } else {
        std::cout << boxRow(std::string(Color::DIM)
            + "  Any key to return  (auto-refresh 1s)"
            + Color::RESET);
    }
    std::cout << boxBot();
    std::cout << std::flush;
}

void ProductionView::run() {
    int waitPage = 0;
    while (true) {
        m_service.autoComplete();

        auto items = m_service.list();
        size_t waitCount = items.size() > 1 ? items.size() - 1 : 0;
        int maxPage = waitCount > 0 ? (int)((waitCount - 1) / WAIT_PAGE_SIZE) : 0;
        if (waitPage > maxPage) waitPage = maxPage;

        drawProductionFrame(items, waitPage);

        bool redraw = false;
        for (int t = 0; t < 20; ++t) {
            Sleep(50);
            if (_kbhit()) {
                int ch = _getch();
                if (ch == 'n' || ch == 'N') {
                    if (waitPage < maxPage) waitPage++;
                    redraw = true;
                    break;
                } else if (ch == 'p' || ch == 'P') {
                    if (waitPage > 0) waitPage--;
                    redraw = true;
                    break;
                } else {
                    return;
                }
            }
        }
        (void)redraw;
    }
}
