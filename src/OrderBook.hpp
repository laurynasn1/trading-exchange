#pragma once
#include "Order.hpp"
#include "MarketDataEvent.hpp"
#include <array>
#include <span>
#include <memory>
#include <unordered_map>
#include <vector>

static constexpr double MAX_PRICE = 10000.0;
static constexpr double TICK_SIZE = 0.01;
static constexpr size_t NUM_PRICE_LEVELS = MAX_PRICE / TICK_SIZE + 1;

struct PriceLevel
{
    std::shared_ptr<Order> head = nullptr;
    std::shared_ptr<Order> tail = nullptr;
};

class OrderBook
{
private:
    std::string symbol;

    std::array<PriceLevel, NUM_PRICE_LEVELS> bids;
    std::array<PriceLevel, NUM_PRICE_LEVELS> asks;

    uint32_t maxBid = 0;
    uint32_t minAsk = NUM_PRICE_LEVELS;

    std::unordered_map<uint64_t, std::shared_ptr<Order>> orders;

    uint64_t nextTradeId = 1;

    MarketDataEvent AddOrder(std::shared_ptr<Order> order);

    void MatchAgainstBook(std::shared_ptr<Order> order, std::span<PriceLevel> bookSide, uint32_t & topPrice, uint32_t endOfBook, char dir, bool shouldBeLess, std::vector<MarketDataEvent> & events);

    bool CheckAvailableLiquidity(std::shared_ptr<Order> order, std::span<PriceLevel> bookSide, uint32_t topPrice, uint32_t endOfBook, char dir, bool shouldBeLess);

public:
    OrderBook(std::string symbol_);

    MarketDataEvent CancelOrder(uint64_t targetOrderId, uint64_t requestId = 0);

    std::vector<MarketDataEvent> MatchOrder(std::shared_ptr<Order> order);

    std::pair<uint32_t, uint32_t> GetTopOfBook();
    void PrintBook(int levels = 5);
};