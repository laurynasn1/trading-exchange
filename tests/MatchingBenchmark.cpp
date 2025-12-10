#include <iostream>
#include <chrono>
#include <thread>
#include <random>

#include <benchmark/benchmark.h>

#include "MatchingEngine.hpp"
#include "Timer.hpp"

static void BM_InsertOrder(benchmark::State& state)
{
    MatchingEngine engine;
    uint64_t orderId = 0;

    int delta = state.range(0);

    for (auto _ : state)
    {
        Order order{ orderId, 0, Side::SELL, OrderType::LIMIT, 100, orderId * delta + 1 };

        uint64_t start = Timer::rdtsc();
        engine.SubmitOrder(&order);
        uint64_t end = Timer::rdtsc();

        benchmark::DoNotOptimize(order);
        state.SetIterationTime(Timer::cycles_to_ns(end - start) / 1e9);
        orderId++;
    }
}

BENCHMARK(BM_InsertOrder)->UseManualTime()->Arg(0)->Arg(1)->Iterations(1000000);

static void BM_MatchSingle(benchmark::State& state)
{
    MatchingEngine engine;

    uint64_t orderId = 1;

    for (auto _ : state)
    {
        Order sell{ orderId++, 0, Side::SELL, OrderType::LIMIT, 100, 15000 };
        engine.SubmitOrder(&sell);

        Order buy{ orderId++, 0, Side::BUY, OrderType::LIMIT, 100, 15000 };

        uint64_t start = Timer::rdtsc();
        engine.SubmitOrder(&buy);
        uint64_t end = Timer::rdtsc();

        benchmark::DoNotOptimize(buy);
        state.SetIterationTime(Timer::cycles_to_ns(end - start) / 1e9);
    }
}

BENCHMARK(BM_MatchSingle)->UseManualTime();

static void BM_MatchOrder(benchmark::State& state)
{
    MatchingEngine engine;

    const int totalOrders = 100;
    int levelsToSweep = state.range(0);
    int ordersPerLevel = totalOrders / levelsToSweep;
    int quantityPerLevel = 10;
    int totalQty = levelsToSweep * ordersPerLevel * quantityPerLevel;

    uint64_t buy_id = 1000000;

    for (auto _ : state)
    {
        for (int i = 0; i < levelsToSweep; ++i)
        {
            for (int j = 0; j < ordersPerLevel; j++)
            {
                Order sell{ i * ordersPerLevel + j, 0, Side::SELL, OrderType::LIMIT, quantityPerLevel, 15000 + i };
                engine.SubmitOrder(&sell);
            }
        }

        Order buy{ buy_id++, 0, Side::BUY, OrderType::MARKET, totalQty, 0 };

        uint64_t start = Timer::rdtsc();
        engine.SubmitOrder(&buy);
        uint64_t end = Timer::rdtsc();

        state.SetIterationTime(Timer::cycles_to_ns(end - start) / 1e9);
    }
}

BENCHMARK(BM_MatchOrder)->UseManualTime()->Arg(1)->Arg(10)->Arg(100);

static void BM_CancelOrder(benchmark::State& state)
{
    MatchingEngine engine;

    int levels = state.range(0);
    int ordersPerLevel = 1'000'000 / levels;
    std::vector<int> indices;
    indices.reserve(levels * ordersPerLevel);
    for (int i = 0; i < levels; ++i)
    {
        for (int j = 0; j < ordersPerLevel; j++)
        {
            Order order{ i * ordersPerLevel + j, 0, Side::SELL, OrderType::LIMIT, 100, i + 1 };
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