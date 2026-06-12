#include "SampleView.h"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <limits>
#include <cctype>
#include <cstdio>

static const int PAGE_SIZE = 5;

// UTF-8 기준 터미널 표시 너비 계산 (ASCII=1, CJK/한글=2)
static int displayWidth(const std::string& s) {
    int w = 0;
    for (size_t i = 0; i < s.size(); ) {
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (c < 0x80) {
            ++w; ++i;
        } else if ((c & 0xE0) == 0xC0) {
            ++w; i += 2;
        } else if ((c & 0xF0) == 0xE0) {
            w += 2; i += 3;
        } else {
            w += 2; i += 4;
        }
    }
    return w;
}

static std::string padRight(const std::string& s, int width) {
    int dw = displayWidth(s);
    return dw >= width ? s : s + std::string(width - dw, ' ');
}

SampleView::SampleView(SampleService& service) : m_service(service) {}

void SampleView::run() {
    int choice = -1;
    while (choice != 0) {
        std::cout << "\n============================================================\n";
        std::cout << "  [1] 시료 관리\n";
        std::cout << "------------------------------------------------------------\n";
        std::cout << "  [1] 시료 등록    [2] 시료 목록    [3] 시료 검색    [0] 위로\n";
        std::cout << "  선택 > ";

        if (!(std::cin >> choice)) {
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            choice = -1;
            continue;
        }
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

        switch (choice) {
            case 1: showRegister(); break;
            case 2: showList();     break;
            case 3: showSearch();   break;
            case 0: break;
            default: std::cout << "  잘못된 입력입니다.\n";
        }
    }
}

void SampleView::showRegister() {
    std::cout << "\n--- 시료 등록 ---\n";

    std::string name;
    std::cout << "  시료명: ";
    std::getline(std::cin, name);
    if (name.empty()) {
        std::cout << "  [오류] 시료명을 입력해야 합니다.\n";
        return;
    }

    double avgTime = 0;
    std::cout << "  평균 생산시간 (min/ea): ";
    if (!(std::cin >> avgTime) || avgTime <= 0) {
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        std::cout << "  [오류] 평균 생산시간은 0 초과 숫자여야 합니다.\n";
        return;
    }

    double yield = 0;
    std::cout << "  수율 (0 < 수율 <= 1): ";
    if (!(std::cin >> yield) || yield <= 0 || yield > 1) {
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        std::cout << "  [오류] 수율은 0 초과 1 이하 숫자여야 합니다.\n";
        return;
    }

    int stock = 0;
    std::cout << "  초기 재고 (ea): ";
    if (!(std::cin >> stock) || stock < 0) {
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        std::cout << "  [오류] 재고는 0 이상 정수여야 합니다.\n";
        return;
    }
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

    try {
        Sample s = m_service.add(name, avgTime, yield, stock);
        std::cout << "\n  [완료] " << s.id << " / " << s.name << " 등록됨\n";
    } catch (const std::exception& e) {
        std::cout << "  [오류] " << e.what() << "\n";
    }
}

static void printTableHeader() {
    std::cout << "\n"
              << padRight("ID",       8)
              << padRight("시료명",   24)
              << padRight("평균생산시간", 18)
              << padRight("수율",     8)
              << "현재재고\n"
              << std::string(70, '-') << "\n";
}

static void printTableRow(const Sample& s) {
    char timeBuf[32], yieldBuf[16];
    std::snprintf(timeBuf,  sizeof(timeBuf),  "%.2f min/ea", s.avgProductionTime);
    std::snprintf(yieldBuf, sizeof(yieldBuf), "%.2f",        s.yield);

    std::cout << padRight(s.id,          8)
              << padRight(s.name,        24)
              << padRight(timeBuf,       18)
              << padRight(yieldBuf,      8)
              << s.stock << " ea\n";
}

void SampleView::showList() {
    const auto& samples = m_service.all();
    if (samples.empty()) {
        std::cout << "\n  (등록된 시료가 없습니다)\n";
        return;
    }

    int total      = static_cast<int>(samples.size());
    int totalPages = (total + PAGE_SIZE - 1) / PAGE_SIZE;
    int page       = 0;

    while (true) {
        int start = page * PAGE_SIZE;
        int end   = std::min(start + PAGE_SIZE, total);

        std::cout << "\n등록 시료 목록  (총 " << total << "종, "
                  << (page + 1) << "/" << totalPages << " 페이지)\n";
        printTableHeader();
        for (int i = start; i < end; ++i)
            printTableRow(samples[i]);

        bool hasPrev = (page > 0);
        bool hasNext = (end < total);
        if (!hasPrev && !hasNext) break;

        std::cout << "\n  ";
        if (hasPrev) std::cout << "[P] 이전페이지  ";
        if (hasNext) std::cout << "[N] 다음페이지  ";
        std::cout << "[0] 돌아가기\n  선택 > ";

        char nav;
        std::cin >> nav;
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        nav = static_cast<char>(std::toupper(static_cast<unsigned char>(nav)));

        if      (nav == 'N' && hasNext) ++page;
        else if (nav == 'P' && hasPrev) --page;
        else break;
    }
}

void SampleView::showSearch() {
    std::cout << "\n--- 시료 검색 (이름) ---\n  검색어: ";

    std::string keyword;
    std::getline(std::cin, keyword);

    auto results = m_service.searchByName(keyword);
    if (results.empty()) {
        std::cout << "  (검색 결과 없음)\n";
        return;
    }

    std::cout << "\n검색 결과 (" << results.size() << "건)\n";
    printTableHeader();
    for (const auto* s : results)
        printTableRow(*s);
}
