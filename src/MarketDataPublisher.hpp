#pragma once
#include "SPSCQueue.hpp"
#include "OrderBook.hpp"
#include <thread>
#include <atomic>
#include <iostream>

class MarketDataPublisher {
private:
    std::shared_ptr<SPSCQueue<MarketDataEvent>> queue;
    std::thread thread;
    std::atomic<bool> running{ false };

public:
    MarketDataPublisher(std::shared_ptr<SPSCQueue<MarketDataEvent>> queue_)
        : queue(queue_) {}

    ~MarketDataPublisher()
    {
        Stop();
    }

    void Start()
    {
        running = true;
        thread = std::thread(&MarketDataPublisher::Run, this);
    }

    void Stop()
    {
        running = false;
        if (thread.joinable())
            thread.join();
    }

    void Run()
    {
        uint64_t eventsProcessed = 0;

        while (running)
        {
            MarketDataEvent* trade = queue->TryPop();

            if (trade == nullptr)
            {
                std::this_thread::yield();
                continue;
            }

            Publish(*trade);
            eventsProcessed++;
        }

        std::cout << "Market data publisher processed " << eventsProcessed << " events\n";
    }

private:
    void Publish(const MarketDataEvent& trade)
    {
        switch (trade.type)
        {
        case EventType::ORDER_ACKED:
            stats.ackedOrders++;
            break;
        case EventType::ORDER_FILLED:
            stats.filledOrders++;
            break;
        case EventType::ORDER_CANCELLED:
            stats.canceledOrders++;
            break;
        default:
            break;
        }
    }

public:
    struct Stats
    {
        uint64_t filledOrders{0};
        uint64_t ackedOrders{0};
        uint64_t canceledOrders{0};
    } stats;
};