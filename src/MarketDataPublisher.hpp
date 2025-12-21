#pragma once
#include "SPSCQueue.hpp"
#include "OrderBook.hpp"
#include "Threading.hpp"
#include "UDPTransmitter.hpp"

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

    UDPTransmitter udpTransmitter;

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
            MarketDataEvent* event = queue->GetReadIndex();

            if (event == nullptr)
            {
                _mm_pause();
                continue;
            }

            Publish(*event);
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
    void Publish(const MarketDataEvent& event)
    {
        if (!seenRequestIds[event.requestId])
        {
            receiveTimes[event.requestId] = Timer::rdtsc();
            seenRequestIds[event.requestId] = true;
            std::atomic_thread_fence(std::memory_order_release);
        }

        switch (event.type)
        {
        case EventType::ORDER_ACKED:
            udpTransmitter.SendOrderAdd(event.orderId, SYMBOLS[event.symbolId], event.side == Side::BUY ? 'B' : 'S', event.price, event.quantity, event.timestamp);
            stats.ackedOrders++;
            break;
        case EventType::ORDER_FILLED:
            udpTransmitter.SendOrderExecuted(event.orderId, event.quantity, event.tradeId, event.timestamp);
            udpTransmitter.SendOrderExecuted(event.restingOrderId, event.quantity, event.tradeId, event.timestamp);
            udpTransmitter.SendTradeMessage(SYMBOLS[event.symbolId], event.side == Side::BUY ? 'B' : 'S', event.price, event.quantity, event.tradeId, event.timestamp);
            stats.filledOrders++;
            break;
        case EventType::ORDER_CANCELLED:
            udpTransmitter.SendOrderDeleted(event.orderId, event.timestamp);
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