#pragma once
#include <cstdint>

class Timer {
public:
    static uint64_t rdtsc() {
        unsigned int lo, hi;
        __asm__ __volatile__ ("rdtsc" : "=a" (lo), "=d" (hi));
        return ((uint64_t)hi << 32) | lo;
    }
    
    static uint64_t cycles_to_ns(uint64_t cycles, double cpu_freq_ghz = 3.4) {
        return cycles / cpu_freq_ghz;
    }
    
    // Calibrate CPU frequency
    static double calibrate_cpu_freq();
};
