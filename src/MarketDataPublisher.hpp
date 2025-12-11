#pragma once
#include "SPSCQueue.hpp"
#include "OrderBook.hpp"
#include "Threading.hpp"

#include <thread>
#include <atomic>
#include <iostream>
#include <algorithm>
#include <iomanip>
#include <immintrin.h>

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
        for (int i = 0; i < numRequests; i++)
            seenRequestIds.emplace_back(false);
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
        PinThread(6);
        uint64_t eventsProcessed = 0;

        while (running)
        {
            MarketDataEvent* trade = queue->GetReadIndex();

            if (trade == nullptr)
            {
                _mm_pause();
                continue;
            }

            Publish(*trade);
            eventsProcessed++;

            queue->UpdateReadIndex();
        }

        std::cout << "Market data publisher processed " << eventsProcessed << " events\n";
    }

    bool HasProcessed(size_t index)
    {
        auto val = receiveTimes[index];
        if (val != 0)
        {
            std::atomic_thread_fence(std::memory_order_acquire);
            return true;
        }
        return false;
    }

private:
    void Publish(const MarketDataEvent& trade)
    {
        if (!seenRequestIds[trade.requestId])
        {
            receiveTimes[trade.requestId] = Timer::rdtsc();
            seenRequestIds[trade.requestId] = true;
            std::atomic_thread_fence(std::memory_order_release);
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