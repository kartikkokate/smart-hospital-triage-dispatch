#pragma once

#include <cstddef>
#include <utility>
#include <vector>
#include <functional>
#include "ds/CustomStack.hpp"
#include "ds/CustomQueue.hpp"

/**
 * @brief Independent Mirror Binary Tree representation.
 * 
 * DESIGN RATIONALE:
 * - A mirror tree inverts the binary search property (all right descendants < root < left descendants).
 * - To prevent subtle operational bugs, MirrorBinaryTree is an independent, type-safe class
 *   that CANNOT be used as an operational BST (it intentionally does NOT provide BST search or
 *   ordered insertion methods).
 * - Supports traversal, inspection, and verification of the mirrored structure.
 */
template <typename K, typename V>
class MirrorBinaryTree {
public:
    struct MirrorNode {
        K key;
        V value;
        MirrorNode* left = nullptr;
        MirrorNode* right = nullptr;

        MirrorNode(const K& k, const V& v) : key(k), value(v), left(nullptr), right(nullptr) {}
        MirrorNode(K&& k, V&& v) : key(std::move(k)), value(std::move(v)), left(nullptr), right(nullptr) {}
    };

private:
    MirrorNode* root_ = nullptr;
    size_t size_ = 0;

    static MirrorNode* cloneSubtree(MirrorNode* node) {
        if (node == nullptr) return nullptr;
        MirrorNode* newNode = new MirrorNode(node->key, node->value);
        try {
            newNode->left = cloneSubtree(node->left);
            newNode->right = cloneSubtree(node->right);
        } catch (...) {
            deleteSubtree(newNode);
            throw;
        }
        return newNode;
    }

    static void deleteSubtree(MirrorNode* node) noexcept {
        if (node == nullptr) return;
        deleteSubtree(node->left);
        deleteSubtree(node->right);
        delete node;
    }

    static void inorderInternal(MirrorNode* node, std::vector<K>& outKeys) {
        if (node == nullptr) return;
        inorderInternal(node->left, outKeys);
        outKeys.push_back(node->key);
        inorderInternal(node->right, outKeys);
    }

    static size_t heightInternal(MirrorNode* node) {
        if (node == nullptr) return 0;
        size_t lh = heightInternal(node->left);
        size_t rh = heightInternal(node->right);
        return 1 + (lh > rh ? lh : rh);
    }

public:
    MirrorBinaryTree() = default;

    explicit MirrorBinaryTree(MirrorNode* root, size_t sz) : root_(root), size_(sz) {}

    ~MirrorBinaryTree() {
        clear();
    }

    MirrorBinaryTree(const MirrorBinaryTree& other) : size_(other.size_) {
        root_ = cloneSubtree(other.root_);
    }

    MirrorBinaryTree& operator=(const MirrorBinaryTree& other) {
        if (this != &other) {
            MirrorBinaryTree temp(other);
            swap(temp);
        }
        return *this;
    }

    MirrorBinaryTree(MirrorBinaryTree&& other) noexcept 
        : root_(other.root_), size_(other.size_) {
        other.root_ = nullptr;
        other.size_ = 0;
    }

    MirrorBinaryTree& operator=(MirrorBinaryTree&& other) noexcept {
        if (this != &other) {
            clear();
            root_ = other.root_;
            size_ = other.size_;
            other.root_ = nullptr;
            other.size_ = 0;
        }
        return *this;
    }

    void swap(MirrorBinaryTree& other) noexcept {
        MirrorNode* tempRoot = root_; root_ = other.root_; other.root_ = tempRoot;
        size_t tempSize = size_; size_ = other.size_; other.size_ = tempSize;
    }

    size_t size() const noexcept { return size_; }
    bool empty() const noexcept { return size_ == 0; }

    void clear() noexcept {
        deleteSubtree(root_);
        root_ = nullptr;
        size_ = 0;
    }

    size_t height() const {
        return heightInternal(root_);
    }

    // Inorder traversal of a mirror tree yields keys in DESCENDING order
    std::vector<K> inorderKeys() const {
        std::vector<K> result;
        result.reserve(size_);
        inorderInternal(root_, result);
        return result;
    }
};

