#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>
#include <utility>
#include "ds/HashTableChaining.hpp" // For prime utilities and HashDispatcher

/**
 * @brief Custom Hash Table using Open Addressing with Linear Probing and Tombstone Deletion.
 * 
 * EDUCATIONAL OBJECTIVE:
 * - Direct comparison with HashTableChaining: evaluates cache locality vs clustering.
 * - Tri-state slot management: EMPTY, OCCUPIED, and DELETED (tombstone sentinel).
 * - Probing continues past DELETED slots during search.
 * - Insertion reuses DELETED slots when the key is not already present.
 * - Bounded probe loop guaranteed to terminate in at most capacity_ iterations,
 *   even if every slot is occupied or deleted.
 * 
 * OWNERSHIP & MEMORY POLICY:
 * - Uses a flat std::vector<Slot> as contiguous array storage.
 * - Copy and move semantics explicitly implemented.
 * - Moved-from state is safely reusable: insertion into a moved-from table (capacity == 0)
 *   automatically restores valid storage before inserting.
 * - Invalidation policy: Pointers returned by search() are invalidated on insertion or rehash.
 * 
 * COMPLEXITY:
 * - insert:     Theta(1) amortized average time, O(M) worst-case under primary clustering.
 * - search:     Theta(1) average time, O(M) worst-case.
 * - remove:     Theta(1) average time, O(M) worst-case (marks DELETED).
 * - rehash:     Theta(n + M) average time to purge tombstones and restore load factor <= 0.70.
 *               Under pathological clustering where every re-insertion probes through
 *               preceding elements, worst-case rehash can take O(n^2) time.
 */

enum class SlotState : uint8_t {
    EMPTY,
    OCCUPIED,
    DELETED // Tombstone
};

template <typename K, typename V>
class HashTableProbing {
public:
    struct Slot {
        K key;
        V value;
        SlotState state = SlotState::EMPTY;
    };

private:
    std::vector<Slot> slots_;
    size_t size_ = 0;
    size_t capacity_ = 17;
    double maxLoadFactor_ = 0.70; // 0.70 load factor ceiling for open addressing

    size_t computeHash(const K& key, size_t cap) const {
        return (cap == 0) ? 0 : ds_internal::HashDispatcher<K>{}(key, cap);
    }

public:
    explicit HashTableProbing(size_t initialCapacity = 17, double maxLoadFactor = 0.70)
        : size_(0), maxLoadFactor_(maxLoadFactor) {
        capacity_ = ds_internal::nextPrime(initialCapacity);
        slots_.resize(capacity_);
    }

    ~HashTableProbing() = default;

    // Explicit Copy Constructor
    HashTableProbing(const HashTableProbing& other)
        : slots_(other.slots_),
          size_(other.size_),
          capacity_(other.capacity_),
          maxLoadFactor_(other.maxLoadFactor_) {}

    // Explicit Copy Assignment
    HashTableProbing& operator=(const HashTableProbing& other) {
        if (this != &other) {
            slots_ = other.slots_;
            size_ = other.size_;
            capacity_ = other.capacity_;
            maxLoadFactor_ = other.maxLoadFactor_;
        }
        return *this;
    }

    // Move Constructor
    HashTableProbing(HashTableProbing&& other) noexcept
        : slots_(std::move(other.slots_)),
          size_(other.size_),
          capacity_(other.capacity_),
          maxLoadFactor_(other.maxLoadFactor_) {
        other.size_ = 0;
        other.capacity_ = 0;
    }

