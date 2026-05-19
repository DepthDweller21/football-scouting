#pragma once

#include <cstdint>
#include <cstddef>
#include <vector>

/**
 * Binary max-heap (array-backed). Larger priority wins.
 * Used for pass target selection in the formation simulator.
 */
class MaxHeap {
public:
    struct Entry {
        std::int64_t priority{0};
        std::uint64_t id{0};
    };

    void clear();
    bool empty() const;
    std::size_t size() const;

    void push(std::int64_t priority, std::uint64_t id);
  /** Removes and returns the maximum entry. */
    bool pop_max(Entry& out);

private:
    std::vector<Entry> heap_;

    static bool higher_priority(const Entry& a, const Entry& b);
    void sift_up(std::size_t i);
    void sift_down(std::size_t i);
};