/**
 * @brief Custom Binary Search Tree (BST) parameterized by Key and Value.
 * 
 * OWNERSHIP & MEMORY POLICY:
 * - BinarySearchTree owns all allocated BSTNode structures.
 * - Non-Owning Pointer Safety: When storing pointer values (e.g. BinarySearchTree<std::string, Patient*>),
 *   deleting a BST node or clearing the tree deallocates the internal BSTNode only; it NEVER
 *   deallocates the pointed-to domain entity (such as registry-owned Patient records).
 * - Exception-Safety: Copy construction uses exception-safe subtree cloning (strong guarantee).
 * - Move semantics transfer root pointer in O(1) time without allocations.
 * 
 * DUPLICATE KEY POLICY:
 * - When insert(key, value) encounters an already existing key, it updates the associated value
 *   in-place and returns false (0 new nodes added). When inserting a unique key, it creates
 *   a new node, increments size_, and returns true.
 * 
 * DELETION POLICY:
 * - Fully implements all three standard cases:
 *   1. Leaf node (0 children): unlinks from parent and deallocates.
 *   2. Single child: bypasses node, linking parent directly to child.
 *   3. Two children: finds in-order successor (minimum in right subtree), copies key/value,
 *      and deletes successor from the right subtree.
 * - Root node deletion is cleanly handled across all cases.
 * 
 * COMPLEXITY:
 * - insert / search / remove: O(h) where h is the tree height.
 *   - Average (random input): Theta(log n) time.
 *   - Worst-case (sorted input): O(n) degeneration into an unbalanced linear chain.
 * - traversals (inorder, preorder, postorder, level-order): Theta(n) time.
 * - height / leaf count: Theta(n) time.
 */
template <typename K, typename V>
class BinarySearchTree {
public:
    struct BSTNode {
        K key;
        V value;
        BSTNode* left = nullptr;
        BSTNode* right = nullptr;

        BSTNode(const K& k, const V& v) : key(k), value(v), left(nullptr), right(nullptr) {}
        BSTNode(K&& k, V&& v) : key(std::move(k)), value(std::move(v)), left(nullptr), right(nullptr) {}
    };

private:
    BSTNode* root_ = nullptr;
    size_t size_ = 0;

    static BSTNode* cloneSubtree(BSTNode* node) {
        if (node == nullptr) return nullptr;
        BSTNode* newNode = new BSTNode(node->key, node->value);
        try {
            newNode->left = cloneSubtree(node->left);
            newNode->right = cloneSubtree(node->right);
        } catch (...) {
            deleteSubtree(newNode);
            throw;
        }
        return newNode;
    }

    static typename MirrorBinaryTree<K, V>::MirrorNode* mirrorClone(BSTNode* node) {
        if (node == nullptr) return nullptr;
        using MNode = typename MirrorBinaryTree<K, V>::MirrorNode;
        MNode* newNode = new MNode(node->key, node->value);
        try {
            newNode->left = mirrorClone(node->right); // Inverted: left becomes right
            newNode->right = mirrorClone(node->left); // Inverted: right becomes left
        } catch (...) {
            deleteMirrorSubtree(newNode);
            throw;
        }
        return newNode;
    }

    static void deleteMirrorSubtree(typename MirrorBinaryTree<K, V>::MirrorNode* node) noexcept {
        if (node == nullptr) return;
        deleteMirrorSubtree(node->left);
        deleteMirrorSubtree(node->right);
        delete node;
    }

    static void deleteSubtree(BSTNode* node) noexcept {
        if (node == nullptr) return;
        deleteSubtree(node->left);
        deleteSubtree(node->right);
        delete node;
    }

    static size_t heightInternal(BSTNode* node) {
        if (node == nullptr) return 0;
        size_t lh = heightInternal(node->left);
        size_t rh = heightInternal(node->right);
        return 1 + (lh > rh ? lh : rh);
    }

    static size_t countLeavesInternal(BSTNode* node) {
        if (node == nullptr) return 0;
        if (node->left == nullptr && node->right == nullptr) return 1;
        return countLeavesInternal(node->left) + countLeavesInternal(node->right);
    }

