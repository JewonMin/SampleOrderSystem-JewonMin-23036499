#include "OrderView.h"
#include <iostream>
#include <limits>
#include <cctype>

OrderView::OrderView(OrderService& orderService, SampleService& sampleService)
    : m_orderService(orderService), m_sampleService(sampleService) {}

void OrderView::run() {
    int choice = -1;
    while (choice != 0) {
        std::cout << "\n============================================================\n";
        std::cout << "  [2] 시료 주문\n";
        std::cout << "------------------------------------------------------------\n";
        std::cout << "  [1] 주문 접수    [0] 위로\n";
        std::cout << "  선택 > ";

        if (!(std::cin >> choice)) {
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            choice = -1;
            continue;
        }
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

        switch (choice) {
            case 1: showCreate(); break;
            case 0: break;
            default: std::cout << "  잘못된 입력입니다.\n";
        }
    }
}

void OrderView::showCreate() {
    std::cout << "\n--- 주문 접수 ---\n";

    // 시료 ID 입력
    std::string sampleId;
    std::cout << "  시료 ID    > ";
    std::getline(std::cin, sampleId);

    const Sample* sample = m_sampleService.findById(sampleId);
    if (!sample) {
        std::cout << "  [오류] 등록되지 않은 시료 ID입니다.\n";
        return;
    }

    // 고객명 입력
    std::string customerName;
    std::cout << "  고객명     > ";
    std::getline(std::cin, customerName);
    if (customerName.empty()) {
        std::cout << "  [오류] 고객명을 입력해야 합니다.\n";
        return;
    }

    // 수량 입력
    int quantity = 0;
    std::cout << "  주문 수량  > ";
    if (!(std::cin >> quantity) || quantity <= 0) {
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        std::cout << "  [오류] 주문 수량은 1 이상 정수여야 합니다.\n";
        return;
    }
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

    // 확인 화면
    std::cout << "\n  입력 내용 확인\n";
    std::cout << "  시료    " << sample->name << "  (" << sample->id << ")\n";
    std::cout << "  고객    " << customerName << "\n";
    std::cout << "  수량    " << quantity << " ea\n";
    std::cout << "\n  [Y] 예약 접수    [N] 취소\n";
    std::cout << "  선택 > ";

    char confirm;
    std::cin >> confirm;
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    confirm = static_cast<char>(std::toupper(static_cast<unsigned char>(confirm)));

    if (confirm != 'Y') {
        std::cout << "  주문이 취소되었습니다.\n";
        return;
    }

    try {
        Order o = m_orderService.create(sampleId, customerName, quantity);
        std::cout << "\n  예약 접수 완료.\n\n";
        std::cout << "  주문번호    " << o.orderId << "\n";
        std::cout << "  현재 상태   RESERVED\n\n";
        std::cout << "  ※ 재고 확인은 [3] 승인 메뉴에서 직접 진행하세요.\n";
    } catch (const std::exception& e) {
        std::cout << "  [오류] " << e.what() << "\n";
    }
}
