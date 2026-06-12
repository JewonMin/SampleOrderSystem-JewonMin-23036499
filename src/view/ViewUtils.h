#pragma once
#include <string>
#include <cstdio>

// ── ANSI Color ────────────────────────────────────────────────────────────────
namespace Color {
    inline const char* RESET   = "\033[0m";
    inline const char* BOLD    = "\033[1m";
    inline const char* RED     = "\033[31m";
    inline const char* GREEN   = "\033[32m";
    inline const char* YELLOW  = "\033[33m";
    inline const char* BLUE    = "\033[34m";
    inline const char* MAGENTA = "\033[35m";
    inline const char* CYAN    = "\033[36m";
    inline const char* WHITE   = "\033[37m";
    inline const char* BRED    = "\033[1;31m";
    inline const char* BGREEN  = "\033[1;32m";
    inline const char* BYELLOW = "\033[1;33m";
    inline const char* BCYAN   = "\033[1;36m";
    inline const char* BWHITE  = "\033[1;37m";
}

// ── UTF-8 display width (ASCII=1, CJK/Korean=2) ───────────────────────────────
inline int displayWidth(const std::string& s) {
    int w = 0;
    for (size_t i = 0; i < s.size(); ) {
        unsigned char c = static_cast<unsigned char>(s[i]);
        if      (c < 0x80)              { ++w;    ++i;   }
        else if ((c & 0xE0) == 0xC0)   { ++w;    i += 2; }
        else if ((c & 0xF0) == 0xE0)   { w += 2; i += 3; }
        else                            { w += 2; i += 4; }
    }
    return w;
}

// ── Pad string to display width ───────────────────────────────────────────────
inline std::string padRight(const std::string& s, int width) {
    int dw = displayWidth(s);
    return dw >= width ? s : s + std::string(width - dw, ' ');
}

// ── Unicode block progress bar (filled=█, empty=░) ────────────────────────────
inline std::string progressBar(double pct, int width = 10) {
    if (pct < 0.0)   pct = 0.0;
    if (pct > 100.0) pct = 100.0;
    int filled = static_cast<int>(pct / 100.0 * width + 0.5);
    std::string bar;
    bar.reserve(static_cast<size_t>(width) * 3);
    for (int i = 0; i < width; ++i)
        bar += (i < filled) ? "\xe2\x96\x88" : "\xe2\x96\x91";  // █ / ░
    return bar;
}

// ── Colored status badge ──────────────────────────────────────────────────────
#include "../model/Order.h"
inline std::string statusBadge(OrderStatus st) {
    switch (st) {
        case OrderStatus::RESERVED:  return std::string(Color::CYAN)    + "[RESERVED]"  + Color::RESET;
        case OrderStatus::PRODUCING: return std::string(Color::BYELLOW) + "[PRODUCING]" + Color::RESET;
        case OrderStatus::CONFIRMED: return std::string(Color::BGREEN)  + "[CONFIRMED]" + Color::RESET;
        case OrderStatus::RELEASED:  return std::string(Color::BLUE)    + "[RELEASED]"  + Color::RESET;
        case OrderStatus::REJECTED:  return std::string(Color::RED)     + "[REJECTED]"  + Color::RESET;
    }
    return "";
}

// ── Section header ────────────────────────────────────────────────────────────
#include <ctime>
inline std::string currentTimeStr() {
    std::time_t t = std::time(nullptr);
    char buf[32];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", std::localtime(&t));
    return buf;
}