    static void getLeafKeysInternal(BSTNode* node, std::vector<K>& outLeaves) {
        if (node == nullptr) return;
        if (node->left == nullptr && node->right == nullptr) {
            outLeaves.push_back(node->key);
            return;
        }
        getLeafKeysInternal(node->left, outLeaves);
        getLeafKeysInternal(node->right, outLeaves);
    }

    static void inorderInternal(BSTNode* node, std::vector<K>& outKeys) {
        if (node == nullptr) return;
        inorderInternal(node->left, outKeys);
        outKeys.push_back(node->key);
        inorderInternal(node->right, outKeys);
    }

    static void preorderInternal(BSTNode* node, std::vector<K>& outKeys) {
        if (node == nullptr) return;
        outKeys.push_back(node->key);
        preorderInternal(node->left, outKeys);
        preorderInternal(node->right, outKeys);
    }

    static void postorderInternal(BSTNode* node, std::vector<K>& outKeys) {
        if (node == nullptr) return;
        postorderInternal(node->left, outKeys);
        postorderInternal(node->right, outKeys);
        outKeys.push_back(node->key);
    }

    static BSTNode* findMin(BSTNode* node) {
        while (node != nullptr && node->left != nullptr) {
            node = node->left;
        }
        return node;
    }

    // Three-case recursive deletion helper
    static BSTNode* removeInternal(BSTNode* node, const K& key, bool& outRemoved) {
        if (node == nullptr) {
            outRemoved = false;
            return nullptr;
        }

        if (key < node->key) {
            node->left = removeInternal(node->left, key, outRemoved);
        } else if (node->key < key) {
            node->right = removeInternal(node->right, key, outRemoved);
        } else {
            // Target node found!
            outRemoved = true;

            // Case 1: 0 children (leaf)
            if (node->left == nullptr && node->right == nullptr) {
                delete node;
                return nullptr;
            }
            // Case 2: 1 child (right child only)
            if (node->left == nullptr) {
                BSTNode* temp = node->right;
                delete node;
                return temp;
            }
            // Case 2: 1 child (left child only)
            if (node->right == nullptr) {
                BSTNode* temp = node->left;
                delete node;
                return temp;
            }
            // Case 3: 2 children
            // Find in-order successor (minimum key in right subtree)
            BSTNode* successor = findMin(node->right);
            node->key = successor->key;
            node->value = successor->value;
            // Recursively delete the successor node from the right subtree
            bool succRemoved = false;
            node->right = removeInternal(node->right, successor->key, succRemoved);
        }
        return node;
    }

public:
    BinarySearchTree() = default;

    ~BinarySearchTree() {
        clear();
    }

    // Explicit Exception-Safe Deep Copy Constructor
    BinarySearchTree(const BinarySearchTree& other) : size_(other.size_) {
        root_ = cloneSubtree(other.root_);
    }

    // Deep Copy Assignment via copy-and-swap
    BinarySearchTree& operator=(const BinarySearchTree& other) {
        if (this != &other) {
            BinarySearchTree temp(other);
            swap(temp);
        }
        return *this;
    }

    // Move Constructor
    BinarySearchTree(BinarySearchTree&& other) noexcept 
        : root_(other.root_), size_(other.size_) {
        other.root_ = nullptr;
        other.size_ = 0;
    }

    // Move Assignment
    BinarySearchTree& operator=(BinarySearchTree&& other) noexcept {
        if (this != &other) {
            clear();
            root_ = other.root_;
            size_ = other.size_;
            other.root_ = nullptr;
            other.size_ = 0;
        }
        return *this;
    }

    void swap(BinarySearchTree& other) noexcept {
        BSTNode* tempRoot = root_; root_ = other.root_; other.root_ = tempRoot;
        size_t tempSize = size_; size_ = other.size_; other.size_ = tempSize;
    }

    size_t size() const noexcept { return size_; }
    bool empty() const noexcept { return size_ == 0; }

    void clear() noexcept {
        deleteSubtree(root_);
        root_ = nullptr;
        size_ = 0;
    }

