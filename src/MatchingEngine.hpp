#pragma once

#include <unordered_map>
#include <thread>
#include <functional>

#include "OrderBook.hpp"
#include "SPSCQueue.hpp"
#include "SymbolMap.hpp"
#include "OutputPolicy.hpp"
#include "Timer.hpp"
#include "Threading.hpp"

template<typename OutputPolicy>
class MatchingEngine {
private:
    std::unordered_map<std::string, uint8_t> symbolMap;
    std::vector<std::unique_ptr<OrderBook>> books;

    std::vector<char> orderToSymbol;

    std::shared_ptr<SPSCQueue<OrderRequest>> inputQueue;
    OutputPolicy & output;

    std::thread thread;
    std::atomic<bool> running{ false };

    RejectionType ValidateOrder(const Order& order)
    {
        if (order.quantity == 0)
            return RejectionType::INVALID_QUANTITY;

        if (order.type == OrderType::LIMIT && order.price <= 0)
            return RejectionType::INVALID_PRICE;

        return RejectionType::NONE;
    }

public:
    MatchingEngine(std::shared_ptr<SPSCQueue<OrderRequest>> input, OutputPolicy & output_, size_t numBooks, size_t maxNumOrders)
        : symbolMap(LoadSymbolMap()), inputQueue(input), output(output_)
    {
        orderToSymbol.resize(maxNumOrders, -1);
        books.reserve(numBooks);
        auto it = symbolMap.begin();
        for (int i = 0; i < numBooks && it != symbolMap.end(); i++, it++)
            books.emplace_back(std::make_unique<OrderBook>(it->first, maxNumOrders));
    }

    MatchingEngine(OutputPolicy & output_, size_t numBooks = 1, size_t maxNumOrders = 20'000'000)
        : MatchingEngine(nullptr, output_, numBooks, maxNumOrders) {}

    ~MatchingEngine()
    {
        Stop();
    }

    void SubmitOrder(Order* order)
    {
        if (order->orderId >= orderToSymbol.size())
            throw std::runtime_error{ "Order id exceeds capacity" };

        RejectionType rejection = ValidateOrder(*order);
        if (rejection > RejectionType::NONE)
        {
            output.OnMarketEvent(MarketDataEvent{
                    .type = EventType::ORDER_REJECTED,
                    .orderId = order->orderId,
                    .requestId = order->orderId,
                    .timestamp = Timer::rdtsc(),
                    .rejectionReason = rejection
                });
            return;
        }

        auto book = GetBook(order->symbolId);
        book->MatchOrder(order, output);
        orderToSymbol[order->orderId] = order->symbolId;
    }

    bool CancelOrder(uint64_t targetOrderId, uint64_t requestId = 0)
    {
        if (targetOrderId >= orderToSymbol.size())
            throw std::runtime_error{ "Order id exceeds capacity" };

        if (orderToSymbol[targetOrderId] < 0)
            return false;

        auto book = GetBook(orderToSymbol[targetOrderId]);
        book->CancelOrder(targetOrderId, requestId, output);
        return true;
    }

    OrderBook* GetBook(uint8_t symbolId)
    {
        if (symbolId < 0 || symbolId >= books.size())
            throw std::runtime_error{ "Invalid symbol id" };
        return books[symbolId].get();
    }

    void Start()
    {
        running = true;
        thread = std::thread(&MatchingEngine::Run, this);
    }

    void Stop()
    {
        running = false;
        if (thread.joinable())
            thread.join();
    }

    void Run()
    {
        PinThread(3);
        while (running)
        {
            OrderRequest* req = inputQueue->GetReadIndex();

            if (req == nullptr)
            {
                _mm_pause();
                continue;
            }

            if (Order* order = std::get_if<Order>(&req->data))
            {
                SubmitOrder(order);
            }
            else if (CancelRequest* cancelReq = std::get_if<CancelRequest>(&req->data))
            {
                CancelOrder(cancelReq->targetOrderId, cancelReq->requestId);
            }

            inputQueue->UpdateReadIndex();
        }
    }
};