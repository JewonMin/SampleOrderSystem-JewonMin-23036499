#define NOMINMAX
#include <windows.h>
#include <iostream>
#include <limits>

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

    int choice = -1;
    while (choice != 0) {
        std::cout << "\n============================================================\n";
        std::cout << "          반도체 시료 생산주문관리 시스템\n";
        std::cout << "============================================================\n";
        std::cout << "  [0] 종료\n";
        std::cout << "------------------------------------------------------------\n";
        std::cout << "  선택 > ";

        if (!(std::cin >> choice)) {
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            choice = -1;
            continue;
        }

        if (choice != 0) {
            std::cout << "  (준비 중인 메뉴입니다)\n";
        }
    }

    std::cout << "\n시스템을 종료합니다.\n";
    return 0;
}