    // Insert or update. Returns true if new key inserted, false if existing key updated.
    bool insert(const K& key, const V& value) {
        if (root_ == nullptr) {
            root_ = new BSTNode(key, value);
            size_++;
            return true;
        }

        BSTNode* curr = root_;
        while (curr != nullptr) {
            if (key == curr->key) {
                curr->value = value;
                return false; // Duplicate key: value updated in-place
            }
            if (key < curr->key) {
                if (curr->left == nullptr) {
                    curr->left = new BSTNode(key, value);
                    size_++;
                    return true;
                }
                curr = curr->left;
            } else {
                if (curr->right == nullptr) {
                    curr->right = new BSTNode(key, value);
                    size_++;
                    return true;
                }
                curr = curr->right;
            }
        }
        return false;
    }

    // Lookup returning direct pointer to value (nullptr if missing)
    V* search(const K& key) {
        BSTNode* curr = root_;
        while (curr != nullptr) {
            if (key == curr->key) {
                return &(curr->value);
            }
            curr = (key < curr->key) ? curr->left : curr->right;
        }
        return nullptr;
    }

    const V* search(const K& key) const {
        BSTNode* curr = root_;
        while (curr != nullptr) {
            if (key == curr->key) {
                return &(curr->value);
            }
            curr = (key < curr->key) ? curr->left : curr->right;
        }
        return nullptr;
    }

    bool contains(const K& key) const {
        return (search(key) != nullptr);
    }

    // Three-case node deletion
    bool remove(const K& key) {
        if (root_ == nullptr) return false;
        bool removed = false;
        root_ = removeInternal(root_, key, removed);
        if (removed) {
            size_--;
        }
        return removed;
    }

    // Tree metrics
    size_t height() const {
        return heightInternal(root_);
    }

    size_t countLeaves() const {
        return countLeavesInternal(root_);
    }

    std::vector<K> getLeafKeys() const {
        std::vector<K> leaves;
        getLeafKeysInternal(root_, leaves);
        return leaves;
    }

    // Traversals
    std::vector<K> inorderRecursive() const {
        std::vector<K> keys;
        keys.reserve(size_);
        inorderInternal(root_, keys);
        return keys;
    }

    std::vector<K> preorderRecursive() const {
        std::vector<K> keys;
        keys.reserve(size_);
        preorderInternal(root_, keys);
        return keys;
    }

    std::vector<K> postorderRecursive() const {
        std::vector<K> keys;
        keys.reserve(size_);
        postorderInternal(root_, keys);
        return keys;
    }

    // Iterative in-order traversal using CustomStack (verifies stack-based traversal mechanics)
    std::vector<K> inorderIterative() const {
        std::vector<K> keys;
        keys.reserve(size_);
        CustomStack<BSTNode*> stack;
        BSTNode* curr = root_;

        while (curr != nullptr || !stack.isEmpty()) {
            while (curr != nullptr) {
                stack.push(curr);
                curr = curr->left;
            }
            BSTNode* topNode = nullptr;
            stack.pop(topNode);
            keys.push_back(topNode->key);
            curr = topNode->right;
        }
        return keys;
    }

    // Level-order (breadth-first) traversal using CustomQueue
    std::vector<K> levelOrder() const {
        std::vector<K> keys;
        if (root_ == nullptr) return keys;
        keys.reserve(size_);

        CustomQueue<BSTNode*> queue;
        queue.enqueue(root_);

        while (!queue.isEmpty()) {
            BSTNode* curr = nullptr;
            queue.dequeue(curr);
            keys.push_back(curr->key);
            if (curr->left != nullptr) {
                queue.enqueue(curr->left);
            }
            if (curr->right != nullptr) {
                queue.enqueue(curr->right);
            }
        }
        return keys;
    }

    // Create an independent mirror copy that cannot be used as an operational BST
    MirrorBinaryTree<K, V> createMirrorTree() const {
        typename MirrorBinaryTree<K, V>::MirrorNode* mirrorRoot = mirrorClone(root_);
        return MirrorBinaryTree<K, V>(mirrorRoot, size_);
    }
};
