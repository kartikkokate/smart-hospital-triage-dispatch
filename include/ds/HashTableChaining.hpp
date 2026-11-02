#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>
#include <utility>
#include <functional>

/**
 * @brief Custom Hash Table using Separate Chaining for collision resolution.
 * 
 * DESIGN & SYLLABUS ALIGNMENT:
 * - Bucket array storing singly linked list heads of (key, value) pairs.
 * - String keys use polynomial rolling hash:
 *     H(s) = (sum_{i=0}^{L-1} s[i] * 31^i) mod M
 * - Initial prime capacity M = 17 (configurable).
 * - Automatic dynamic resizing when load factor alpha = n / M exceeds 0.75.
 * - Rehashing targets the next prime >= 2M + 1, guaranteeing load factor stays <= 0.75.
 * 
 * OWNERSHIP & MEMORY POLICY:
 * - HashTableChaining owns all dynamically allocated bucket nodes.
 * - Exception safety: Copy constructor cleans up partially allocated nodes if copying throws.
 * - Moved-from state is safely reusable: insertion into a moved-from table (capacity == 0)
 *   automatically restores valid bucket storage before inserting.
 * - Destructor cleans up all bucket lists iteratively to prevent call stack overflow.
 * 
 * LIFETIME & REFERENCE POLICY:
 * - search(key) returns a direct pointer V* to the value stored in the node.
 * - Node Pointer Validity: In this implementation, rehash() relinks existing Node pointers
 *   into the new bucket array without reallocating node memory. Therefore, pointers V*
 *   returned by search() remain valid across rehash() operations, but are invalidated
 *   if the specific key is removed via remove() or if clear() is invoked.
 * 
 * COMPLEXITY:
 * - insert:      Theta(1) amortized average time; an insertion triggering rehash() takes
 *                Theta(n + M) time. O(n) worst-case under pathological collisions.
 * - search:      Theta(1) average time, O(n) worst-case.
 * - remove:      Theta(1) average time, O(n) worst-case.
 * - rehash:      Theta(n + M) time when triggered.
 * - size/empty:  O(1) time.
 * - loadFactor:  O(1) time.
 */

namespace ds_internal {
    inline bool isPrime(size_t n) {
        if (n <= 1) return false;
        if (n <= 3) return true;
        if (n % 2 == 0 || n % 3 == 0) return false;
        for (size_t i = 5; i * i <= n; i += 6) {
            if (n % i == 0 || n % (i + 2) == 0) return false;
        }
        return true;
    }

    inline size_t nextPrime(size_t minVal) {
        if (minVal <= 2) return 2;
        size_t p = (minVal % 2 == 0) ? minVal + 1 : minVal;
        while (!isPrime(p)) {
            p += 2;
        }
        return p;
    }

    template <typename K>
    struct HashDispatcher {
        size_t operator()(const K& key, size_t capacity) const {
            return (capacity == 0) ? 0 : (std::hash<K>{}(key) % capacity);
        }
    };

    template <>
    struct HashDispatcher<std::string> {
        size_t operator()(const std::string& key, size_t capacity) const {
            if (capacity == 0) return 0;
            uint64_t hashVal = 0;
            uint64_t p_pow = 1;
            const uint64_t p = 31;
            for (char c : key) {
                hashVal = (hashVal + static_cast<unsigned char>(c) * p_pow) % capacity;
                p_pow = (p_pow * p) % capacity;
            }
            return static_cast<size_t>(hashVal % capacity);
        }
    };
}

template <typename K, typename V>
class HashTableChaining {
public:
    struct Node {
        K key;
        V value;
        Node* next;

        Node(const K& k, const V& v, Node* n = nullptr) 
            : key(k), value(v), next(n) {}
        Node(K&& k, V&& v, Node* n = nullptr) 
            : key(std::move(k)), value(std::move(v)), next(n) {}
    };

private:
    std::vector<Node*> buckets_;
    size_t size_ = 0;
    size_t capacity_ = 17;
    double maxLoadFactor_ = 0.75;
    bool simulateRehashFailure_ = false;

    size_t computeHash(const K& key, size_t cap) const {
        return (cap == 0) ? 0 : ds_internal::HashDispatcher<K>{}(key, cap);
    }

public:
    explicit HashTableChaining(size_t initialCapacity = 17, double maxLoadFactor = 0.75)
        : size_(0), maxLoadFactor_(maxLoadFactor) {
        capacity_ = ds_internal::nextPrime(initialCapacity);
        buckets_.assign(capacity_, nullptr);
    }

    ~HashTableChaining() {
        clear();
    }

    // Explicit Exception-Safe Deep Copy Constructor
    HashTableChaining(const HashTableChaining& other)
        : size_(0), capacity_(other.capacity_), maxLoadFactor_(other.maxLoadFactor_) {
        buckets_.assign(capacity_, nullptr);
        try {
            for (size_t i = 0; i < other.capacity_; ++i) {
                Node* curr = other.buckets_[i];
                while (curr != nullptr) {
                    insert(curr->key, curr->value);
                    curr = curr->next;
                }
            }
        } catch (...) {
            clear();
            throw;
        }
    }

    // Deep Copy Assignment via copy-and-swap
    HashTableChaining& operator=(const HashTableChaining& other) {
        if (this != &other) {
            HashTableChaining temp(other);
            swap(temp);
        }
        return *this;
    }

    // Move Constructor
    HashTableChaining(HashTableChaining&& other) noexcept
        : buckets_(std::move(other.buckets_)),
          size_(other.size_),
          capacity_(other.capacity_),
          maxLoadFactor_(other.maxLoadFactor_) {
        other.size_ = 0;
        other.capacity_ = 0;
    }

