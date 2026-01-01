#pragma once
#include <cstdint>

#pragma pack(push, 1)

struct ItchHeader
{
    uint64_t sequenceNumber;
    char messageType;
    uint64_t timestamp;
};

struct OrderAddMsg
{
    ItchHeader header;
    uint64_t orderId;
    uint8_t side;
    uint32_t quantity;
    char symbol[4];
    uint32_t price;
};

struct OrderExecMsg
{
    ItchHeader header;
    uint64_t orderId;
    uint32_t quantity;
    uint64_t matchId;
};

struct OrderDeleteMsg
{
    ItchHeader header;
    uint64_t orderId;
};

struct TradeMsg
{
    ItchHeader header;
    uint8_t side;
    uint32_t quantity;
    char symbol[4];
    uint32_t price;
    uint64_t matchId;
};

struct EndMarketMsg
{
    ItchHeader header;
};

#pragma pack(pop)