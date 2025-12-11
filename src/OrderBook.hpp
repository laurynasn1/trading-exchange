#pragma once
#include "Order.hpp"
#include "MarketDataEvent.hpp"
#include "ObjectPool.hpp"
#include "OutputPolicy.hpp"
#include "Timer.hpp"

#include <array>
#include <span>
#include <iostream>
#include <vector>

static constexpr double MAX_PRICE = 10000.0;
static constexpr double TICK_SIZE = 0.01;
static constexpr size_t NUM_PRICE_LEVELS = MAX_PRICE / TICK_SIZE + 1;

struct PriceLevel
{
    Order* head = nullptr;
    Order* tail = nullptr;
};

class OrderBook
{
private:
    std::string symbol;

    std::array<PriceLevel, NUM_PRICE_LEVELS> bids;
    std::array<PriceLevel, NUM_PRICE_LEVELS> asks;

    uint32_t maxBid = 0;
    uint32_t minAsk = NUM_PRICE_LEVELS;

    std::vector<Order*> orders;
    ObjectPool<Order> orderPool;

    uint64_t nextTradeId = 1;

    Order* RemoveOrder(Order* order, PriceLevel & level)
    {
        orders[order->orderId] = nullptr;

        if (order->prev) order->prev->next = order->next;
        if (order->next) order->next->prev = order->prev;

        if (level.head == order) level.head = order->next;
        if (level.tail == order) level.tail = order->prev;

        auto nextOrder = order->next;
        order->next = nullptr;
        order->prev = nullptr;

        orderPool.Deallocate(order);
        return nextOrder;
    }

    template<typename OutputPolicy>
    void AddOrder(Order* orderToAdd, OutputPolicy & output)
    {
        if (orderToAdd->orderId >= orders.size())
            throw std::runtime_error{ "Order id exceeds capacity" };

        Order* order = orderPool.Allocate();
        *order = *orderToAdd;

        orders[order->orderId] = order;

        PriceLevel & level = (order->side == Side::BUY ? bids[order->price] : asks[order->price]);

        if (level.head == nullptr)
        {
            level.head = order;
            level.tail = order;
            order->next = nullptr;
            order->prev = nullptr;
        }
        else
        {
            level.tail->next = order;
            order->prev = level.tail;
            order->next = nullptr;
            level.tail = order;
        }

        if (order->side == Side::BUY)
            maxBid = std::max(maxBid, order->price);
        else
            minAsk = std::min(minAsk, order->price);

        output.OnMarketEvent(MarketDataEvent{
            .type = EventType::ORDER_ACKED,
            .orderId = order->orderId,
            .requestId = order->orderId,
            .timestamp = Timer::rdtsc(),
            .price = order->price,
            .quantity = order->RemainingQuantity()
        });
    }

    template<typename OutputPolicy>
    void MatchAgainstBook(Order* order, std::span<PriceLevel> bookSide, uint32_t & topPrice, uint32_t endOfBook, char dir, bool shouldBeLess, OutputPolicy & output)
    {
        while (topPrice != endOfBook && order->RemainingQuantity() > 0)
        {
            if (order->price > 0 && topPrice != order->price && (topPrice < order->price) == shouldBeLess)
                break;

            auto & level = bookSide[topPrice];

            if (level.head == nullptr)
            {
                topPrice += dir;
                continue;
            }

            auto resting = level.head;
            while (resting != nullptr && order->RemainingQuantity() > 0)
            {
                uint32_t filledQty = std::min(order->RemainingQuantity(), resting->RemainingQuantity());

                order->filledQuantity += filledQty;
                resting->filledQuantity += filledQty;

                output.OnMarketEvent(MarketDataEvent{
                        .type = EventType::ORDER_FILLED,
                        .orderId = order->orderId,
                        .requestId = order->orderId,
                        .timestamp = Timer::rdtsc(),
                        .tradeId = nextTradeId++,
                        .restingOrderId = resting->orderId,
                        .price = order->price,
                        .quantity = filledQty
                    });

                if (resting->IsFilled())
                    resting = RemoveOrder(resting, level);
                else
                    resting = resting->next;
            }

            if (level.head == nullptr)
                topPrice += dir;
        }
    }

