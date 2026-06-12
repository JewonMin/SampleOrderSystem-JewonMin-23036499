#pragma once
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <string>
#include <iostream>
#include <cstdio>
#include <ctime>
#include <algorithm>
#include "../model/Order.h"

// ── ANSI Color ────────────────────────────────────────────────────────────────
namespace Color {
    inline const char* RESET   = "\033[0m";
    inline const char* BOLD    = "\033[1m";
    inline const char* DIM     = "\033[2m";
    inline const char* RED     = "\033[31m";
    inline const char* GREEN   = "\033[32m";
    inline const char* YELLOW  = "\033[33m";
    inline const char* BLUE    = "\033[34m";
    inline const char* CYAN    = "\033[36m";
    inline const char* WHITE   = "\033[37m";
    inline const char* BRED    = "\033[1;31m";
    inline const char* BGREEN  = "\033[1;32m";
    inline const char* BYELLOW = "\033[1;33m";
    inline const char* BBLUE   = "\033[1;34m";
    inline const char* BCYAN   = "\033[1;36m";
    inline const char* BWHITE  = "\033[1;37m";
}

// ── Box layout constants ──────────────────────────────────────────────────────
inline constexpr int BOX_W   = 72;   // total display width including borders
inline constexpr int INNER_W = BOX_W - 4;  // 68 = 1 space + content + 1 space

// ── UTF-8 display width (ASCII=1, CJK/Korean=2) ───────────────────────────────
inline int displayWidth(const std::string& s) {
    int w = 0;
    for (size_t i = 0; i < s.size(); ) {
        unsigned char c = static_cast<unsigned char>(s[i]);
        if      (c < 0x80)            { ++w;    ++i;   }
        else if ((c & 0xE0) == 0xC0)  { ++w;    i += 2; }
        else if ((c & 0xF0) == 0xE0)  { w += 2; i += 3; }
        else                          { w += 2; i += 4; }
    }
    return w;
}

inline std::string padRight(const std::string& s, int width) {
    int dw = displayWidth(s);
    return dw >= width ? s : s + std::string(width - dw, ' ');
}

inline std::string centerText(const std::string& s, int width) {
    int dw = displayWidth(s);
    if (dw >= width) return s;
    int pad = width - dw, left = pad / 2, right = pad - left;
    return std::string(left, ' ') + s + std::string(right, ' ');
}

// ── Box drawing ───────────────────────────────────────────────────────────────
inline std::string hline(int n) {
    std::string s; for (int i = 0; i < n; ++i) s += "\xe2\x95\x90"; return s; // ═
}
inline std::string sline(int n) {
    std::string s; for (int i = 0; i < n; ++i) s += "\xe2\x94\x80"; return s; // ─
}

inline std::string boxTop() {
    return "\xe2\x95\x94" + hline(BOX_W-2) + "\xe2\x95\x97\n"; // ╔═...═╗
}
inline std::string boxBot() {
    return "\xe2\x95\x9a" + hline(BOX_W-2) + "\xe2\x95\x9d\n"; // ╚═...═╝
}
inline std::string boxDiv() {
    return "\xe2\x95\xa0" + hline(BOX_W-2) + "\xe2\x95\xa3\n"; // ╠═...═╣
}
inline std::string boxEmpty() {
    return "\xe2\x95\x91" + std::string(BOX_W-2, ' ') + "\xe2\x95\x91\n"; // ║   ║
}

// content is plain text (no embedded ANSI) — auto-padded to INNER_W
inline std::string boxRow(const std::string& content) {
    int pad = std::max(0, INNER_W - displayWidth(content));
    return "\xe2\x95\x91 " + content + std::string(pad, ' ') + " \xe2\x95\x91\n";
}

// content may contain ANSI codes; contentDw = visible display width of content
inline std::string boxRowA(const std::string& content, int contentDw) {
    int pad = std::max(0, INNER_W - contentDw);
    return "\xe2\x95\x91 " + content + std::string(pad, ' ') + " \xe2\x95\x91\n";
}

