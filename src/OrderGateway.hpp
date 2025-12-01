#pragma once

#include <memory>
#include <vector>
#include <random>
#include <thread>
#include <atomic>
#include <iostream>

#include "SPSCQueue.hpp"
#include "Order.hpp"
#include "Timer.hpp"

class OrderGateway
{
private:
    std::shared_ptr<SPSCQueue<OrderRequest>> queue;
    std::vector<OrderRequest> requests;
    std::vector<uint64_t> activeOrderIds;

    std::thread thread;
    std::atomic<bool> running{ false };

    uint64_t nextOrderId = 1;

public:
    OrderGateway(std::shared_ptr<SPSCQueue<OrderRequest>> queue_, size_t orders = 1'000'000) 
        : queue(queue_) 
    {
        generate_requests(orders);
    }

    ~OrderGateway()
    {
        Stop();
    }

    void generate_requests(size_t num_orders)
    {
        requests.reserve(num_orders);
        activeOrderIds.reserve(num_orders / 2);

        std::random_device rd;
        std::mt19937 gen(42);
        std::uniform_real_distribution<> type_dist(0, 1);
        std::uniform_real_distribution<> price_dist(149.00, 151.00);
        std::uniform_int_distribution<> qty_dist(100, 1000);
        std::uniform_int_distribution<> side_dist(0, 1);

        double mid_price = 150.00;

        for (size_t i = 0; i < num_orders; ++i)
        {
            double p = type_dist(gen);
            
            OrderRequest req;
            
            if (p < 0.10 && !activeOrderIds.empty())
            {
                // 10% Cancel orders

                std::uniform_int_distribution<> cancel_dist(0, activeOrderIds.size() - 1);
                size_t idx = cancel_dist(gen);
                req.data = activeOrderIds[idx];

                activeOrderIds[idx] = activeOrderIds.back();
                activeOrderIds.pop_back();
            }
            else if (p < 0.30)
            {
                // 20% Market orders
                req.data = Order(nextOrderId, "AAPL", side_dist(gen) == 0 ? Side::BUY : Side::SELL, OrderType::MARKET, qty_dist(gen), 0.0);
                activeOrderIds.push_back(nextOrderId);
                nextOrderId++;
            }
            else if (p < 0.60)
            {
                // 30% Limit orders that match immediately

                Side side = side_dist(gen) == 0 ? Side::BUY : Side::SELL;
                double aggressive_price;

                if (side == Side::BUY)
                    aggressive_price = mid_price + 0.05 + type_dist(gen) * 0.10;
                else
                    aggressive_price = mid_price - 0.05 - type_dist(gen) * 0.10;
                
                req.data = Order(nextOrderId, "AAPL", side, OrderType::LIMIT, qty_dist(gen), aggressive_price);
                activeOrderIds.push_back(nextOrderId);
                nextOrderId++;
            }
            else
            {
                // 40% Limit orders that rest

                Side side = side_dist(gen) == 0 ? Side::BUY : Side::SELL;
                double resting_price;

                if (side == Side::BUY)
                    resting_price = mid_price - 0.05 - type_dist(gen) * 0.50;
                else
                    resting_price = mid_price + 0.05 + type_dist(gen) * 0.50;

                req.data = Order(nextOrderId, "AAPL", side, OrderType::LIMIT, qty_dist(gen), resting_price);
                activeOrderIds.push_back(nextOrderId);
                nextOrderId++;
            }
            requests.push_back(req);
        }

        std::cout << "Generated " << requests.size() << " requests\n";
    }

    void Start()
    {
        running = true;
        thread = std::thread(&OrderGateway::Run, this);
    }

    void Stop()
    {
        running = false;
        WaitUntilFinished();
    }

    void WaitUntilFinished()
    {
        if (thread.joinable())
            thread.join();
    }

    void Run()
    {
        size_t sent = 0;
        auto start = std::chrono::steady_clock::now();

        for (const auto& req : requests)
        {
            if (!running) break;

            OrderRequest* slot = nullptr;
            while ((slot = queue->TryPut()) == nullptr)
                std::this_thread::yield();

            *slot = req;
            //std::get<Order>(slot->data).timestamp = Timer::rdtsc();

            sent++;
        }

        auto end = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();

        std::cout << "Gateway sent " << sent << " requests in " << duration << "us\n";
    }
};