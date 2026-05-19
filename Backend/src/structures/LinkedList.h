#pragma once

#include <cstddef>
#include <utility>

/**
 * Project-owned singly linked list (template).
 * Used for report history per player and roster traversals where ordered chains matter.
 */
template <typename T>
class LinkedList {
    struct Node {
        T data{};
        Node* next{nullptr};
    };

    Node* head_{nullptr};
    Node* tail_{nullptr};
    std::size_t size_{0};

public:
    LinkedList() = default;
    ~LinkedList() { clear(); }

    LinkedList(const LinkedList&) = delete;
    LinkedList& operator=(const LinkedList&) = delete;

    LinkedList(LinkedList&& other) noexcept
        : head_(other.head_), tail_(other.tail_), size_(other.size_) {
        other.head_ = other.tail_ = nullptr;
        other.size_ = 0;
    }

    LinkedList& operator=(LinkedList&& other) noexcept {
        if (this == &other) {
            return *this;
        }
        clear();
        head_ = other.head_;
        tail_ = other.tail_;
        size_ = other.size_;
        other.head_ = other.tail_ = nullptr;
        other.size_ = 0;
        return *this;
    }

    void clear() {
        while (head_) {
            Node* n = head_;
            head_ = head_->next;
            delete n;
        }
        tail_ = nullptr;
        size_ = 0;
    }

    void push_back(const T& value) {
        Node* n = new Node{value, nullptr};
        if (!tail_) {
            head_ = tail_ = n;
        } else {
            tail_->next = n;
            tail_ = n;
        }
        ++size_;
    }

    void push_front(const T& value) {
        Node* n = new Node{value, head_};
        head_ = n;
        if (!tail_) {
            tail_ = n;
        }
        ++size_;
    }

    [[nodiscard]] std::size_t size() const { return size_; }
    [[nodiscard]] bool empty() const { return head_ == nullptr; }

    template <typename Fn>
    void for_each(Fn&& fn) const {
        for (Node* cur = head_; cur; cur = cur->next) {
            fn(cur->data);
        }
    }

    template <typename Fn>
    void for_each(Fn&& fn) {
        for (Node* cur = head_; cur; cur = cur->next) {
            fn(cur->data);
        }
    }
};