// Two-column row: left and right split at midpoint
inline std::string boxRow2(const std::string& left, const std::string& right) {
    int lw = displayWidth(left), rw = displayWidth(right);
    int mid = INNER_W / 2;
    std::string row;
    row += std::string(std::max(0, mid - lw), ' ');
    std::string total = left + row + right;
    return boxRow(padRight(left, mid) + right);
}

inline void clearScreen() {
    std::cout << "\033[2J\033[H" << std::flush;
}

// ── Progress bar (Unicode blocks) ─────────────────────────────────────────────
inline std::string progressBar(double pct, int width = 20) {
    if (pct < 0.0)   pct = 0.0;
    if (pct > 100.0) pct = 100.0;
    int filled = static_cast<int>(pct / 100.0 * width + 0.5);
    std::string bar;
    for (int i = 0; i < width; ++i)
        bar += (i < filled) ? "\xe2\x96\x88" : "\xe2\x96\x91"; // █ / ░
    return bar;
}

// ── Status badge (with ANSI color) ───────────────────────────────────────────
inline std::string statusBadge(OrderStatus st) {
    switch (st) {
        case OrderStatus::RESERVED:  return std::string(Color::BCYAN)   + "RESERVED"  + Color::RESET;
        case OrderStatus::PRODUCING: return std::string(Color::BYELLOW) + "PRODUCING" + Color::RESET;
        case OrderStatus::CONFIRMED: return std::string(Color::BGREEN)  + "CONFIRMED" + Color::RESET;
        case OrderStatus::RELEASED:  return std::string(Color::BBLUE)   + "RELEASED"  + Color::RESET;
        case OrderStatus::REJECTED:  return std::string(Color::BRED)    + "REJECTED"  + Color::RESET;
    }
    return "";
}

// ── Current time string ───────────────────────────────────────────────────────
inline std::string currentTimeStr() {
    std::time_t t = std::time(nullptr);
    char buf[32];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d  %H:%M:%S", std::localtime(&t));
    return buf;
}

// ── Spinner animation (uses Windows Sleep) ───────────────────────────────────
inline void spin(const std::string& label, int ms = 1200) {
    static const char* fr[] = {"|", "/", "-", "\\"};
    int steps = ms / 80;
    for (int i = 0; i < steps; ++i) {
        std::cout << "\r  " << Color::CYAN << fr[i % 4] << Color::RESET
                  << "  " << label << "   " << std::flush;
        Sleep(80);
    }
    std::cout << "\r\033[K" << std::flush;
}

// ── Startup splash ────────────────────────────────────────────────────────────
inline void showSplash() {
    clearScreen();
    std::cout << "\n\n";
    std::cout << "  " << boxTop();
    std::cout << "  " << boxEmpty();
    std::cout << "  " << boxRow(centerText(
        std::string(Color::BWHITE) + "반도체 시료 생산주문관리 시스템" + Color::RESET, INNER_W + 11));
    std::cout << "  " << boxRow(centerText(
        std::string(Color::DIM) + "S-Semi Corporation" + Color::RESET, INNER_W + 7));
    std::cout << "  " << boxEmpty();
    std::cout << "  " << boxBot();

    const int BAR = 44;
    for (int i = 0; i <= BAR; ++i) {
        std::cout << "\r  " << Color::DIM << "[" << Color::RESET;
        for (int j = 0; j < BAR; ++j) {
            if (j < i) std::cout << Color::BGREEN  << "\xe2\x96\x88" << Color::RESET;
            else        std::cout << Color::DIM     << "\xe2\x96\x91" << Color::RESET;
        }
        std::cout << Color::DIM << "]" << Color::RESET
                  << "  " << Color::BYELLOW << (i * 100 / BAR) << "%" << Color::RESET
                  << "  시스템 초기화 중..." << std::flush;
        Sleep(22);
    }
    std::cout << "\n\n";
    Sleep(350);
}