    bool CheckAvailableLiquidity(Order* order, std::span<PriceLevel> bookSide, uint32_t topPrice, uint32_t endOfBook, char dir, bool shouldBeLess)
    {
        uint32_t availableShares = 0;
        auto idx = topPrice;

        while (idx != endOfBook && availableShares < order->quantity)
        {
            if (order->price > 0 && idx != order->price && (idx < order->price) == shouldBeLess)
                break;

            auto & level = bookSide[idx];

            if (level.head == nullptr)
            {
                idx += dir;
                continue;
            }

            auto resting = level.head;
            while (resting != nullptr && availableShares < order->quantity)
            {
                availableShares += resting->RemainingQuantity();
                resting = resting->next;
            }

            idx += dir;
        }
        return availableShares >= order->quantity;
    }

public:
    OrderBook(std::string symbol_, size_t numMaxOrders) : symbol(std::move(symbol_)), orderPool(numMaxOrders)
    {
        orders.resize(numMaxOrders, nullptr);
    }

    template<typename OutputPolicy>
    void CancelOrder(uint64_t targetOrderId, OutputPolicy & output, uint64_t requestId)
    {
        if (orders[targetOrderId] == nullptr)
        {
            output.OnMarketEvent(MarketDataEvent{
                .type = EventType::ORDER_REJECTED,
                .orderId = targetOrderId,
                .requestId = requestId,
                .timestamp = Timer::rdtsc(),
                .rejectionReason = "Order not found"
            });
            return;
        }

        auto order = orders[targetOrderId];

        auto & level = (order->side == Side::BUY ? bids[order->price] : asks[order->price]);
        RemoveOrder(order, level);

        output.OnMarketEvent(MarketDataEvent{
            .type = EventType::ORDER_CANCELLED,
            .orderId = order->orderId,
            .requestId = requestId,
            .timestamp = Timer::rdtsc()
        });
    }

    template<typename OutputPolicy>
    void MatchOrder(Order* order, OutputPolicy & output)
    {
        if (order->side == Side::BUY)
        {
            if (order->type == OrderType::FOK && !CheckAvailableLiquidity(order, asks, minAsk, NUM_PRICE_LEVELS, 1, false))
                return;

            MatchAgainstBook(order, asks, minAsk, NUM_PRICE_LEVELS, 1, false, output);
        }
        else
        {
            if (order->type == OrderType::FOK && !CheckAvailableLiquidity(order, bids, maxBid, 0, -1, true))
                return;

            MatchAgainstBook(order, bids, maxBid, 0, -1, true, output);
        }

        if (order->RemainingQuantity() > 0)
        {
            if (order->type == OrderType::LIMIT)
                AddOrder(order, output);
            else
                output.OnMarketEvent(MarketDataEvent{
                    .type = EventType::ORDER_CANCELLED,
                    .orderId = order->orderId,
                    .requestId = order->orderId,
                    .timestamp = Timer::rdtsc()
                });
        }
    }

    std::pair<uint32_t, uint32_t> GetTopOfBook()
    {
        while (bids[maxBid].head == nullptr && maxBid > 0) maxBid--;
        while (asks[minAsk].head == nullptr && minAsk < NUM_PRICE_LEVELS) minAsk++;
        return { maxBid, minAsk };
    }

    void PrintBook(int levels = 5)
    {
        std::cout << "=== Order Book: " << symbol << " ===\n";

        auto askIdx = minAsk;
        for (int i = 0; i < levels && askIdx < NUM_PRICE_LEVELS; askIdx++)
        {
            int totalQty = 0;
            auto & level = asks[askIdx];
            if (level.head == nullptr)
                continue;

            i++;
            auto order = level.head;
            while (order != nullptr)
            {
                totalQty += order->RemainingQuantity();
                order = order->next;
            }

            std::cout << "  ASK: " << askIdx << " | " << totalQty << " shares\n";
        }

        auto [bid, ask] = GetTopOfBook();
        std::cout << "  --- SPREAD: " << (ask - bid) << " ---\n";

        auto bidIdx = maxBid;
        for (int i = 0; i < levels && bidIdx > 0; bidIdx--)
        {
            int totalQty = 0;
            auto & level = bids[bidIdx];
            if (level.head == nullptr)
                continue;

            i++;
            auto order = level.head;
            while (order != nullptr)
            {
                totalQty += order->RemainingQuantity();
                order = order->next;
            }

            std::cout << "  BID: " << bidIdx << " | " << totalQty << " shares\n";
        }
        std::cout << "========================\n";
    }
};