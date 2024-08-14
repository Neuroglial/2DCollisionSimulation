#pragma once
#include <cstdint>
#include <mutex>

template<uint32_t maxNum>
struct CollisionCell
{
    static constexpr uint8_t cell_capacity = maxNum;
    static constexpr uint8_t max_cell_idx = cell_capacity - 1;

    uint32_t objects_count = 0;
    uint32_t objects[cell_capacity] = {};
    std::mutex mutex;

    CollisionCell() = default;
    CollisionCell(const CollisionCell<maxNum>& a) {

    }

    inline uint32_t& operator[](int i) {
        return objects[i];
    }

    void push_back(uint32_t id)
    {
        std::lock_guard<std::mutex> lock(mutex);
        objects[objects_count] = id;
        objects_count += objects_count < max_cell_idx;
    }

    void clear()
    {
        objects_count = 0u;
    }

    void remove(uint32_t id)
    {
        for (uint32_t i{ 0 }; i < objects_count; ++i) {
            if (objects[i] == id) {
                // Swap pop
                objects[i] = objects[objects_count - 1];
                --objects_count;
                return;
            }
        }
    }
};
