#pragma once
#include <array>
#include <string>

constexpr size_t MAX_NUM_SYMBOLS = 50;

constexpr std::array<std::string_view, MAX_NUM_SYMBOLS> SYMBOLS = {
    "AAPL", "MSFT", "NVDA", "GOOGL", "AMZN",
    "META", "TSLA", "AMD", "INTC", "NFLX",

    "JPM", "BAC", "WFC", "C", "GS",
    "MS", "BLK", "V", "MA", "PYPL",

    "WMT", "TGT", "COST", "HD", "MCD",
    "SBUX", "KO", "PEP", "PG", "NKE",

    "XOM", "CVX", "GE", "BA", "CAT",
    "UNP", "UPS", "F", "GM", "DE",

    "JNJ", "PFE", "MRK", "ABBV", "LLY",
    "UNH", "CVS", "BMY", "AMGN", "GILD"
};

constexpr std::string_view IdToSymbol(uint8_t id)
{
    return SYMBOLS[id];
}