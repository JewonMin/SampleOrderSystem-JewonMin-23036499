#define NOMINMAX
#include <windows.h>
#include <iostream>
#include <numeric>
#include <limits>

#include "repository/JsonRepository.h"
#include "service/SampleService.h"
#include "service/OrderService.h"
#include "service/ApprovalService.h"
#include "service/ProductionService.h"
#include "service/ReleaseService.h"
#include "service/MonitorService.h"
#include "view/SampleView.h"
#include "view/OrderView.h"
#include "view/ApprovalView.h"
#include "view/ProductionView.h"
#include "view/ReleaseView.h"
#include "view/MonitorView.h"
#include "view/ViewUtils.h"

static void setupConsole() {
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode = 0;
    GetConsoleMode(h, &mode);
    SetConsoleMode(h, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
}

int main() {
    setupConsole();
    showSplash();

    JsonRepository repo;
    repo.load();

    SampleService     sampleService(repo);
    OrderService      orderService(repo);
    ApprovalService   approvalService(repo);
    ProductionService productionService(repo);
    ReleaseService    releaseService(repo);
    MonitorService    monitorService(repo);

    SampleView     sampleView(sampleService);
    OrderView      orderView(orderService, sampleService);
    ApprovalView   approvalView(approvalService, sampleService);
    ProductionView productionView(productionService);
    ReleaseView    releaseView(releaseService, sampleService);
    MonitorView    monitorView(monitorService, productionService);

    int choice = -1;
    while (choice != 0) {
        productionService.autoComplete();
        clearScreen();

        // ── 집계 ────────────────────────────────────────────────────────────
        int sampleCnt = static_cast<int>(repo.samples().size());
        int totalStock = 0;
        for (const auto& s : repo.samples()) totalStock += s.stock;
        int totalOrders = static_cast<int>(repo.orders().size());
        int queueCnt    = static_cast<int>(repo.productionQueue().size());

        int pendingApproval   = static_cast<int>(approvalService.reservedOrders().size());
        int pendingRelease    = static_cast<int>(releaseService.confirmedOrders().size());

        // ── 메인 메뉴 ────────────────────────────────────────────────────────
        std::cout << boxTop();
        std::cout << boxEmpty();
        std::cout << boxRow(centerText(
            std::string(Color::BWHITE) + "반도체 시료 생산주문관리 시스템" + Color::RESET,
            INNER_W + 11));
        std::cout << boxRow(centerText(
            std::string(Color::DIM) + "S-Semi Corporation" + Color::RESET,
            INNER_W + 7));
        std::cout << boxEmpty();
        std::cout << boxDiv();

        // 시스템 현황
        std::string timeRow = std::string(Color::BWHITE) + " 시스템 현황" + Color::RESET
                            + "   " + currentTimeStr();
        std::cout << boxRowA(timeRow, 12 + 1 + 19);
        std::cout << boxEmpty();

        char stockBuf[32], orderBuf[32], queueBuf[32];
        std::snprintf(stockBuf, sizeof(stockBuf), "%d ea", totalStock);
        std::snprintf(orderBuf, sizeof(orderBuf), "%d건", totalOrders);
        std::snprintf(queueBuf, sizeof(queueBuf), "%d건 대기", queueCnt);

        std::string col1 = "  등록 시료   " + std::string(Color::BCYAN) + std::to_string(sampleCnt) + "종" + Color::RESET;
        std::string col2 = "   총 재고   " + std::string(Color::BGREEN) + stockBuf + Color::RESET;
        int col1dw = 14 + static_cast<int>(std::to_string(sampleCnt).size()) + 1;
        int col2dw = 13 + static_cast<int>(std::string(stockBuf).size());
        std::cout << boxRowA(col1 + col2, col1dw + col2dw);

        std::string col3 = "  전체 주문   " + std::to_string(totalOrders) + "건";
        std::string col4 = "   생산라인  ";
        std::string queueStr = (queueCnt > 0)
            ? std::string(Color::BYELLOW) + queueBuf + Color::RESET
            : std::string(Color::DIM)     + "없음"   + Color::RESET;
        int col34dw = 14 + static_cast<int>(std::to_string(totalOrders).size()) + 1
                    + 13 + (queueCnt > 0 ? static_cast<int>(std::string(queueBuf).size()) : 2);
        std::cout << boxRowA(col3 + col4 + queueStr, col34dw);

        std::cout << boxEmpty();
        std::cout << boxDiv();
        std::cout << boxEmpty();

        // ── 메뉴 항목 ────────────────────────────────────────────────────────
        std::cout << boxRow("  [1] 시료 관리              [2] 시료 주문");

        // [3] with pending badge
        std::string m3 = "  [3] 주문 승인/거절";
        if (pendingApproval > 0)
            m3 += "  " + std::string(Color::BYELLOW) + "(" + std::to_string(pendingApproval) + "건 대기)" + Color::RESET;
        int m3pad = 36 + (pendingApproval > 0 ? static_cast<int>(std::to_string(pendingApproval).size()) + 7 : 0);
        std::string m3full = padRight(m3, 36 + (pendingApproval > 0 ? 9 + (int)std::to_string(pendingApproval).size() : 0))
                           + "  [4] 모니터링";
        // Simplified approach for 2-column menu rows
        {
            std::string left  = "  [3] 주문 승인/거절";
            std::string badge3 = (pendingApproval > 0)
                ? ("  " + std::string(Color::BYELLOW) + "(" + std::to_string(pendingApproval) + "건)" + Color::RESET)
                : "";
            std::string right = "   [4] 모니터링";
            int ldw = displayWidth(left) + (pendingApproval > 0 ? 2 + (int)std::to_string(pendingApproval).size() + 4 : 0);
            std::cout << boxRowA(left + badge3 + padRight(right, INNER_W - ldw), INNER_W);
        }
        {
            std::string left  = "  [5] 생산라인 조회";
            std::string badge5 = (queueCnt > 0)
                ? ("  " + std::string(Color::BYELLOW) + "(" + std::to_string(queueCnt) + "건)" + Color::RESET)
                : "";
            std::string right = "   [6] 출고 처리";
            std::string badge6 = (pendingRelease > 0)
                ? ("  " + std::string(Color::BGREEN) + "(" + std::to_string(pendingRelease) + "건)" + Color::RESET)
                : "";
            int ldw = displayWidth(left) + (queueCnt > 0 ? 2 + (int)std::to_string(queueCnt).size() + 4 : 0);
            int rdw = displayWidth(right) + (pendingRelease > 0 ? 2 + (int)std::to_string(pendingRelease).size() + 4 : 0);
            std::cout << boxRowA(left + badge5 + padRight(right + badge6, INNER_W - ldw), INNER_W);
        }

        std::cout << boxRow("  [0] 종료");
        std::cout << boxEmpty();
        std::cout << boxDiv();

        // 입력 프롬프트
        std::cout << "\xe2\x95\x91 " << Color::BOLD << "  선택 ▶ " << Color::RESET << std::flush;

        if (!(std::cin >> choice)) {
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            choice = -1;
            continue;
        }
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

        switch (choice) {
            case 1: sampleView.run();      break;
            case 2: orderView.run();       break;
            case 3: approvalView.run();    break;
            case 4: monitorView.run();     break;
            case 5: productionView.run();  break;
            case 6: releaseView.run();     break;
            case 0: break;
            default:
                std::cout << "\n  " << Color::BRED << "  잘못된 입력입니다." << Color::RESET << "\n";
                Sleep(600);
        }
    }

    repo.save();
    clearScreen();
    std::cout << "\n  " << Color::BGREEN << "  시스템을 종료합니다." << Color::RESET << "\n\n";
    return 0;
}
