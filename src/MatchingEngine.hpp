#pragma once

#include <unordered_map>
#include <thread>
#include <functional>

#include "OrderBook.hpp"
#include "SPSCQueue.hpp"

class MatchingEngine {
private:
    std::unordered_map<std::string, std::unique_ptr<OrderBook>> books;

    std::shared_ptr<SPSCQueue<OrderRequest>> inputQueue;
    std::function<void(const MarketDataEvent&)> eventCallback;

    std::thread thread;
    std::atomic<bool> running{ false };

    OrderBook* GetOrCreateBook(const std::string& symbol);
    bool ValidateOrder(const Order& order, std::string& error);

public:
    MatchingEngine() = default;

    MatchingEngine(const std::function<void(const MarketDataEvent&)> & eventCallback_)
        : eventCallback(std::move(eventCallback_)) {}

    MatchingEngine(std::shared_ptr<SPSCQueue<OrderRequest>> input, const std::function<void(const MarketDataEvent&)> & eventCallback_)
        : inputQueue(input), eventCallback(std::move(eventCallback_)) {}

    ~MatchingEngine()
    {
        Stop();
    }

    void SubmitOrder(std::shared_ptr<Order> order);
    bool CancelOrder(uint64_t targetOrderId, uint64_t requestId = 0);
    OrderBook* GetBook(const std::string& symbol);
    void Clear();

    void Start();
    void Stop();
    void Run();
};