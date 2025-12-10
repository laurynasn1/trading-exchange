#include "OrderBook.hpp"
#include "Timer.hpp"
#include <iostream>

OrderBook::OrderBook(std::string symbol_, size_t numMaxOrders) : symbol(std::move(symbol_))
{
    orders.resize(numMaxOrders, nullptr);
}

std::shared_ptr<Order> RemoveOrder(std::shared_ptr<Order> order, PriceLevel & level)
{
    if (order->prev) order->prev->next = order->next;
    if (order->next) order->next->prev = order->prev;

    if (level.head == order) level.head = order->next;
    if (level.tail == order) level.tail = order->prev;

    auto nextOrder = order->next;
    order->next = nullptr;
    order->prev = nullptr;
    return nextOrder;
}

MarketDataEvent OrderBook::AddOrder(std::shared_ptr<Order> order)
{
    if (order->orderId >= orders.size())
        throw std::runtime_error{ "Order id exceeds capacity" };

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

    return MarketDataEvent
    {
        .type = EventType::ORDER_ACKED,
        .orderId = order->orderId,
        .requestId = order->orderId,
        .timestamp = Timer::rdtsc(),
        .price = order->price,
        .quantity = order->RemainingQuantity()
    };
}

MarketDataEvent OrderBook::CancelOrder(uint64_t targetOrderId, uint64_t requestId)
{
    if (orders[targetOrderId] == nullptr)
        return MarketDataEvent
        {
            .type = EventType::ORDER_REJECTED,
            .orderId = targetOrderId,
            .requestId = requestId,
            .timestamp = Timer::rdtsc(),
            .rejectionReason = "Order not found"
        };

    auto order = orders[targetOrderId];

    auto & level = (order->side == Side::BUY ? bids[order->price] : asks[order->price]);
    RemoveOrder(order, level);

    orders[targetOrderId] = nullptr;

    return MarketDataEvent
    {
        .type = EventType::ORDER_CANCELLED,
        .orderId = order->orderId,
        .requestId = requestId,
        .timestamp = Timer::rdtsc()
    };
}

std::vector<MarketDataEvent> OrderBook::MatchOrder(std::shared_ptr<Order> order)
{
    std::vector<MarketDataEvent> events;
    events.reserve(1000);

    if (order->side == Side::BUY)
    {
        if (order->type == OrderType::FOK && !CheckAvailableLiquidity(order, asks, minAsk, NUM_PRICE_LEVELS, 1, false))
            return events;

        MatchAgainstBook(order, asks, minAsk, NUM_PRICE_LEVELS, 1, false, events);
    }
    else
    {
        if (order->type == OrderType::FOK && !CheckAvailableLiquidity(order, bids, maxBid, 0, -1, true))
            return events;

        MatchAgainstBook(order, bids, maxBid, 0, -1, true, events);
    }

    if (order->RemainingQuantity() > 0 && order->type == OrderType::LIMIT)
        events.push_back(AddOrder(order));

    return events;
}

void OrderBook::MatchAgainstBook(std::shared_ptr<Order> order, std::span<PriceLevel> bookSide, uint32_t & topPrice, uint32_t endOfBook, char dir, bool shouldBeLess, std::vector<MarketDataEvent> & events)
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

            events.push_back(MarketDataEvent {
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
            {
                orders[resting->orderId] = nullptr;
                resting = RemoveOrder(resting, level);
            }
            else
                resting = resting->next;
        }

        if (level.head == nullptr)
            topPrice += dir;
    }
}

bool OrderBook::CheckAvailableLiquidity(std::shared_ptr<Order> order, std::span<PriceLevel> bookSide, uint32_t topPrice, uint32_t endOfBook, char dir, bool shouldBeLess)
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

std::pair<uint32_t, uint32_t> OrderBook::GetTopOfBook()
{
    while (bids[maxBid].head == nullptr && maxBid > 0) maxBid--;
    while (asks[minAsk].head == nullptr && minAsk < NUM_PRICE_LEVELS) minAsk++;
    return { maxBid, minAsk };
}

void OrderBook::PrintBook(int levels)
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
