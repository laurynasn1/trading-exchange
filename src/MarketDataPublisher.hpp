#pragma once
#include "SPSCQueue.hpp"
#include "OrderBook.hpp"
#include <thread>
#include <atomic>
#include <iostream>
#include <algorithm>
#include <iomanip>

class MarketDataPublisher {
private:
    std::shared_ptr<SPSCQueue<MarketDataEvent>> queue;
    std::thread thread;
    std::atomic<bool> running{ false };

public:
    MarketDataPublisher(std::shared_ptr<SPSCQueue<MarketDataEvent>> queue_, size_t numRequests)
        : queue(queue_)
    {
        receiveTimes.resize(numRequests);
        seenRequestIds.resize(numRequests);
    }

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
            MarketDataEvent* trade = queue->GetReadIndex();

            if (trade == nullptr)
            {
                std::this_thread::yield();
                continue;
            }

            Publish(*trade);
            eventsProcessed++;

            queue->UpdateReadIndex();
        }

        std::cout << "Market data publisher processed " << eventsProcessed << " events\n";
    }

private:
    void Publish(const MarketDataEvent& trade)
    {
        if (!seenRequestIds[trade.requestId])
        {
            receiveTimes[trade.requestId] = Timer::rdtsc();
            seenRequestIds[trade.requestId] = true;
        }

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
        case EventType::ORDER_REJECTED:
            stats.rejectedOrders++;
            break;
        default:
            break;
        }
    }

public:
    std::vector<uint64_t> receiveTimes;
    std::vector<bool> seenRequestIds;

    struct Stats
    {
        uint64_t filledOrders{0};
        uint64_t ackedOrders{0};
        uint64_t canceledOrders{0};
        uint64_t rejectedOrders{0};
    } stats;
};