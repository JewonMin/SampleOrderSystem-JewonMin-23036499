#include "ProductionView.h"
#include "ViewUtils.h"
#include <iostream>
#include <iomanip>
#include <limits>
#include <cstdio>
#include <ctime>
#include <string>

static std::string etaStr(double remainMin) {
    if (remainMin <= 0) return "완료 임박";
    std::time_t eta = std::time(nullptr) + static_cast<std::time_t>(remainMin * 60.0);
    char buf[16];
    std::strftime(buf, sizeof(buf), "%H:%M", std::localtime(&eta));
    return buf;
}

ProductionView::ProductionView(ProductionService& service) : m_service(service) {}

void ProductionView::run() {
    int choice = -1;
    while (choice != 0) {
        std::cout << "\n============================================================\n";
        std::cout << Color::BOLD << "  [5] 생산라인 조회" << Color::RESET
                  << "    FIFO 방식    " << currentTimeStr() << "\n";
        std::cout << "------------------------------------------------------------\n";
        std::cout << "  [1] 생산 현황 조회    [2] 생산 완료 처리    [0] 위로\n";
        std::cout << "  선택 > ";

        if (!(std::cin >> choice)) {
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            choice = -1;
            continue;
        }
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

        switch (choice) {
            case 1: showQueue();          break;
            case 2: completeProduction(); break;
            case 0: break;
            default: std::cout << "  잘못된 입력입니다.\n";
        }
    }
}

void ProductionView::showQueue() {
    auto items = m_service.list();

    std::string stateStr = items.empty()
        ? (std::string(Color::CYAN) + "IDLE" + Color::RESET)
        : (std::string(Color::BGREEN) + "RUNNING" + Color::RESET);
    std::cout << "\n  생산라인 1개 (단일 라인)    현재 상태: " << stateStr << "\n";

    if (items.empty()) {
        std::cout << "  (진행 중인 생산이 없습니다)\n";
        return;
    }

    // ── 현재 처리 중 (FIFO 첫 번째) ──────────────────────────────────────────
    const auto& cur = items[0];
    double remainMin = cur.totalProductionTime - cur.elapsedMin;

    std::cout << "\n  " << Color::BOLD << "현재 처리 중" << Color::RESET << "\n";
    std::cout << "  " << std::string(68, '-') << "\n";
    std::cout << "  주문번호  " << Color::CYAN << cur.orderId << Color::RESET
              << "    시료  " << cur.sampleName << "\n";
    std::cout << "  주문량  " << cur.orderQty << " ea"
              << "    재고부족  " << cur.shortageQty << " ea"
              << "    실생산량  " << cur.actualProductQty << " ea\n";

    char pctBuf[8]; std::snprintf(pctBuf, sizeof(pctBuf), "%.1f%%", cur.progressPct);
    std::cout << "  진행  "
              << Color::BGREEN << progressBar(cur.progressPct) << Color::RESET
              << "  " << Color::BYELLOW << pctBuf << Color::RESET
              << "    완료 예정  " << etaStr(remainMin) << "\n";

    if (items.size() == 1) {
        std::cout << "  " << std::string(68, '-') << "\n";
        return;
    }

    // ── 대기 중인 주문 (FIFO 순) ──────────────────────────────────────────────
    std::cout << "\n  " << Color::BOLD << "대기 중인 주문  (FIFO 순)" << Color::RESET << "\n";
    std::cout << "  " << std::string(68, '-') << "\n";
    std::cout << "  "
              << padRight("순서", 6)
              << padRight("주문번호", 22)
              << padRight("시료", 18)
              << padRight("주문량", 8)
              << padRight("부족분", 8)
              << "실생산량\n";
    std::cout << "  " << std::string(68, '-') << "\n";

    for (size_t i = 1; i < items.size(); ++i) {
        const auto& it = items[i];
        std::cout << "  "
                  << padRight(std::to_string(i), 6)
                  << padRight(it.orderId,  22)
                  << padRight(it.sampleName, 18)
                  << padRight(std::to_string(it.orderQty)  + " ea", 8)
                  << padRight(std::to_string(it.shortageQty) + " ea", 8)
                  << it.actualProductQty << " ea\n";
    }
    std::cout << "  " << std::string(68, '-') << "\n";
}

void ProductionView::completeProduction() {
    auto items = m_service.list();
    if (items.empty()) {
        std::cout << "\n  (진행 중인 생산이 없습니다)\n";
        return;
    }

    // FIFO: 첫 번째 항목만 완료 가능
    const auto& cur = items[0];
    char pctBuf[8]; std::snprintf(pctBuf, sizeof(pctBuf), "%.1f%%", cur.progressPct);

    std::cout << "\n  " << Color::BOLD << "생산 완료 처리" << Color::RESET
              << "  (FIFO — 첫 번째 항목만 처리 가능)\n";
    std::cout << "  " << std::string(56, '-') << "\n";
    std::cout << "  주문번호  " << Color::CYAN << cur.orderId << Color::RESET << "\n";
    std::cout << "  시료      " << cur.sampleName << "\n";
    std::cout << "  실생산량  " << cur.actualProductQty << " ea\n";
    std::cout << "  진행률    "
              << Color::BGREEN << progressBar(cur.progressPct) << Color::RESET
              << "  " << Color::BYELLOW << pctBuf << Color::RESET << "\n\n";
    std::cout << "  생산 완료 처리하시겠습니까? [Y/N] > ";

    char confirm;
    std::cin >> confirm;
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    if (std::toupper(static_cast<unsigned char>(confirm)) != 'Y') {
        std::cout << "  취소되었습니다.\n";
        return;
    }

    try {
        m_service.complete(cur.orderId);
        std::cout << "\n  " << Color::BGREEN << "생산 완료 처리됨." << Color::RESET << "\n";
        std::cout << "  상태 변경   "
                  << Color::BYELLOW << "PRODUCING" << Color::RESET << "  →  "
                  << Color::BGREEN  << "CONFIRMED" << Color::RESET << "\n";
        std::cout << "  주문번호    " << cur.orderId << "\n";
    } catch (const std::exception& e) {
        std::cout << "  " << Color::BRED << "[오류] " << e.what() << Color::RESET << "\n";
    }
}