    // Move Assignment
    HashTableProbing& operator=(HashTableProbing&& other) noexcept {
        if (this != &other) {
            slots_ = std::move(other.slots_);
            size_ = other.size_;
            capacity_ = other.capacity_;
            maxLoadFactor_ = other.maxLoadFactor_;
            other.size_ = 0;
            other.capacity_ = 0;
        }
        return *this;
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

    void clear() noexcept {
        for (auto& slot : slots_) {
            slot.state = SlotState::EMPTY;
        }
        size_ = 0;
    }

    size_t hash(const K& key) const {
        return computeHash(key, capacity_);
    }

    // Dynamic rehashing to prime >= 2M + 1: eliminates tombstones and restores load factor <= 0.70
    void rehash(size_t targetCapacity = 0) {
        size_t newCap = targetCapacity;
        if (newCap <= capacity_) {
            newCap = ds_internal::nextPrime(2 * capacity_ + 1);
        } else {
            newCap = ds_internal::nextPrime(newCap);
        }

        std::vector<Slot> oldSlots = std::move(slots_);
        size_t oldCap = capacity_;

        capacity_ = newCap;
        slots_.clear();
        slots_.resize(capacity_);
        size_ = 0;

        for (size_t i = 0; i < oldCap; ++i) {
            if (oldSlots[i].state == SlotState::OCCUPIED) {
                insert(std::move(oldSlots[i].key), std::move(oldSlots[i].value));
            }
        }
    }

    // Insert or update. Restores storage if moved-from (capacity == 0).
    bool insert(const K& key, const V& value) {
        if (capacity_ == 0) {
            capacity_ = ds_internal::nextPrime(17);
            slots_.clear();
            slots_.resize(capacity_);
            size_ = 0;
        }

        if (static_cast<double>(size_ + 1) / static_cast<double>(capacity_) > maxLoadFactor_) {
            rehash();
        }

        size_t h0 = computeHash(key, capacity_);
        int firstTombstone = -1;

        // Bounded probe loop: exactly at most capacity_ iterations
        for (size_t i = 0; i < capacity_; ++i) {
            size_t idx = (h0 + i) % capacity_;

            if (slots_[idx].state == SlotState::OCCUPIED) {
                if (slots_[idx].key == key) {
                    slots_[idx].value = value;
                    return false; // Updated existing key
                }
            } else if (slots_[idx].state == SlotState::DELETED) {
                if (firstTombstone == -1) {
                    firstTombstone = static_cast<int>(idx);
                }
            } else if (slots_[idx].state == SlotState::EMPTY) {
                // Key does not exist; insert at tombstone if available, else at this empty slot
                size_t insertIdx = (firstTombstone != -1) ? static_cast<size_t>(firstTombstone) : idx;
                slots_[insertIdx].key = key;
                slots_[insertIdx].value = value;
                slots_[insertIdx].state = SlotState::OCCUPIED;
                size_++;
                return true;
            }
        }

        // Entire table probed without finding EMPTY slot
        if (firstTombstone != -1) {
            size_t insertIdx = static_cast<size_t>(firstTombstone);
            slots_[insertIdx].key = key;
            slots_[insertIdx].value = value;
            slots_[insertIdx].state = SlotState::OCCUPIED;
            size_++;
            return true;
        }

        // Table is saturated; rehash and reinsert
        rehash();
        return insert(key, value);
    }

    // Search continuing through tombstones and bounded by capacity_
    V* search(const K& key) {
        if (capacity_ == 0 || size_ == 0) return nullptr;
        size_t h0 = computeHash(key, capacity_);

        for (size_t i = 0; i < capacity_; ++i) {
            size_t idx = (h0 + i) % capacity_;
            if (slots_[idx].state == SlotState::EMPTY) {
                return nullptr; // Search ends at EMPTY
            }
            if (slots_[idx].state == SlotState::OCCUPIED && slots_[idx].key == key) {
                return &(slots_[idx].value);
            }
        }
        return nullptr;
    }

    const V* search(const K& key) const {
        if (capacity_ == 0 || size_ == 0) return nullptr;
        size_t h0 = computeHash(key, capacity_);

        for (size_t i = 0; i < capacity_; ++i) {
            size_t idx = (h0 + i) % capacity_;
            if (slots_[idx].state == SlotState::EMPTY) {
                return nullptr;
            }
            if (slots_[idx].state == SlotState::OCCUPIED && slots_[idx].key == key) {
                return &(slots_[idx].value);
            }
        }
        return nullptr;
    }

    bool contains(const K& key) const {
        return (search(key) != nullptr);
    }

    // Remove marks slot as DELETED (tombstone) without breaking search chains
    bool remove(const K& key) {
        if (capacity_ == 0 || size_ == 0) return false;
        size_t h0 = computeHash(key, capacity_);

        for (size_t i = 0; i < capacity_; ++i) {
            size_t idx = (h0 + i) % capacity_;
            if (slots_[idx].state == SlotState::EMPTY) {
                return false;
            }
            if (slots_[idx].state == SlotState::OCCUPIED && slots_[idx].key == key) {
                slots_[idx].state = SlotState::DELETED;
                size_--;
                return true;
            }
        }
        return false;
    }

    // Diagnostic search that also reports the number of probe steps taken
    V* searchWithProbeCount(const K& key, size_t& outProbes) {
        outProbes = 0;
        if (capacity_ == 0 || size_ == 0) return nullptr;
        size_t h0 = computeHash(key, capacity_);

        for (size_t i = 0; i < capacity_; ++i) {
            outProbes++;
            size_t idx = (h0 + i) % capacity_;
            if (slots_[idx].state == SlotState::EMPTY) {
                return nullptr;
            }
            if (slots_[idx].state == SlotState::OCCUPIED && slots_[idx].key == key) {
                return &(slots_[idx].value);
            }
        }
        return nullptr;
    }

    SlotState slotState(size_t idx) const {
        return (idx < capacity_) ? slots_[idx].state : SlotState::EMPTY;
    }

    bool isTombstone(size_t idx) const {
        return (idx < capacity_) && (slots_[idx].state == SlotState::DELETED);
    }

    bool isOccupied(size_t idx) const {
        return (idx < capacity_) && (slots_[idx].state == SlotState::OCCUPIED);
    }

    bool isEmptySlot(size_t idx) const {
        return (idx < capacity_) && (slots_[idx].state == SlotState::EMPTY);
    }

    size_t tombstoneCount() const {
        size_t count = 0;
        for (const auto& s : slots_) {
            if (s.state == SlotState::DELETED) count++;
        }
        return count;
    }
};
