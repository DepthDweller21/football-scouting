#include "structures/MaxHeap.h"

void MaxHeap::clear() {
    heap_.clear();
}

bool MaxHeap::empty() const {
    return heap_.empty();
}

std::size_t MaxHeap::size() const {
    return heap_.size();
}

bool MaxHeap::higher_priority(const Entry& a, const Entry& b) {
    if (a.priority != b.priority) {
        return a.priority > b.priority;
    }
    return a.id > b.id;
}

void MaxHeap::sift_up(std::size_t i) {
    while (i > 0) {
        const std::size_t parent = (i - 1) / 2;
        if (!higher_priority(heap_[i], heap_[parent])) {
            break;
        }
        std::swap(heap_[i], heap_[parent]);
        i = parent;
    }
}

void MaxHeap::sift_down(std::size_t i) {
    const std::size_t n = heap_.size();
    while (true) {
        const std::size_t left = 2 * i + 1;
        const std::size_t right = 2 * i + 2;
        std::size_t largest = i;
        if (left < n && higher_priority(heap_[left], heap_[largest])) {
            largest = left;
        }
        if (right < n && higher_priority(heap_[right], heap_[largest])) {
            largest = right;
        }
        if (largest == i) {
            break;
        }
        std::swap(heap_[i], heap_[largest]);
        i = largest;
    }
}

void MaxHeap::push(std::int64_t priority, std::uint64_t id) {
    heap_.push_back(Entry{priority, id});
    sift_up(heap_.size() - 1);
}

bool MaxHeap::pop_max(Entry& out) {
    if (heap_.empty()) {
        return false;
    }
    out = heap_.front();
    heap_[0] = heap_.back();
    heap_.pop_back();
    if (!heap_.empty()) {
        sift_down(0);
    }
    return true;
}
