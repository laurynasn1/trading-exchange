#pragma once
#include <cstdint>
#include <string>

enum class EventType
{
    ORDER_ACKED,
    ORDER_FILLED,
    ORDER_CANCELLED,
    ORDER_REJECTED
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

    std::string rejectionReason;
};