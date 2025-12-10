#include "MatchingEngine.hpp"
#include "Timer.hpp"
#include <stdexcept>
#include <immintrin.h>

void MatchingEngine::SubmitOrder(std::shared_ptr<Order> order)
{
    std::string rejectionReason;
    if (!ValidateOrder(*order, rejectionReason))
    {
        if (eventCallback)
            eventCallback(MarketDataEvent{
                    .type = EventType::ORDER_REJECTED,
                    .orderId = order->orderId,
                    .requestId = order->orderId,
                    .timestamp = Timer::rdtsc(),
                    .rejectionReason = rejectionReason
                });
        return;
    }

    auto book = GetBook(order->symbolId);
    auto events = book->MatchOrder(order);
    if (eventCallback)
    {
        if (events.empty())
            eventCallback(MarketDataEvent{
                .type = EventType::ORDER_REJECTED,
                .orderId = order->orderId,
                .requestId = order->orderId,
                .rejectionReason = "Order was submitted but could not be filled"
            });
        else
            for (const auto & event : events)
                eventCallback(event);
    }

    orderToSymbol[order->orderId] = order->symbolId;
}

bool MatchingEngine::CancelOrder(uint64_t targetOrderId, uint64_t requestId)
{
    if (orderToSymbol[targetOrderId] < 0)
        return false;

    auto book = GetBook(orderToSymbol[targetOrderId]);
    auto event = book->CancelOrder(targetOrderId, requestId);
    if (eventCallback) eventCallback(event);
    return true;
}

OrderBook* MatchingEngine::GetBook(uint8_t symbolId)
{
    if (symbolId < 0 || symbolId >= books.size())
        throw std::runtime_error{ "Invalid symbol id" };
    return books[symbolId].get();
}

bool MatchingEngine::ValidateOrder(const Order& order, std::string& error)
{
    if (order.quantity == 0)
    {
        error = "Quantity must be > 0";
        return false;
    }

    if (order.type == OrderType::LIMIT && order.price <= 0)
    {
        error = "Limit order must have price > 0";
        return false;
    }

    return true;
}

void MatchingEngine::Start()
{
    running = true;
    thread = std::thread(&MatchingEngine::Run, this);
}

void MatchingEngine::Stop()
{
    running = false;
    if (thread.joinable())
        thread.join();
}

void MatchingEngine::Run()
{
    while (running)
    {
        OrderRequest* req = inputQueue->GetReadIndex();

        if (req == nullptr)
        {
            _mm_pause();
            continue;
        }

        if (Order* orderRaw = std::get_if<Order>(&req->data))
        {
            auto order = std::make_shared<Order>(*orderRaw);
            SubmitOrder(order);
        }
        else if (CancelRequest* cancelReq = std::get_if<CancelRequest>(&req->data))
        {
            CancelOrder(cancelReq->targetOrderId, cancelReq->requestId);
        }

        inputQueue->UpdateReadIndex();
    }
}