    // Move Assignment
    HashTableChaining& operator=(HashTableChaining&& other) noexcept {
        if (this != &other) {
            clear();
            buckets_ = std::move(other.buckets_);
            size_ = other.size_;
            capacity_ = other.capacity_;
            maxLoadFactor_ = other.maxLoadFactor_;
            other.size_ = 0;
            other.capacity_ = 0;
        }
        return *this;
    }

    void swap(HashTableChaining& other) noexcept {
        buckets_.swap(other.buckets_);
        std::swap(size_, other.size_);
        std::swap(capacity_, other.capacity_);
        std::swap(maxLoadFactor_, other.maxLoadFactor_);
    }

    size_t size() const noexcept {
        return size_;
    }

    size_t capacity() const noexcept {
        return capacity_;
    }

    bool empty() const noexcept {
        return size_ == 0;
    }

    double loadFactor() const noexcept {
        if (capacity_ == 0) return 0.0;
        return static_cast<double>(size_) / static_cast<double>(capacity_);
    }

    // Iterative safe cleanup of all bucket lists
    void clear() noexcept {
        for (size_t i = 0; i < buckets_.size(); ++i) {
            Node* curr = buckets_[i];
            while (curr != nullptr) {
                Node* nextNode = curr->next;
                delete curr;
                curr = nextNode;
            }
            buckets_[i] = nullptr;
        }
        size_ = 0;
    }

    size_t hash(const K& key) const {
        return computeHash(key, capacity_);
    }

    // Insert or update. Restores storage if moved-from (capacity == 0).
    bool insert(const K& key, const V& value) {
        if (capacity_ == 0) {
            capacity_ = ds_internal::nextPrime(17);
            buckets_.assign(capacity_, nullptr);
            size_ = 0;
        }

        size_t bucketIdx = computeHash(key, capacity_);
        Node* curr = buckets_[bucketIdx];

        // Check if key already exists (update in-place)
        while (curr != nullptr) {
            if (curr->key == key) {
                curr->value = value;
                return false; // Updated existing key
            }
            curr = curr->next;
        }

        // Insert new node at bucket head
        Node* newNode = new Node(key, value, buckets_[bucketIdx]);
        buckets_[bucketIdx] = newNode;
        size_++;

        // Check load factor and rehash if threshold exceeded
        if (loadFactor() > maxLoadFactor_) {
            try {
                rehash();
            } catch (...) {
                // Strong exception safety: rollback newly inserted node if rehash throws
                buckets_[bucketIdx] = newNode->next;
                delete newNode;
                size_--;
                throw;
            }
        }

        return true; // Newly inserted
    }

    // Search returning non-owning pointer to value (nullptr if missing or capacity == 0)
    V* search(const K& key) {
        if (capacity_ == 0) return nullptr;
        size_t bucketIdx = computeHash(key, capacity_);
        Node* curr = buckets_[bucketIdx];
        while (curr != nullptr) {
            if (curr->key == key) {
                return &(curr->value);
            }
            curr = curr->next;
        }
        return nullptr;
    }

    const V* search(const K& key) const {
        if (capacity_ == 0) return nullptr;
        size_t bucketIdx = computeHash(key, capacity_);
        Node* curr = buckets_[bucketIdx];
        while (curr != nullptr) {
            if (curr->key == key) {
                return &(curr->value);
            }
            curr = curr->next;
        }
        return nullptr;
    }

    bool contains(const K& key) const {
        return (search(key) != nullptr);
    }

    // Remove key from table. Returns true if removed, false if not found.
    bool remove(const K& key) {
        if (capacity_ == 0 || size_ == 0) return false;
        size_t bucketIdx = computeHash(key, capacity_);
        Node* curr = buckets_[bucketIdx];
        Node* prev = nullptr;

        while (curr != nullptr) {
            if (curr->key == key) {
                if (prev != nullptr) {
                    prev->next = curr->next;
                } else {
                    buckets_[bucketIdx] = curr->next;
                }
                delete curr;
                size_--;
                return true;
            }
            prev = curr;
            curr = curr->next;
        }
        return false;
    }

    // Dynamic rehashing to prime >= 2M + 1: relinks existing nodes without reallocating
    void rehash(size_t targetCapacity = 0) {
        if (simulateRehashFailure_) {
            throw std::bad_alloc();
        }

        size_t newCap = targetCapacity;
        if (newCap <= capacity_) {
            newCap = ds_internal::nextPrime(2 * capacity_ + 1);
        } else {
            newCap = ds_internal::nextPrime(newCap);
        }

        std::vector<Node*> newBuckets(newCap, nullptr);

        // Redistribute all nodes into new buckets by relinking
        for (size_t i = 0; i < capacity_; ++i) {
            Node* curr = buckets_[i];
            while (curr != nullptr) {
                Node* nextNode = curr->next;
                size_t newBucketIdx = computeHash(curr->key, newCap);
                curr->next = newBuckets[newBucketIdx];
                newBuckets[newBucketIdx] = curr;
                curr = nextNode;
            }
        }

        buckets_ = std::move(newBuckets);
        capacity_ = newCap;
    }

    // Educational inspection helpers
    size_t bucketCount() const noexcept {
        return capacity_;
    }

    size_t bucketSize(size_t bucketIndex) const {
        if (bucketIndex >= capacity_) return 0;
        size_t count = 0;
        Node* curr = buckets_[bucketIndex];
        while (curr != nullptr) {
            count++;
            curr = curr->next;
        }
        return count;
    }

    void setSimulateRehashFailure(bool fail) noexcept {
        simulateRehashFailure_ = fail;
    }
};
