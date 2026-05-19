#pragma once

#include <cstddef>
#include <functional>
#include <utility>

/**
 * Project-owned separate-chaining hash table (not std::unordered_map).
 * Buckets hold intrusive singly-linked collision chains.
 */
template <typename K, typename V>
class HashTable {
    struct Node {
        K key{};
        V value{};
        Node* next{nullptr};
    };

    Node** buckets_{nullptr};
    std::size_t bucket_count_;
    std::size_t size_;

    [[nodiscard]] std::size_t bucket_index(const K& key) const {
        return std::hash<K>{}(key) % bucket_count_;
    }

public:
    explicit HashTable(std::size_t bucket_count = 1024) : bucket_count_(bucket_count == 0 ? 1 : bucket_count), size_(0) {
        buckets_ = new Node*[bucket_count_]{};
    }

    ~HashTable() { clear(); delete[] buckets_; }

    HashTable(const HashTable&) = delete;
    HashTable& operator=(const HashTable&) = delete;

    HashTable(HashTable&& other) noexcept
        : buckets_(other.buckets_), bucket_count_(other.bucket_count_), size_(other.size_) {
        other.buckets_ = nullptr;
        other.bucket_count_ = 0;
        other.size_ = 0;
    }

    HashTable& operator=(HashTable&& other) noexcept {
        if (this == &other) {
            return *this;
        }
        clear();
        delete[] buckets_;
        buckets_ = other.buckets_;
        bucket_count_ = other.bucket_count_;
        size_ = other.size_;
        other.buckets_ = nullptr;
        other.bucket_count_ = 0;
        other.size_ = 0;
        return *this;
    }

    void clear() {
        if (!buckets_) {
            return;
        }
        for (std::size_t i = 0; i < bucket_count_; ++i) {
            Node* cur = buckets_[i];
            while (cur) {
                Node* nx = cur->next;
                delete cur;
                cur = nx;
            }
            buckets_[i] = nullptr;
        }
        size_ = 0;
    }

    void insert_or_assign(const K& key, const V& value) {
        const std::size_t ix = bucket_index(key);
        Node* cur = buckets_[ix];
        while (cur) {
            if (cur->key == key) {
                cur->value = value;
                return;
            }
            cur = cur->next;
        }
        Node* n = new Node{key, value, buckets_[ix]};
        buckets_[ix] = n;
        ++size_;
    }

    [[nodiscard]] bool try_get(const K& key, V& out) const {
        const std::size_t ix = bucket_index(key);
        for (Node* cur = buckets_[ix]; cur; cur = cur->next) {
            if (cur->key == key) {
                out = cur->value;
                return true;
            }
        }
        return false;
    }

    [[nodiscard]] bool contains(const K& key) const {
        V dummy{};
        return try_get(key, dummy);
    }

    /** Non-const pointer to stored value; nullptr if missing. */
    V* find_mut(const K& key) {
        const std::size_t ix = bucket_index(key);
        for (Node* cur = buckets_[ix]; cur; cur = cur->next) {
            if (cur->key == key) {
                return &cur->value;
            }
        }
        return nullptr;
    }

    bool erase(const K& key) {
        const std::size_t ix = bucket_index(key);
        Node** pp = &buckets_[ix];
        while (*pp) {
            if ((*pp)->key == key) {
                Node* dead = *pp;
                *pp = dead->next;
                delete dead;
                --size_;
                return true;
            }
            pp = &(*pp)->next;
        }
        return false;
    }

    [[nodiscard]] std::size_t size() const { return size_; }

    template <typename Fn>
    void for_each(Fn&& fn) const {
        if (!buckets_) {
            return;
        }
        for (std::size_t i = 0; i < bucket_count_; ++i) {
            for (Node* cur = buckets_[i]; cur; cur = cur->next) {
                fn(cur->key, cur->value);
            }
        }
    }
};
