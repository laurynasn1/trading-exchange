#pragma once

#include <vector>

template<typename T>
class ObjectPool
{
private:
    T* storage;
    std::vector<T*> freeList;

public:
    ObjectPool(size_t size)
    {
        storage = new T[size];
        freeList.reserve(size);
        for (int i = 0; i < size; i++)
            freeList.push_back(&storage[i]);
    }

    ~ObjectPool()
    {
        delete[] storage;
    }

    T* Allocate()
    {
        if (freeList.empty()) return nullptr;
        T* node = freeList.back();
        freeList.pop_back();
        return node;
    }

    void Deallocate(T* node)
    {
        freeList.push_back(node);
    }
};