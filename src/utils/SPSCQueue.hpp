#pragma once

#include <atomic>
#include <cstddef>

template <class T>
class SPSCQueue
{
private:
    T* data;
    size_t size;
    std::atomic<size_t> head;
    std::atomic<size_t> tail;

public:
    SPSCQueue(size_t size_) : size(size_), head(0), tail(0)
    {
        data = new T[size];
    }

    ~SPSCQueue()
    {
        delete[] data;
    }

    T* GetWriteIndex()
    {
        auto currHead = head.load(std::memory_order_relaxed);
        auto nextHead = currHead + 1;
        if (nextHead >= size)
            nextHead = 0;

        if (nextHead == tail.load(std::memory_order_acquire))
            return nullptr;

        return &data[currHead];
    }

    void UpdateWriteIndex()
    {
        auto currHead = head.load(std::memory_order_relaxed);
        auto nextHead = currHead + 1;
        if (nextHead >= size)
            nextHead = 0;
        head.store(nextHead, std::memory_order_release);
    }

    T* GetReadIndex()
    {
        auto currTail = tail.load(std::memory_order_relaxed);

        if (currTail == head.load(std::memory_order_acquire))
            return nullptr;

        return &data[currTail];
    }

    void UpdateReadIndex()
    {
        auto nextTail = tail.load(std::memory_order_relaxed) + 1;
        if (nextTail >= size)
            nextTail = 0;
        tail.store(nextTail, std::memory_order_release);
    }

    bool IsEmpty()
    {
        return tail.load() == head.load();
    }
};