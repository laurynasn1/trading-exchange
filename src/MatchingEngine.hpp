#pragma once

#include <unordered_map>
#include <thread>
#include <functional>

#include "OrderBook.hpp"
#include "SPSCQueue.hpp"
#include "SymbolMap.hpp"

class MatchingEngine {
private:
    std::unordered_map<std::string, uint8_t> symbolMap;
    std::vector<std::unique_ptr<OrderBook>> books;

    std::vector<char> orderToSymbol;

    std::shared_ptr<SPSCQueue<OrderRequest>> inputQueue;
    std::function<void(const MarketDataEvent&)> eventCallback;

    std::thread thread;
    std::atomic<bool> running{ false };

    bool ValidateOrder(const Order& order, std::string& error);

public:
    MatchingEngine(std::shared_ptr<SPSCQueue<OrderRequest>> input, const std::function<void(const MarketDataEvent&)> & eventCallback_, size_t numBooks, size_t maxNumOrders)
        : symbolMap(LoadSymbolMap()), inputQueue(input), eventCallback(std::move(eventCallback_))
    {
        orderToSymbol.resize(maxNumOrders, -1);
        books.reserve(numBooks);
        auto it = symbolMap.begin();
        for (int i = 0; i < numBooks && it != symbolMap.end(); i++, it++)
            books.emplace_back(std::make_unique<OrderBook>(it->first, maxNumOrders));
    }

    MatchingEngine(const std::function<void(const MarketDataEvent&)> & eventCallback_, size_t numBooks = 1, size_t maxNumOrders = 1000)
        : MatchingEngine(nullptr, eventCallback_, numBooks, maxNumOrders) {}

    MatchingEngine(size_t numBooks = 1, size_t maxNumOrders = 50'000'000)
        : MatchingEngine(nullptr, [](const auto &){}, numBooks, maxNumOrders) {}

    ~MatchingEngine()
    {
        Stop();
    }

    void SubmitOrder(Order* order);
    bool CancelOrder(uint64_t targetOrderId, uint64_t requestId = 0);
    OrderBook* GetBook(uint8_t symbolId);

    void Start();
    void Stop();
    void Run();
};