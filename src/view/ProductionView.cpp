#include "ProductionView.h"
#include <iostream>
#include <iomanip>
#include <limits>
#include <cstdio>
#include <string>

static std::string progressBar(double pct) {
    constexpr int BAR = 20;
    int filled = static_cast<int>(pct / 100.0 * BAR + 0.5);
    if (filled > BAR) filled = BAR;
    return "[" + std::string(filled, '#') + std::string(BAR - filled, '-') + "]";
}

ProductionView::ProductionView(ProductionService& service) : m_service(service) {}

void ProductionView::run() {
    int choice = -1;
    while (choice != 0) {
        std::cout << "\n============================================================\n";
        std::cout << "  [4] 생산라인 관리\n";
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
            case 1: showQueue();            break;
            case 2: completeProduction();   break;
            case 0: break;
            default: std::cout << "  잘못된 입력입니다.\n";
        }
    }
}

void ProductionView::showQueue() {
    auto items = m_service.list();
    if (items.empty()) {
        std::cout << "\n  (진행 중인 생산이 없습니다)\n";
        return;
    }

    std::cout << "\n  생산 현황  (" << items.size() << "건)\n\n";
    std::cout << "  " << std::left
              << std::setw(6)  << "번호"
              << std::setw(22) << "주문번호"
              << std::setw(16) << "시료"
              << std::setw(10) << "생산수량"
              << std::setw(28) << "진행률"
              << "경과/총시간\n";
    std::cout << "  " << std::string(88, '-') << "\n";

    for (int i = 0; i < static_cast<int>(items.size()); ++i) {
        const auto& it = items[i];
        char timeBuf[32];
        std::snprintf(timeBuf, sizeof(timeBuf), "%.1f / %.1f min",
                      it.elapsedMin, it.totalProductionTime);
        char pctBuf[8];
        std::snprintf(pctBuf, sizeof(pctBuf), " %5.1f%%", it.progressPct);

        std::cout << "  [" << std::left << std::setw(3) << (i + 1) << "] "
                  << std::setw(22) << it.orderId
                  << std::setw(16) << it.sampleName
                  << std::setw(10) << it.actualProductQty
                  << progressBar(it.progressPct) << pctBuf
                  << "  " << timeBuf << "\n";
    }
    std::cout << "\n  시작일시: " << items[0].startedAt << " (첫 번째 항목 기준)\n";
}

void ProductionView::completeProduction() {
    auto items = m_service.list();
    if (items.empty()) {
        std::cout << "\n  (진행 중인 생산이 없습니다)\n";
        return;
    }

    // 목록 표시
    std::cout << "\n  완료 처리할 생산을 선택하세요\n\n";
    for (int i = 0; i < static_cast<int>(items.size()); ++i) {
        char pctBuf[8];
        std::snprintf(pctBuf, sizeof(pctBuf), "%.1f%%", items[i].progressPct);
        std::cout << "  [" << (i + 1) << "] "
                  << items[i].orderId << "  "
                  << items[i].sampleName << "  "
                  << items[i].actualProductQty << " ea  "
                  << pctBuf << "\n";
    }
    std::cout << "  [0] 취소\n";
    std::cout << "  선택 > ";

    int sel;
    if (!(std::cin >> sel) || sel < 0 || sel > static_cast<int>(items.size())) {
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        std::cout << "  잘못된 입력입니다.\n";
        return;
    }
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    if (sel == 0) return;

    const auto& target = items[sel - 1];
    std::cout << "\n  주문번호  " << target.orderId << "\n";
    std::cout << "  시료      " << target.sampleName << "\n";
    std::cout << "  생산수량  " << target.actualProductQty << " ea\n";
    std::cout << "  진행률    ";
    char pct[8]; std::snprintf(pct, sizeof(pct), "%.1f%%", target.progressPct);
    std::cout << progressBar(target.progressPct) << " " << pct << "\n\n";
    std::cout << "  생산 완료 처리하시겠습니까? [Y/N] > ";

    char confirm;
    std::cin >> confirm;
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    if (std::toupper(static_cast<unsigned char>(confirm)) != 'Y') {
        std::cout << "  취소되었습니다.\n";
        return;
    }

    try {
        m_service.complete(target.orderId);
        std::cout << "\n  생산 완료 처리됨.\n";
        std::cout << "  상태 변경   PRODUCING  →  CONFIRMED\n";
        std::cout << "  주문번호    " << target.orderId << "\n";
    } catch (const std::exception& e) {
        std::cout << "  [오류] " << e.what() << "\n";
    }
}
