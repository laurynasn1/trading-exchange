#pragma once
#include <vector>
#include <algorithm>
#include <cstdint>
#include <iostream>
#include <iomanip>
#include <numeric>
#include "Timer.hpp"

class LatencyStats
{
public:
    void record(uint64_t cycles)
    {
        samples.push_back(cycles);
    }

    void print_stats()
    {
        std::sort(samples.begin(), samples.end());

        double min = samples.front();
        double max = samples.back();
        double mean = std::accumulate(samples.begin(), samples.end(), 0.0) / samples.size();
        double median = samples[samples.size() / 2];
        double p95 = samples[(size_t)(samples.size() * 0.95)];
        double p99 = samples[(size_t)(samples.size() * 0.99)];
        double p999 = samples[(size_t)(samples.size() * 0.999)];

        std::cout << std::fixed << std::setprecision(2);
        std::cout << "Min:    " << std::setw(10) << Timer::cycles_to_ns(min) << " ns\n";
        std::cout << "Mean:   " << std::setw(10) << Timer::cycles_to_ns(mean) << " ns\n";
        std::cout << "Median: " << std::setw(10) << Timer::cycles_to_ns(median) << " ns\n";
        std::cout << "P95:    " << std::setw(10) << Timer::cycles_to_ns(p95) << " ns\n";
        std::cout << "P99:    " << std::setw(10) << Timer::cycles_to_ns(p99) << " ns\n";
        std::cout << "P99.9:  " << std::setw(10) << Timer::cycles_to_ns(p999) << " ns\n";
        std::cout << "Max:    " << std::setw(10) << Timer::cycles_to_ns(max) << " ns\n";
    }

private:
    std::vector<uint64_t> samples;
};