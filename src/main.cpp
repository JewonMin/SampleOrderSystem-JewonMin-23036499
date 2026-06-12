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

    auto badge = [](int n) -> std::string {
        if (n <= 0) return "";
        return "  (" + std::to_string(n) + "건 대기)";
    };

    int choice = -1;
    while (choice != 0) {
        // ── 시스템 현황 집계 ──────────────────────────────────────────────────
        int sampleCount   = static_cast<int>(repo.samples().size());
        int totalStock    = 0;
        for (const auto& s : repo.samples()) totalStock += s.stock;
        int totalOrders   = static_cast<int>(repo.orders().size());
        int queueCount    = static_cast<int>(repo.productionQueue().size());

        int pendingApproval   = static_cast<int>(approvalService.reservedOrders().size());
        int activeProductions = queueCount;
        int pendingRelease    = static_cast<int>(releaseService.confirmedOrders().size());

        // ── 메인 메뉴 출력 ────────────────────────────────────────────────────
        std::cout << "\n============================================================\n";
        std::cout << Color::BOLD
                  << "          반도체 시료 생산주문관리 시스템\n"
                  << Color::RESET;
        std::cout << "============================================================\n";
        std::cout << Color::BWHITE << " 시스템 현황" << Color::RESET
                  << "   " << currentTimeStr() << "\n\n";
        std::cout << "  등록 시료   " << Color::CYAN << sampleCount << "종" << Color::RESET
                  << "        총 재고      "
                  << Color::GREEN << totalStock << " ea" << Color::RESET << "\n";
        std::cout << "  전체 주문   " << totalOrders << "건"
                  << "        생산라인     "
                  << (queueCount > 0
                      ? std::string(Color::BYELLOW) + std::to_string(queueCount) + "건 대기" + Color::RESET
                      : std::string("없음"))
                  << "\n";
        std::cout << "------------------------------------------------------------\n";

        // PRD 메뉴 순서: 1=시료관리, 2=시료주문, 3=승인/거절, 4=모니터링, 5=생산라인, 6=출고
        std::cout << "  [1] 시료 관리              [2] 시료 주문\n";
        std::cout << "  [3] 주문 승인/거절" << badge(pendingApproval)
                  << "          [4] 모니터링\n";
        std::cout << "  [5] 생산라인 조회" << badge(activeProductions)
                  << "          [6] 출고 처리" << badge(pendingRelease) << "\n";
        std::cout << "  [0] 종료\n";
        std::cout << "------------------------------------------------------------\n";
        std::cout << "  선택 > ";

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
            default: std::cout << "  (준비 중인 메뉴입니다)\n";
        }
    }

    repo.save();
    std::cout << "\n시스템을 종료합니다.\n";
    return 0;
}
