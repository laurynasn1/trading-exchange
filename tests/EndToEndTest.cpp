#include <gtest/gtest.h>
#include <thread>
#include <chrono>

#include "MatchingEngine.hpp"
#include "OrderGateway.hpp"
#include "MarketDataPublisher.hpp"
#include "SPSCQueue.hpp"
#include "LatencyStats.hpp"

TEST(EndToEndTest, ThroughputTest)
{
    const size_t NUM_ORDERS = 2'000'000;
    const size_t QUEUE_SIZE = 2'000'000;
    auto inputQueue = std::make_shared<SPSCQueue<OrderRequest>>(QUEUE_SIZE);
    auto outputQueue = std::make_shared<SPSCQueue<MarketDataEvent>>(QUEUE_SIZE);

    OrderGateway gateway(inputQueue, NUM_ORDERS);
    MatchingEngine engine(inputQueue, [outputQueue](const auto & event) {
        MarketDataEvent* slot = nullptr;
        while ((slot = outputQueue->GetWriteIndex()) == nullptr)
            _mm_pause();
        *slot = event;
        outputQueue->UpdateWriteIndex();
    });
    MarketDataPublisher publisher(outputQueue, NUM_ORDERS);

    auto start = std::chrono::steady_clock::now();

    publisher.Start();
    engine.Start();
    gateway.Start();

    gateway.WaitUntilFinished();
    while (!inputQueue->IsEmpty());
    engine.Stop();
    while (!outputQueue->IsEmpty());
    publisher.Stop();

    auto end = std::chrono::steady_clock::now();
    auto durationMs = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

    std::cout << "Duration: " << durationMs << " ms\n";
    std::cout << "Orders acked: " << publisher.stats.ackedOrders << "\n";
    std::cout << "Orders filled: " << publisher.stats.filledOrders << "\n";
    std::cout << "Orders canceled: " << publisher.stats.canceledOrders << "\n";
    std::cout << "Orders rejected: " << publisher.stats.rejectedOrders << "\n";
    std::cout << "Throughput: " << (NUM_ORDERS * 1000.0 / durationMs) << " orders/sec\n";
}

TEST(EndToEndTest, LatencyTest)
{
    const size_t NUM_ORDERS = 200'000;
    const size_t QUEUE_SIZE = 100;
    auto inputQueue = std::make_shared<SPSCQueue<OrderRequest>>(QUEUE_SIZE);
    auto outputQueue = std::make_shared<SPSCQueue<MarketDataEvent>>(QUEUE_SIZE);

    OrderGateway gateway(inputQueue, NUM_ORDERS);
    MatchingEngine engine(inputQueue, [outputQueue](const auto & event) {
        MarketDataEvent* slot = nullptr;
        while ((slot = outputQueue->GetWriteIndex()) == nullptr)
            _mm_pause();
        *slot = event;
        outputQueue->UpdateWriteIndex();
    });
    MarketDataPublisher publisher(outputQueue, NUM_ORDERS);

    LatencyStats latencyStats;

    publisher.Start();
    engine.Start();

    for (int i = 0; i < NUM_ORDERS; i++)
    {
        while (!gateway.SendRequest(i))
            _mm_pause();

        while (!publisher.HasProcessed(i))
            _mm_pause();

        if (i >= 10000) // Warmup
            latencyStats.record(publisher.receiveTimes[i] - gateway.requestTimes[i]);
    }

    engine.Stop();
    publisher.Stop();

    std::cout << "Orders acked: " << publisher.stats.ackedOrders << "\n";
    std::cout << "Orders filled: " << publisher.stats.filledOrders << "\n";
    std::cout << "Orders canceled: " << publisher.stats.canceledOrders << "\n";
    std::cout << "Orders rejected: " << publisher.stats.rejectedOrders << "\n";

    latencyStats.print_stats();
}
