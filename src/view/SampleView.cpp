#include "SampleView.h"
#include "ViewUtils.h"
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <limits>
#include <cctype>
#include <cstdio>

static const int PAGE_SIZE = 5;

SampleView::SampleView(SampleService& service) : m_service(service) {}

void SampleView::run() {
    int choice = -1;
    while (choice != 0) {
        clearScreen();
        std::cout << boxTop();
        std::cout << boxRowA(std::string(Color::BOLD) + "  [1] 시료 관리" + Color::RESET, 15);
        std::cout << boxDiv();
        std::cout << boxRow("  [1] 시료 등록    [2] 시료 목록    [3] 시료 검색    [0] 위로");
        std::cout << boxDiv();
        std::cout << "\xe2\x95\x91 " << Color::BOLD << "  선택 ▶ " << Color::RESET << std::flush;

        if (!(std::cin >> choice)) {
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            choice = -1; continue;
        }
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

        switch (choice) {
            case 1: showRegister(); break;
            case 2: showList();     break;
            case 3: showSearch();   break;
            case 0: break;
            default:
                std::cout << boxRowA(std::string(Color::BRED) + "  잘못된 입력입니다." + Color::RESET, 14);
                std::cout << boxBot(); Sleep(600);
        }
    }
}

void SampleView::showRegister() {
    clearScreen();
    std::cout << boxTop();
    std::cout << boxRowA(std::string(Color::BOLD) + "  시료 등록" + Color::RESET, 11);
    std::cout << boxDiv();
    std::cout << boxEmpty();

    // 시료명
    std::cout << "\xe2\x95\x91 " << "  시료명               ▶ " << std::flush;
    std::string name;
    std::getline(std::cin, name);
    if (name.empty()) {
        std::cout << boxRowA(std::string(Color::BRED) + "  [오류] 시료명을 입력해야 합니다." + Color::RESET, 24);
        std::cout << boxBot(); Sleep(600); return;
    }

    // 평균 생산시간
    std::cout << "\xe2\x95\x91 " << "  평균 생산시간 (min)  ▶ " << std::flush;
    double avgTime = 0;
    if (!(std::cin >> avgTime) || avgTime <= 0) {
        std::cin.clear(); std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        std::cout << boxRowA(std::string(Color::BRED) + "  [오류] 0 초과 숫자여야 합니다." + Color::RESET, 22);
        std::cout << boxBot(); Sleep(600); return;
    }

    // 수율
    std::cout << "\xe2\x95\x91 " << "  수율 (0 < y ≤ 1)    ▶ " << std::flush;
    double yield = 0;
    if (!(std::cin >> yield) || yield <= 0 || yield > 1) {
        std::cin.clear(); std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        std::cout << boxRowA(std::string(Color::BRED) + "  [오류] 0 초과 1 이하 숫자여야 합니다." + Color::RESET, 28);
        std::cout << boxBot(); Sleep(600); return;
    }

    // 초기 재고
    std::cout << "\xe2\x95\x91 " << "  초기 재고 (ea)       ▶ " << std::flush;
    int stock = 0;
    if (!(std::cin >> stock) || stock < 0) {
        std::cin.clear(); std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        std::cout << boxRowA(std::string(Color::BRED) + "  [오류] 0 이상 정수여야 합니다." + Color::RESET, 22);
        std::cout << boxBot(); Sleep(600); return;
    }
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

    try {
        Sample s = m_service.add(name, avgTime, yield, stock);
        std::cout << boxEmpty();
        std::cout << boxRowA(
            std::string(Color::BGREEN) + "  ✓ 등록 완료" + Color::RESET, 12);
        char timeBuf[32], yieldBuf[16];
        std::snprintf(timeBuf,  sizeof(timeBuf),  "%.2f min/ea", s.avgProductionTime);
        std::snprintf(yieldBuf, sizeof(yieldBuf), "%.2f",        s.yield);
        std::cout << boxRow("  ID    " + s.id);
        std::cout << boxRow("  시료명 " + s.name);
        std::cout << boxRow("  생산시간 " + std::string(timeBuf) + "    수율 " + yieldBuf + "    재고 " + std::to_string(s.stock) + " ea");
        std::cout << boxEmpty();
    } catch (const std::exception& e) {
        std::cout << boxRowA(std::string(Color::BRED) + "  [오류] " + e.what() + Color::RESET,
            8 + (int)std::strlen(e.what()));
    }
    std::cout << boxBot();
    Sleep(1000);
}

