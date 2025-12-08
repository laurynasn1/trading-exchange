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
    std::array<std::unique_ptr<OrderBook>, NUM_SYMBOLS> books;

    std::unordered_map<uint64_t, uint8_t> orderToSymbol;

    std::shared_ptr<SPSCQueue<OrderRequest>> inputQueue;
    std::function<void(const MarketDataEvent&)> eventCallback;

    std::thread thread;
    std::atomic<bool> running{ false };

    bool ValidateOrder(const Order& order, std::string& error);

public:
    MatchingEngine(std::shared_ptr<SPSCQueue<OrderRequest>> input, const std::function<void(const MarketDataEvent&)> & eventCallback_)
        : symbolMap(LoadSymbolMap()), inputQueue(input), eventCallback(std::move(eventCallback_))
    {
        orderToSymbol.reserve(2'000'000);
        for (const auto & [symbol, id] : symbolMap)
            books[id] = std::make_unique<OrderBook>(symbol);
    }

    MatchingEngine(const std::function<void(const MarketDataEvent&)> & eventCallback_)
        : MatchingEngine(nullptr, eventCallback_) {}

    MatchingEngine()
        : MatchingEngine(nullptr, [](const auto &){}) {}

    ~MatchingEngine()
    {
        Stop();
    }

    void SubmitOrder(std::shared_ptr<Order> order);
    bool CancelOrder(uint64_t targetOrderId, uint64_t requestId = 0);
    OrderBook* GetBook(uint8_t symbolId);
    void Clear();

    void Start();
    void Stop();
    void Run();
};