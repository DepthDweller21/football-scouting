#include "structures/AVLTree.h"

#include <algorithm>

AVLTree::AVLTree() = default;

AVLTree::~AVLTree() {
    clear();
}

void AVLTree::clear_recursive(Node* n) {
    if (!n) {
        return;
    }
    clear_recursive(n->left);
    clear_recursive(n->right);
    delete n;
}

void AVLTree::clear() {
    clear_recursive(root_);
    root_ = nullptr;
    size_ = 0;
}

int AVLTree::height(Node* n) {
    return n ? n->height : 0;
}

int AVLTree::balance_factor(Node* n) {
    return n ? height(n->left) - height(n->right) : 0;
}

void AVLTree::update_height(Node* n) {
    if (n) {
        n->height = 1 + std::max(height(n->left), height(n->right));
    }
}

AVLTree::Node* AVLTree::rotate_right(Node* y) {
    Node* x = y->left;
    Node* t2 = x->right;
    x->right = y;
    y->left = t2;
    update_height(y);
    update_height(x);
    return x;
}

AVLTree::Node* AVLTree::rotate_left(Node* x) {
    Node* y = x->right;
    Node* t2 = y->left;
    y->left = x;
    x->right = t2;
    update_height(x);
    update_height(y);
    return y;
}

AVLTree::Node* AVLTree::balance(Node* n) {
    update_height(n);
    const int bf = balance_factor(n);
    if (bf > 1 && balance_factor(n->left) >= 0) {
        return rotate_right(n);
    }
    if (bf > 1 && balance_factor(n->left) < 0) {
        n->left = rotate_left(n->left);
        return rotate_right(n);
    }
    if (bf < -1 && balance_factor(n->right) <= 0) {
        return rotate_left(n);
    }
    if (bf < -1 && balance_factor(n->right) > 0) {
        n->right = rotate_right(n->right);
        return rotate_left(n);
    }
    return n;
}

AVLTree::Node* AVLTree::insert_node(Node* n, int64_t key, uint64_t player_id, bool& inserted) {
    if (!n) {
        inserted = true;
        Node* leaf = new Node{key, player_id, nullptr, nullptr, 1};
        return leaf;
    }
    if (key < n->key || (key == n->key && player_id < n->player_id)) {
        n->left = insert_node(n->left, key, player_id, inserted);
    } else if (key > n->key || (key == n->key && player_id > n->player_id)) {
        n->right = insert_node(n->right, key, player_id, inserted);
    } else {
        inserted = false;
        return n;
    }
    return balance(n);
}

void AVLTree::insert(int64_t key, uint64_t player_id) {
    bool inserted = false;
    root_ = insert_node(root_, key, player_id, inserted);
    if (inserted) {
        ++size_;
    }
}

AVLTree::Node* AVLTree::min_node(Node* n) {
    while (n && n->left) {
        n = n->left;
    }
    return n;
}

AVLTree::Node* AVLTree::erase_node(Node* n, int64_t key, uint64_t player_id, bool& removed) {
    if (!n) {
        return nullptr;
    }
    if (key < n->key || (key == n->key && player_id < n->player_id)) {
        n->left = erase_node(n->left, key, player_id, removed);
    } else if (key > n->key || (key == n->key && player_id > n->player_id)) {
        n->right = erase_node(n->right, key, player_id, removed);
    } else {
        removed = true;
        if (!n->left || !n->right) {
            Node* tmp = n->left ? n->left : n->right;
            delete n;
            return tmp;
        }
        Node* succ = min_node(n->right);
        n->key = succ->key;
        n->player_id = succ->player_id;
        bool dummy = false;
        n->right = erase_node(n->right, succ->key, succ->player_id, dummy);
    }
    return balance(n);
}

bool AVLTree::erase(int64_t key, uint64_t player_id) {
    bool removed = false;
    root_ = erase_node(root_, key, player_id, removed);
    if (removed) {
        --size_;
    }
    return removed;
}

void AVLTree::collect_recursive(Node* n, std::vector<uint64_t>& out) {
    if (!n) {
        return;
    }
    collect_recursive(n->left, out);
    out.push_back(n->player_id);
    collect_recursive(n->right, out);
}

void AVLTree::collect_in_order(std::vector<uint64_t>& player_ids_out) const {
    collect_recursive(root_, player_ids_out);
}

void AVLTree::walk_recursive(Node* n,
                              const std::function<void(int64_t key, uint64_t player_id)>& visitor) {
    if (!n) {
        return;
    }
    walk_recursive(n->left, visitor);
    visitor(n->key, n->player_id);
    walk_recursive(n->right, visitor);
}

void AVLTree::walk_in_order(const std::function<void(int64_t key, uint64_t player_id)>& visitor) const {
    walk_recursive(root_, visitor);
}
