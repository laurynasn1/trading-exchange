#pragma once
#include <cstdint>
#include <string>
#include "Timer.hpp"

enum class EventType
{
    ORDER_ACKED,
    ORDER_FILLED,
    ORDER_CANCELLED,
    ORDER_REJECTED
};

enum class RejectionType
{
    NONE,
    INVALID_QUANTITY,
    INVALID_PRICE,
    ORDER_NOT_FOUND,
};

struct MarketDataEvent
{
    EventType type;
    uint64_t orderId;
    uint64_t requestId;
    uint64_t timestamp;

    uint64_t tradeId;
    uint64_t restingOrderId;
    uint32_t price;
    uint32_t quantity;

    RejectionType rejectionReason;

    MarketDataEvent() {}

    MarketDataEvent(uint64_t oId, uint64_t rId, uint32_t p, uint32_t q)
        : type(EventType::ORDER_ACKED), orderId(oId), requestId(rId), price(p), quantity(q), timestamp(Timer::rdtsc()) {}

    MarketDataEvent(uint64_t oId, uint64_t rId, uint64_t tId, uint64_t roId, uint32_t p, uint32_t q)
        : type(EventType::ORDER_FILLED), orderId(oId), requestId(rId), tradeId(tId), restingOrderId(rId), price(p), quantity(q), timestamp(Timer::rdtsc()) {}

    MarketDataEvent(uint64_t oId, uint64_t rId)
        : type(EventType::ORDER_CANCELLED), orderId(oId), requestId(rId), timestamp(Timer::rdtsc()) {}

    MarketDataEvent(uint64_t oId, uint64_t rId, RejectionType rej)
        : type(EventType::ORDER_REJECTED), orderId(oId), requestId(rId), rejectionReason(rej), timestamp(Timer::rdtsc()) {}
};