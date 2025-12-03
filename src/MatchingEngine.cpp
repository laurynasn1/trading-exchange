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

    auto book = GetOrCreateBook(order->symbol);
    auto events = book->MatchOrder(order);
    if (eventCallback)
        for (const auto & event : events)
            eventCallback(event);
}

bool MatchingEngine::CancelOrder(uint64_t targetOrderId, uint64_t requestId)
{
    for (auto& [symbol, book] : books)
    {
        auto event = book->CancelOrder(requestId, targetOrderId);
        if (eventCallback) eventCallback(event);
        return true;
    }
    return false;
}

OrderBook* MatchingEngine::GetOrCreateBook(const std::string & symbol)
{
    auto it = books.find(symbol);
    if (it == books.end())
    {
        auto book = std::make_unique<OrderBook>(symbol);
        auto ptr = book.get();
        books[symbol] = std::move(book);
        return ptr;
    }
    return it->second.get();
}

OrderBook* MatchingEngine::GetBook(const std::string & symbol)
{
    auto it = books.find(symbol);
    return it == books.end() ? nullptr : it->second.get();
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

    if (order.symbol.empty())
    {
        error = "Symbol cannot be empty";
        return false;
    }

    return true;
}

void MatchingEngine::Clear()
{
    books.clear();
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