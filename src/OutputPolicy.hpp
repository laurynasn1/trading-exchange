#pragma once

#include <memory>
#include <thread>
#include <vector>
#include <immintrin.h>

#include "MarketDataEvent.hpp"
#include "SPSCQueue.hpp"

struct NoOpOutputPolicy
{
    void OnMarketEvent(const MarketDataEvent & event) {}
};

struct VectorOutputPolicy
{
    std::vector<MarketDataEvent> events;
    void OnMarketEvent(const MarketDataEvent & event)
    {
        events.emplace_back(event);
    }
};

struct QueueOutputPolicy
{
    std::shared_ptr<SPSCQueue<MarketDataEvent>> queue;

    QueueOutputPolicy(std::shared_ptr<SPSCQueue<MarketDataEvent>> queue_)
        : queue(queue_) {}

    void OnMarketEvent(const MarketDataEvent & event)
    {
        MarketDataEvent* slot = nullptr;
        while ((slot = queue->GetWriteIndex()) == nullptr)
            _mm_pause();
        *slot = event;
        queue->UpdateWriteIndex();
    }
};

