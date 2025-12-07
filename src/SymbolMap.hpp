#pragma once
#include <unordered_map>
#include <string>
#include <cstdint>

inline const size_t NUM_SYMBOLS = 50;

inline std::unordered_map<std::string, uint8_t> LoadSymbolMap() {
    return {
        {"AAPL", 0},  {"MSFT", 1},  {"NVDA", 2},  {"GOOGL", 3}, {"AMZN", 4},
        {"META", 5},  {"TSLA", 6},  {"AMD", 7},   {"INTC", 8},  {"NFLX", 9},

        {"JPM", 10},  {"BAC", 11},  {"WFC", 12},  {"C", 13},    {"GS", 14},
        {"MS", 15},   {"BLK", 16},  {"V", 17},    {"MA", 18},   {"PYPL", 19},

        {"WMT", 20},  {"TGT", 21},  {"COST", 22}, {"HD", 23},   {"MCD", 24},
        {"SBUX", 25}, {"KO", 26},   {"PEP", 27},  {"PG", 28},   {"NKE", 29},

        {"XOM", 30},  {"CVX", 31},  {"GE", 32},   {"BA", 33},   {"CAT", 34},
        {"UNP", 35},  {"UPS", 36},  {"F", 37},    {"GM", 38},   {"DE", 39},

        {"JNJ", 40},  {"PFE", 41},  {"MRK", 42},  {"ABBV", 43}, {"LLY", 44},
        {"UNH", 45},  {"CVS", 46},  {"BMY", 47},  {"AMGN", 48}, {"GILD", 49}
    };
}