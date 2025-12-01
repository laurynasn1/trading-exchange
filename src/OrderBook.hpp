#pragma once
#include "Order.hpp"
#include "MarketDataEvent.hpp"
#include "OutputPolicy.hpp"
#include <map>
#include <list>
#include <memory>
#include <unordered_map>
#include <vector>

class OrderBook
{
private:
    std::string symbol;

    std::map<double, std::list<std::shared_ptr<Order>>, std::greater<double>> bids;
    std::map<double, std::list<std::shared_ptr<Order>>> asks;

    std::unordered_map<uint64_t, std::shared_ptr<Order>> orders;

    uint64_t nextTradeId = 1;

    MarketDataEvent AddOrder(std::shared_ptr<Order> order);

    template<typename BookSide>
    std::vector<MarketDataEvent> MatchAgainstBook(std::shared_ptr<Order> order, BookSide& bookSide);

    template<typename BookSide>
    bool CheckAvailableLiquidity(std::shared_ptr<Order> order, BookSide& bookSide);

public:
    OrderBook(std::string symbol_);

    MarketDataEvent CancelOrder(uint64_t orderId);

    std::vector<MarketDataEvent> MatchOrder(std::shared_ptr<Order> order);

    std::pair<double, double> GetTopOfBook() const;
    void PrintBook(int levels = 5) const;
    void Clear();
};