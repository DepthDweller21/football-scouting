#pragma once

#include <cstdint>
#include <functional>
#include <vector>

/**
 * Project-owned AVL tree keyed by sortKey (must be unique per insertion).
 * Satellite stores player id for ranking queries.
 */
class AVLTree {
    struct Node {
        int64_t key{};
        uint64_t player_id{};
        Node* left{nullptr};
        Node* right{nullptr};
        int height{1};
    };

    Node* root_{nullptr};
    std::size_t size_{0};

    static int height(Node* n);
    static int balance_factor(Node* n);
    static void update_height(Node* n);
    static Node* rotate_right(Node* y);
    static Node* rotate_left(Node* x);
    static Node* balance(Node* n);
    static Node* insert_node(Node* n, int64_t key, uint64_t player_id, bool& inserted);
    static Node* min_node(Node* n);
    static Node* erase_node(Node* n, int64_t key, uint64_t player_id, bool& removed);

    void clear_recursive(Node* n);
    static void collect_recursive(Node* n, std::vector<uint64_t>& out);
    static void walk_recursive(Node* n,
                               const std::function<void(int64_t key, uint64_t player_id)>& visitor);

public:
    AVLTree();
    ~AVLTree();

    AVLTree(const AVLTree&) = delete;
    AVLTree& operator=(const AVLTree&) = delete;

    [[nodiscard]] std::size_t size() const { return size_; }

    void clear();
    void insert(int64_t key, uint64_t player_id);
    bool erase(int64_t key, uint64_t player_id);

    /** In-order traversal: ascending sortKey. */
    void collect_in_order(std::vector<uint64_t>& player_ids_out) const;

    void walk_in_order(const std::function<void(int64_t key, uint64_t player_id)>& visitor) const;
};
