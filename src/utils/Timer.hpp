#pragma once
#include <cstdint>
#include <chrono>
#include <thread>

class Timer
{
public:
    static inline uint64_t rdtsc()
    {
        unsigned int lo, hi;
        __asm__ __volatile__ ("rdtsc" : "=a" (lo), "=d" (hi));
        return ((uint64_t)hi << 32) | lo;
    }

    static uint64_t cycles_to_ns(uint64_t cycles)
    {
        static const double ns_per_cycle = EstimateNsPerCycle();
        return static_cast<uint64_t>(cycles * ns_per_cycle);
    }

private:
    static double EstimateNsPerCycle()
    {
        auto start_time = std::chrono::steady_clock::now();
        uint64_t start_cycle = rdtsc();

        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        auto end_time = std::chrono::steady_clock::now();
        uint64_t end_cycle = rdtsc();

        auto duration_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time).count();
        uint64_t cycles = end_cycle - start_cycle;

        return static_cast<double>(duration_ns) / static_cast<double>(cycles);
    }
};
