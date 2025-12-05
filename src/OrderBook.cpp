#include "OrderBook.hpp"
#include "Timer.hpp"
#include <iostream>

OrderBook::OrderBook(std::string symbol_) : symbol(std::move(symbol_))
{
    orders.reserve(1'000'000);
}

template<typename BookSide>
void RemoveOrder(std::shared_ptr<Order> order, BookSide & bookSide)
{
    auto& orderList = bookSide[order->price];

    orderList.erase(order->position);
    if (orderList.empty())
        bookSide.erase(order->price);
}

MarketDataEvent OrderBook::AddOrder(std::shared_ptr<Order> order)
{
    orders[order->orderId] = order;

    if (order->side == Side::BUY)
    {
        bids[order->price].push_back(order);
        order->position = std::prev(bids[order->price].end());
    }
    else
    {
        asks[order->price].push_back(order);
        order->position = std::prev(asks[order->price].end());
    }

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
    auto it = orders.find(targetOrderId);
    if (it == orders.end())
        return MarketDataEvent
        {
            .type = EventType::ORDER_REJECTED,
            .orderId = targetOrderId,
            .requestId = requestId,
            .timestamp = Timer::rdtsc(),
            .rejectionReason = "Order not found"
        };

    auto order = it->second;

    if (order->side == Side::BUY)
        RemoveOrder(order, bids);
    else
        RemoveOrder(order, asks);

    orders.erase(targetOrderId);

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

    if (order->side == Side::BUY)
    {
        if (order->type == OrderType::FOK && !CheckAvailableLiquidity(order, asks))
            return {};

        events = MatchAgainstBook(order, asks);
    }
    else
    {
        if (order->type == OrderType::FOK && !CheckAvailableLiquidity(order, bids))
            return {};

        events = MatchAgainstBook(order, bids);
    }

    if (order->RemainingQuantity() > 0 && order->type == OrderType::LIMIT)
        events.push_back(AddOrder(order));

    return events;
}

template<typename BookSide>
std::vector<MarketDataEvent> OrderBook::MatchAgainstBook(std::shared_ptr<Order> order, BookSide &bookSide)
{
    std::vector<MarketDataEvent> events;
    events.reserve(1000);

    bool isBuy = order->side == Side::BUY;

    auto priceIt = bookSide.begin();
    while (priceIt != bookSide.end() && order->RemainingQuantity() > 0)
    {
        double restingPrice = priceIt->first;

        if (isBuy && order->price > 0 && restingPrice > order->price)
            break;
        if (!isBuy && order->price > 0 && restingPrice < order->price)
            break;

        auto &ordersAtPrice = priceIt->second;
        auto restingIt = ordersAtPrice.begin();
        while (restingIt != ordersAtPrice.end() && order->RemainingQuantity() > 0)
        {
            auto resting = *restingIt;

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

            if (resting->RemainingQuantity() == 0)
            {
                restingIt = ordersAtPrice.erase(restingIt);
                orders.erase(resting->orderId);
            }
            else
                restingIt++;
        }

        if (ordersAtPrice.empty())
            priceIt = bookSide.erase(priceIt);
        else
            priceIt++;
    }

    return events;
}

template<typename BookSide>
bool OrderBook::CheckAvailableLiquidity(std::shared_ptr<Order> order, BookSide& bookSide)
{
    uint32_t availableShares = 0;
    bool isBuy = order->side == Side::BUY;

    for (auto priceIt = bookSide.begin(); priceIt != bookSide.end() && availableShares < order->quantity; priceIt++)
    {
        double restingPrice = priceIt->first;

        if (isBuy && order->price > 0 && restingPrice > order->price)
            break;
        if (!isBuy && order->price > 0 && restingPrice < order->price)
            break;

        auto &ordersAtPrice = priceIt->second;
        for (auto restingIt = ordersAtPrice.begin(); restingIt != ordersAtPrice.end() && availableShares < order->quantity; restingIt++)
            availableShares += (*restingIt)->quantity;
    }
    return availableShares >= order->quantity;
}

std::pair<double, double> OrderBook::GetTopOfBook() const
{
    double bestBid = bids.empty() ? 0.0 : bids.begin()->first;
    double bestAsk = asks.empty() ? 0.0 : asks.begin()->first;
    return { bestBid, bestAsk };
}

void OrderBook::PrintBook(int levels) const
{
    std::cout << "=== Order Book: " << symbol << " ===\n";

    auto ask_it = asks.rbegin();
    for (int i = 0; i < levels && ask_it != asks.rend(); ++i, ++ask_it)
    {
        int total_qty = 0;
        for (const auto& order : ask_it->second)
            total_qty += order->RemainingQuantity();

        std::cout << "  ASK: " << ask_it->first << " | " << total_qty << " shares\n";
    }
    
    auto [bid, ask] = GetTopOfBook();
    std::cout << "  --- SPREAD: " << (ask - bid) << " ---\n";

    auto bid_it = bids.begin();
    for (int i = 0; i < levels && bid_it != bids.end(); ++i, ++bid_it)
    {
        int total_qty = 0;
        for (const auto& order : bid_it->second)
            total_qty += order->RemainingQuantity();

        std::cout << "  BID: " << bid_it->first << " | " << total_qty << " shares\n";
    }
    std::cout << "========================\n";
}

void OrderBook::Clear()
{
    bids.clear();
    asks.clear();
    orders.clear();
}
