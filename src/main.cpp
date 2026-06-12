#define NOMINMAX
#include <windows.h>
#include <iostream>
#include <limits>

#include "repository/JsonRepository.h"
#include "service/SampleService.h"
#include "service/OrderService.h"
#include "view/SampleView.h"
#include "view/OrderView.h"

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

    SampleService sampleService(repo);
    OrderService  orderService(repo);

    SampleView sampleView(sampleService);
    OrderView  orderView(orderService, sampleService);

    int choice = -1;
    while (choice != 0) {
        std::cout << "\n============================================================\n";
        std::cout << "          반도체 시료 생산주문관리 시스템\n";
        std::cout << "============================================================\n";
        std::cout << "  [1] 시료 관리\n";
        std::cout << "  [2] 시료 주문\n";
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
            case 1: sampleView.run(); break;
            case 2: orderView.run();  break;
            case 0: break;
            default: std::cout << "  (준비 중인 메뉴입니다)\n";
        }
    }

    repo.save();
    std::cout << "\n시스템을 종료합니다.\n";
    return 0;
}
