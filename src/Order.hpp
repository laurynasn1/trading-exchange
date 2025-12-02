#pragma once
#include <string>
#include <cstdint>
#include <chrono>
#include <list>
#include <memory>
#include <variant>

enum class Side
{
    BUY,
    SELL
};

enum class OrderType
{
    MARKET,
    LIMIT,
    IOC,
    FOK
};

struct Order
{
    uint64_t orderId;
    std::string symbol;
    Side side;
    OrderType type;
    uint32_t quantity;
    double price;
    uint64_t timestamp;

    uint32_t filledQuantity = 0;
    std::list<std::shared_ptr<Order>>::iterator position;

    Order() {}

    Order(uint64_t id, std::string sym, Side s, OrderType t, uint32_t qty, double p)
        : orderId(id), symbol(std::move(sym)), side(s), type(t), quantity(qty), price(p)
    {
        auto now = std::chrono::high_resolution_clock::now();
        timestamp = std::chrono::duration_cast<std::chrono::nanoseconds>(now.time_since_epoch()).count();
    }

    uint32_t RemainingQuantity() const
    {
        return quantity - filledQuantity;
    }

    bool IsFilled() const
    {
        return filledQuantity >= quantity;
    }
};

struct CancelRequest
{
    uint64_t requestId;
    uint64_t targetOrderId;
    uint64_t timestamp;
};

struct OrderRequest
{
    std::variant<Order, CancelRequest> data;
};