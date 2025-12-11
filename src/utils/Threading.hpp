#pragma once

#include <thread>
#include <iostream>

#include <sched.h>
#include <pthread.h>

void PinThread(int cpuId)
{
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(cpuId, &cpuset);

    pthread_t thread = pthread_self();

    int rc = pthread_setaffinity_np(thread, sizeof(cpu_set_t), &cpuset);
    if (rc != 0)
        std::cerr << "Error calling pthread_setaffinity_np: " << rc << "\n";
}