void SampleView::showList() {
    const auto& samples = m_service.all();

    clearScreen();
    std::cout << boxTop();
    std::cout << boxRowA(std::string(Color::BOLD) + "  시료 목록" + Color::RESET, 11);
    std::cout << boxDiv();

    if (samples.empty()) {
        std::cout << boxEmpty();
        std::cout << boxRow(centerText("등록된 시료가 없습니다", INNER_W));
        std::cout << boxEmpty();
        std::cout << boxBot();
        Sleep(600); return;
    }

    int total      = static_cast<int>(samples.size());
    int totalPages = (total + PAGE_SIZE - 1) / PAGE_SIZE;
    int page       = 0;

    while (true) {
        // Re-draw box header only for page transitions
        if (page > 0) {
            clearScreen();
            std::cout << boxTop();
            char pageHdr[64];
            std::snprintf(pageHdr, sizeof(pageHdr), "  시료 목록  (총 %d종,  %d/%d 페이지)", total, page+1, totalPages);
            std::cout << boxRowA(std::string(Color::BOLD) + pageHdr + Color::RESET,
                (int)std::strlen(pageHdr));
            std::cout << boxDiv();
        } else {
            char pageHdr[64];
            std::snprintf(pageHdr, sizeof(pageHdr), "  총 %d종,  %d/%d 페이지", total, page+1, totalPages);
            std::cout << boxRow(pageHdr);
            std::cout << boxDiv();
        }

        int start = page * PAGE_SIZE;
        int end   = std::min(start + PAGE_SIZE, total);

        std::string hdr = padRight("  ID",  10) + padRight("시료명", 20)
                        + padRight("생산시간", 16) + padRight("수율", 8) + "재고";
        std::cout << boxEmpty();
        std::cout << boxRow(hdr);
        std::cout << boxRow("  " + sline(INNER_W - 2));

        for (int i = start; i < end; ++i) {
            const Sample& s = samples[i];
            char timeBuf[32], yieldBuf[16];
            std::snprintf(timeBuf,  sizeof(timeBuf),  "%.2f min/ea", s.avgProductionTime);
            std::snprintf(yieldBuf, sizeof(yieldBuf), "%.2f",        s.yield);
            std::string row = padRight("  " + s.id, 10)
                            + padRight(s.name, 20)
                            + padRight(timeBuf, 16)
                            + padRight(yieldBuf, 8)
                            + std::to_string(s.stock) + " ea";
            std::cout << boxRow(row);
        }

        std::cout << boxEmpty();

        bool hasPrev = (page > 0);
        bool hasNext = (end < total);

        if (!hasPrev && !hasNext) {
            std::cout << boxDiv();
            std::cout << boxRow(std::string(Color::DIM) + "  임의 키를 눌러 돌아가세요" + Color::RESET);
            std::cout << boxBot();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            break;
        }

        std::cout << boxDiv();
        std::string nav = "  ";
        if (hasPrev) nav += "[P] 이전    ";
        if (hasNext) nav += "[N] 다음    ";
        nav += "[0] 위로";
        std::cout << boxRow(nav);
        std::cout << boxDiv();
        std::cout << "\xe2\x95\x91 " << Color::BOLD << "  선택 ▶ " << Color::RESET << std::flush;

        char c; std::cin >> c;
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));

        if      (c == 'N' && hasNext) ++page;
        else if (c == 'P' && hasPrev) --page;
        else { std::cout << boxBot(); break; }
    }
}

void SampleView::showSearch() {
    clearScreen();
    std::cout << boxTop();
    std::cout << boxRowA(std::string(Color::BOLD) + "  시료 검색" + Color::RESET, 11);
    std::cout << boxDiv();
    std::cout << "\xe2\x95\x91 " << "  검색어 ▶ " << std::flush;

    std::string keyword;
    std::getline(std::cin, keyword);

    auto results = m_service.searchByName(keyword);

    if (results.empty()) {
        std::cout << boxEmpty();
        std::cout << boxRow(centerText("검색 결과가 없습니다", INNER_W));
        std::cout << boxEmpty();
        std::cout << boxBot();
        Sleep(600); return;
    }

    char hdr[64]; std::snprintf(hdr, sizeof(hdr), "  검색 결과  %d건", (int)results.size());
    std::cout << boxRow(hdr);
    std::cout << boxDiv();
    std::cout << boxEmpty();

    std::string colHdr = padRight("  ID", 10) + padRight("시료명", 20)
                       + padRight("생산시간", 16) + padRight("수율", 8) + "재고";
    std::cout << boxRow(colHdr);
    std::cout << boxRow("  " + sline(INNER_W - 2));

    for (const auto* s : results) {
        char timeBuf[32], yieldBuf[16];
        std::snprintf(timeBuf,  sizeof(timeBuf),  "%.2f min/ea", s->avgProductionTime);
        std::snprintf(yieldBuf, sizeof(yieldBuf), "%.2f",        s->yield);
        std::string row = padRight("  " + s->id, 10)
                        + padRight(s->name, 20)
                        + padRight(timeBuf, 16)
                        + padRight(yieldBuf, 8)
                        + std::to_string(s->stock) + " ea";
        std::cout << boxRow(row);
    }

    std::cout << boxEmpty();
    std::cout << boxDiv();
    std::cout << boxRow(std::string(Color::DIM) + "  임의 키를 눌러 돌아가세요" + Color::RESET);
    std::cout << boxBot();
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
}
