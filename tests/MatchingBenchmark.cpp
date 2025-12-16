#include <iostream>
#include <chrono>
#include <thread>
#include <random>

#include <benchmark/benchmark.h>

#include "MatchingEngine.hpp"
#include "Timer.hpp"

static void BM_InsertOrderFixed(benchmark::State& state)
{
    NoOpOutputPolicy output;
    MatchingEngine engine(output);
    uint64_t orderId = 0;

    for (auto _ : state)
    {
        Order order(orderId, 0, Side::SELL, OrderType::LIMIT, 100, 15000);

        uint64_t start = Timer::rdtsc();
        engine.SubmitOrder(&order);
        uint64_t end = Timer::rdtsc();

        benchmark::DoNotOptimize(order);
        state.SetIterationTime(Timer::cycles_to_ns(end - start) / 1e9);
        orderId++;
    }
}

BENCHMARK(BM_InsertOrderFixed)->UseManualTime()->Iterations(1000000);

static void BM_InsertOrderRandom(benchmark::State& state)
{
    NoOpOutputPolicy output;
    MatchingEngine engine(output);
    uint64_t orderId = 0;

    std::vector<uint32_t> prices;
    prices.reserve(1000000);
    for (int i = 1; i <= 1000000; i++)
        prices.emplace_back(i);

    std::random_device rd;
    std::mt19937 gen(rd());

    std::shuffle(prices.begin(), prices.end(), gen);

    for (auto _ : state)
    {
        Order order(orderId, 0, Side::SELL, OrderType::LIMIT, 100, prices[orderId]);

        uint64_t start = Timer::rdtsc();
        engine.SubmitOrder(&order);
        uint64_t end = Timer::rdtsc();

        benchmark::DoNotOptimize(order);
        state.SetIterationTime(Timer::cycles_to_ns(end - start) / 1e9);
        orderId++;
    }
}

BENCHMARK(BM_InsertOrderRandom)->UseManualTime()->Iterations(1000000);

static void BM_MatchSingle(benchmark::State& state)
{
    NoOpOutputPolicy output;
    MatchingEngine engine(output);

    uint64_t orderId = 0;

    for (auto _ : state)
    {
        Order sell(orderId++, 0, Side::SELL, OrderType::LIMIT, 100, 15000);
        engine.SubmitOrder(&sell);

        Order buy(orderId++, 0, Side::BUY, OrderType::LIMIT, 100, 15000);

        uint64_t start = Timer::rdtsc();
        engine.SubmitOrder(&buy);
        uint64_t end = Timer::rdtsc();

        benchmark::DoNotOptimize(buy);
        state.SetIterationTime(Timer::cycles_to_ns(end - start) / 1e9);
    }
}

BENCHMARK(BM_MatchSingle)->UseManualTime()->Iterations(10000000);

static void BM_MatchOrder(benchmark::State& state)
{
    NoOpOutputPolicy output;
    MatchingEngine engine(output);

    const size_t totalOrders = 100;
    auto levelsToSweep = state.range(0);
    auto ordersPerLevel = totalOrders / levelsToSweep;
    uint32_t quantityPerLevel = 10;
    uint32_t totalQty = levelsToSweep * ordersPerLevel * quantityPerLevel;

    uint64_t buy_id = 1000000;

    for (auto _ : state)
    {
        for (int i = 0; i < levelsToSweep; ++i)
        {
            for (int j = 0; j < ordersPerLevel; j++)
            {
                Order sell(i * ordersPerLevel + j, 0, Side::SELL, OrderType::LIMIT, quantityPerLevel, 15000u + i);
                engine.SubmitOrder(&sell);
            }
        }

        Order buy(buy_id++, 0, Side::BUY, OrderType::MARKET, totalQty, 0);

        uint64_t start = Timer::rdtsc();
        engine.SubmitOrder(&buy);
        uint64_t end = Timer::rdtsc();

        state.SetIterationTime(Timer::cycles_to_ns(end - start) / 1e9);
    }
}

BENCHMARK(BM_MatchOrder)->UseManualTime()->Arg(1)->Arg(10)->Arg(100);

static void BM_CancelOrder(benchmark::State& state)
{
    NoOpOutputPolicy output;
    MatchingEngine engine(output);

    auto levels = state.range(0);
    size_t ordersPerLevel = 1'000'000 / levels;
    std::vector<int> indices;
    indices.reserve(levels * ordersPerLevel);
    for (int i = 0; i < levels; ++i)
    {
        for (int j = 0; j < ordersPerLevel; j++)
        {
            Order order(i * ordersPerLevel + j, 0, Side::SELL, OrderType::LIMIT, 100, (uint32_t) i + 1);
            engine.SubmitOrder(&order);
            indices.push_back(i * ordersPerLevel + j);
        }
    }

    std::random_device rd;
    std::mt19937 gen(rd());

    std::shuffle(indices.begin(), indices.end(), gen);

    int index = 0;
    for (auto _ : state)
    {
        uint64_t start = Timer::rdtsc();
        auto result = engine.CancelOrder(indices[index++]);
        uint64_t end = Timer::rdtsc();

        benchmark::DoNotOptimize(result);
        state.SetIterationTime(Timer::cycles_to_ns(end - start) / 1e9);
    }
}

BENCHMARK(BM_CancelOrder)->UseManualTime()->Arg(1)->Arg(1000)->Arg(1000000)->Iterations(1000000);

BENCHMARK_MAIN();