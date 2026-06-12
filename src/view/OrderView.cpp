#include "OrderView.h"
#include "ViewUtils.h"
#include <iostream>
#include <limits>
#include <cctype>
#include <cstdio>

OrderView::OrderView(OrderService& orderService, SampleService& sampleService)
    : m_orderService(orderService), m_sampleService(sampleService) {}

void OrderView::run() {
    int choice = -1;
    while (choice != 0) {
        clearScreen();
        std::cout << boxTop();
        std::cout << boxRowA(std::string(Color::BOLD) + "  [2] 시료 주문" + Color::RESET, 15);
        std::cout << boxDiv();
        std::cout << boxRow("  [1] 주문 접수    [0] 위로");
        std::cout << boxDiv();
        std::cout << "\xe2\x95\x91 " << Color::BOLD << "  선택 ▶ " << Color::RESET << std::flush;

        if (!(std::cin >> choice)) {
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            choice = -1; continue;
        }
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

        switch (choice) {
            case 1: showCreate(); break;
            case 0: break;
            default:
                std::cout << boxRowA(std::string(Color::BRED) + "  잘못된 입력입니다." + Color::RESET, 14);
                std::cout << boxBot(); Sleep(600);
        }
    }
}

void OrderView::showCreate() {
    clearScreen();
    std::cout << boxTop();
    std::cout << boxRowA(std::string(Color::BOLD) + "  주문 접수" + Color::RESET, 11);
    std::cout << boxDiv();
    std::cout << boxEmpty();

    // 시료 ID
    std::cout << "\xe2\x95\x91 " << "  시료 ID       ▶ " << std::flush;
    std::string sampleId;
    std::getline(std::cin, sampleId);
    const Sample* sample = m_sampleService.findById(sampleId);
    if (!sample) {
        std::cout << boxRowA(std::string(Color::BRED) + "  [오류] 등록되지 않은 시료 ID입니다." + Color::RESET, 27);
        std::cout << boxBot(); Sleep(600); return;
    }

    // 시료 정보 미리보기
    char yieldBuf[16], timeBuf[32];
    std::snprintf(yieldBuf, sizeof(yieldBuf), "%.2f",        sample->yield);
    std::snprintf(timeBuf,  sizeof(timeBuf),  "%.2f min/ea", sample->avgProductionTime);
    std::cout << boxRowA(
        "    → " + std::string(Color::BWHITE) + sample->name + Color::RESET
        + "   재고 " + Color::BGREEN + std::to_string(sample->stock) + " ea" + Color::RESET
        + "   수율 " + yieldBuf,
        6 + displayWidth(sample->name) + 5 + (int)std::to_string(sample->stock).size() + 3 + 4 + (int)std::strlen(yieldBuf));
    std::cout << boxEmpty();

    // 고객명
    std::cout << "\xe2\x95\x91 " << "  고객명        ▶ " << std::flush;
    std::string customerName;
    std::getline(std::cin, customerName);
    if (customerName.empty()) {
        std::cout << boxRowA(std::string(Color::BRED) + "  [오류] 고객명을 입력해야 합니다." + Color::RESET, 24);
        std::cout << boxBot(); Sleep(600); return;
    }

    // 수량
    std::cout << "\xe2\x95\x91 " << "  주문 수량 (ea) ▶ " << std::flush;
    int quantity = 0;
    if (!(std::cin >> quantity) || quantity <= 0) {
        std::cin.clear(); std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        std::cout << boxRowA(std::string(Color::BRED) + "  [오류] 1 이상 정수여야 합니다." + Color::RESET, 22);
        std::cout << boxBot(); Sleep(600); return;
    }
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

    // 확인 화면
    std::cout << boxEmpty();
    std::cout << boxDiv();
    std::cout << boxRowA(std::string(Color::BOLD) + "  입력 내용 확인" + Color::RESET, 16);
    std::cout << boxDiv();
    std::cout << boxEmpty();
    std::cout << boxRow("  시료    " + sample->name + "    (" + sample->id + ")");
    std::cout << boxRow("  고객    " + customerName);
    std::cout << boxRow("  수량    " + std::to_string(quantity) + " ea");
    std::cout << boxEmpty();
    std::cout << boxDiv();
    std::cout << "\xe2\x95\x91 " << Color::BOLD << "  [Y] 예약 접수    [N] 취소    ▶ " << Color::RESET << std::flush;

    char confirm; std::cin >> confirm;
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    confirm = static_cast<char>(std::toupper(static_cast<unsigned char>(confirm)));
    std::cout << "\n";

    if (confirm != 'Y') {
        std::cout << boxRow(std::string(Color::DIM) + "  주문이 취소되었습니다." + Color::RESET);
        std::cout << boxBot(); Sleep(600); return;
    }

    try {
        Order o = m_orderService.create(sampleId, customerName, quantity);
        std::cout << boxEmpty();
        std::cout << boxRowA(std::string(Color::BGREEN) + "  ✓ 예약 접수 완료" + Color::RESET, 17);
        std::cout << boxEmpty();
        std::cout << boxRow("  주문번호    " + o.orderId);
        std::cout << boxRowA(
            "  현재 상태   " + std::string(Color::BCYAN) + "RESERVED" + Color::RESET, 14+8);
        std::cout << boxEmpty();
        std::cout << boxRowA(
            "  " + std::string(Color::DIM) + "※ 재고 확인은 [3] 승인 메뉴에서 진행하세요." + Color::RESET, 40);
        std::cout << boxEmpty();
    } catch (const std::exception& e) {
        std::cout << boxRowA(std::string(Color::BRED) + "  [오류] " + e.what() + Color::RESET,
            8 + (int)std::strlen(e.what()));
    }
    std::cout << boxBot();
    Sleep(1200);
}
