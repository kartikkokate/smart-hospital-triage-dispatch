#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>
#include <utility>
#include <functional>
#include <string>
#include <algorithm>
#include "models/Patient.hpp"

/**
 * @brief Generic Binary Min-Heap parameterized by element type and comparator.
 * 
 * DESIGN RATIONALE:
 * - Backed by a contiguous std::vector<T> for optimal cache locality.
 * - Accepts arbitrary comparators; default is std::less<T> (min-heap).
 * - Kept strictly isolated from triage handle tracking so routing algorithms
 *   (Dijkstra's shortest path, Prim's MST) execute with zero tracking overhead.
 * - Read-only peek() guarantees callers cannot silently corrupt heap order.
 * 
 * COMPLEXITY:
 * - insert:      O(log n) time.
 * - extractMin:  O(log n) time.
 * - peek:        O(1) time (read-only inspection).
 * - buildHeap:   O(n) time via bottom-up sift-down construction.
 * - size/empty:  O(1) time.
 * 
 * REFERENCE INVALIDATION POLICY:
 * - Dynamic vector resizes or element swaps invalidate references/pointers to internal
 *   heap elements. Callers should store values or stable keys rather than raw heap references.
 */
template <typename T, typename Comparator = std::less<T>>
class BinaryMinHeap {
protected:
    std::vector<T> heap_;
    Comparator comp_;

    static size_t parent(size_t i) noexcept { return (i - 1) / 2; }
    static size_t leftChild(size_t i) noexcept { return 2 * i + 1; }
    static size_t rightChild(size_t i) noexcept { return 2 * i + 2; }

    virtual void onSwap(size_t i, size_t j) {
        std::swap(heap_[i], heap_[j]);
    }

    void siftUp(size_t i) {
        while (i > 0) {
            size_t p = parent(i);
            if (comp_(heap_[i], heap_[p])) {
                onSwap(i, p);
                i = p;
            } else {
                break;
            }
        }
    }

    void siftDown(size_t i) {
        size_t n = heap_.size();
        while (leftChild(i) < n) {
            size_t left = leftChild(i);
            size_t right = rightChild(i);
            size_t smallest = i;

            if (comp_(heap_[left], heap_[smallest])) {
                smallest = left;
            }
            if (right < n && comp_(heap_[right], heap_[smallest])) {
                smallest = right;
            }

            if (smallest != i) {
                onSwap(i, smallest);
                i = smallest;
            } else {
                break;
            }
        }
    }

public:
    explicit BinaryMinHeap(Comparator comp = Comparator()) : comp_(comp) {}

    virtual ~BinaryMinHeap() = default;

    BinaryMinHeap(const BinaryMinHeap&) = default;
    BinaryMinHeap& operator=(const BinaryMinHeap&) = default;
    BinaryMinHeap(BinaryMinHeap&&) noexcept = default;
    BinaryMinHeap& operator=(BinaryMinHeap&&) noexcept = default;

    size_t size() const noexcept { return heap_.size(); }
    bool empty() const noexcept { return heap_.empty(); }

    virtual void clear() noexcept {
        heap_.clear();
    }

    // O(log n) insertion
    void insert(const T& val) {
        size_t idx = heap_.size();
        heap_.push_back(val);
        siftUp(idx);
    }

    void insert(T&& val) {
        size_t idx = heap_.size();
        heap_.push_back(std::move(val));
        siftUp(idx);
    }

    // O(1) read-only root inspection (prevents accidental external invariant violation)
    const T* peek() const noexcept {
        return heap_.empty() ? nullptr : &heap_[0];
    }

    // O(log n) min extraction
    bool extractMin(T& outVal) {
        if (heap_.empty()) return false;
        outVal = std::move(heap_[0]);
        if (heap_.size() == 1) {
            heap_.pop_back();
            return true;
        }
        heap_[0] = std::move(heap_.back());
        heap_.pop_back();
        siftDown(0);
        return true;
    }

    bool extractMin() {
        if (heap_.empty()) return false;
        if (heap_.size() == 1) {
            heap_.pop_back();
            return true;
        }
        heap_[0] = std::move(heap_.back());
        heap_.pop_back();
        siftDown(0);
        return true;
    }

    // O(n) bottom-up heap construction
    void buildHeap(const std::vector<T>& items) {
        heap_ = items;
        if (heap_.size() <= 1) return;
        for (size_t i = (heap_.size() / 2); i > 0; --i) {
            siftDown(i - 1);
        }
    }
};

/**
 * @brief Record structure stored in the emergency triage min-heap.
 */
struct TriageRecord {
    int internalHandle = -1;
    std::string patientId;
    UrgencyLevel urgency = UrgencyLevel::NON_URGENT;
    uint64_t arrivalSequence = 0;

    TriageRecord() = default;
    TriageRecord(int handle, const std::string& id, UrgencyLevel urg, uint64_t seq)
        : internalHandle(handle), patientId(id), urgency(urg), arrivalSequence(seq) {}
};

/**
 * @brief Composite comparator enforcing Urgency primary ordering and FIFO secondary tie-breaker.
 */
struct TriageRecordComparator {
    bool operator()(const TriageRecord& a, const TriageRecord& b) const noexcept {
        int uA = urgencyToInt(a.urgency);
        int uB = urgencyToInt(b.urgency);
        if (uA != uB) {
            return uA < uB; // Lower numeric value = higher medical priority (1 is most acute)
        }
        return a.arrivalSequence < b.arrivalSequence; // FIFO tie-breaker within same urgency
    }
};

/**
 * @brief Triage Min-Heap with Direct Handle Tracking (heapPosByHandle).
 * 
 * KEY FEATURES & INVARIANTS:
 * - Directly indexed std::vector<int> heapPosByHandle maintains current heap position
 *   for every registered patient handle in O(1) lookup time.
 * - Supported handle capacity: Handles must satisfy 0 <= handle < kMaxSupportedHandle (100,000).
 * - Urgency Validation: Urgency levels must strictly satisfy 1 <= urgency <= 5.
 * - Position updates are strictly executed on every swap, insert, extract, arbitrary removal,
 *   and transactional bottom-up rebuild.
 * - Transactional buildHeap: Fully pre-validates all records before touching live state.
 *   If any record fails validation or allocation, the original heap remains untouched.
 * - Dynamic priority updates (changeUrgency) support both urgency increases (sift-up) and
 *   decreases (sift-down) while preserving the original arrivalSequence.
 * - Root protection: Explicit `i > 0` condition prevents nonexistent-parent access at root.
 * 
 * COMPLEXITY:
 * - insert:         O(log n) amortized; up to O(H) on initial handle vector expansion where H is max handle.
 * - changeUrgency:  O(log n) time.
 * - removePatient:  O(log n) time.
 * - extractMin:     O(log n) time.
 * - peek:           O(1) read-only inspection.
 * - clear:          O(H) time to reset the handle-position vector.
 */
class TriageMinHeap {
public:
    static constexpr int kMaxSupportedHandle = 100000;

private:
    std::vector<TriageRecord> heap_;
    std::vector<int> heapPosByHandle_;
    TriageRecordComparator comp_;

    static size_t parent(size_t i) noexcept { return (i - 1) / 2; }
    static size_t leftChild(size_t i) noexcept { return 2 * i + 1; }
    static size_t rightChild(size_t i) noexcept { return 2 * i + 2; }

    static bool isValidUrgency(UrgencyLevel u) noexcept {
        int val = urgencyToInt(u);
        return (val >= 1 && val <= 5);
    }

    void swapRecords(size_t i, size_t j) noexcept {
        std::swap(heap_[i], heap_[j]);
        heapPosByHandle_[heap_[i].internalHandle] = static_cast<int>(i);
        heapPosByHandle_[heap_[j].internalHandle] = static_cast<int>(j);
    }

    void siftUp(size_t i) {
        while (i > 0) { // Strictly protects root from parent access
            size_t p = parent(i);
            if (comp_(heap_[i], heap_[p])) {
                swapRecords(i, p);
                i = p;
            } else {
                break;
            }
        }
    }

    void siftDown(size_t i) {
        size_t n = heap_.size();
        while (leftChild(i) < n) {
            size_t left = leftChild(i);
            size_t right = rightChild(i);
            size_t smallest = i;

            if (comp_(heap_[left], heap_[smallest])) {
                smallest = left;
            }
            if (right < n && comp_(heap_[right], heap_[smallest])) {
                smallest = right;
            }

            if (smallest != i) {
                swapRecords(i, smallest);
                i = smallest;
            } else {
                break;
            }
        }
    }

public:
    TriageMinHeap() = default;

    size_t size() const noexcept { return heap_.size(); }
    bool empty() const noexcept { return heap_.empty(); }

    void clear() noexcept {
        heap_.clear();
        std::fill(heapPosByHandle_.begin(), heapPosByHandle_.end(), -1);
    }

    // Insert record with handle boundary, duplicate active handle, and urgency validation
    bool insert(const TriageRecord& record) {
        if (record.internalHandle < 0 || record.internalHandle >= kMaxSupportedHandle) {
            return false; // Reject negative or excessively large handles
        }
        if (!isValidUrgency(record.urgency)) {
            return false; // Reject out-of-range urgency
        }

        size_t handleIdx = static_cast<size_t>(record.internalHandle);
        if (handleIdx < heapPosByHandle_.size()) {
            if (heapPosByHandle_[handleIdx] != -1) {
                return false; // Reject duplicate active handle
            }
        } else {
            heapPosByHandle_.resize(handleIdx + 1, -1);
        }

        size_t newIdx = heap_.size();
        heap_.push_back(record);
        heapPosByHandle_[record.internalHandle] = static_cast<int>(newIdx);
        siftUp(newIdx);
        return true;
    }

    // Read-only root inspection
    const TriageRecord* peek() const noexcept {
        return heap_.empty() ? nullptr : &heap_[0];
    }

    // Extract root element and update handle positions
    bool extractMin(TriageRecord& outRecord) {
        if (heap_.empty()) return false;
        outRecord = heap_[0];
        int removedHandle = heap_[0].internalHandle;

        if (heap_.size() == 1) {
            heap_.pop_back();
            heapPosByHandle_[removedHandle] = -1;
            return true;
        }

        heap_[0] = heap_.back();
        heapPosByHandle_[heap_[0].internalHandle] = 0;
        heap_.pop_back();
        heapPosByHandle_[removedHandle] = -1;

        siftDown(0);
        return true;
    }

    // Dynamic priority update preserving arrival sequence
    bool changeUrgency(int handle, UrgencyLevel newUrgency) {
        if (!isValidUrgency(newUrgency)) {
            return false; // Reject invalid urgency without modifying state
        }
        if (handle < 0 || static_cast<size_t>(handle) >= heapPosByHandle_.size()) {
            return false;
        }
        int pos = heapPosByHandle_[handle];
        if (pos == -1 || static_cast<size_t>(pos) >= heap_.size()) {
            return false; // Handle not currently active in heap
        }

        UrgencyLevel oldUrgency = heap_[pos].urgency;
        if (oldUrgency == newUrgency) {
            return true; // Safe no-op
        }

        heap_[pos].urgency = newUrgency; // Preserves arrivalSequence

        if (urgencyToInt(newUrgency) < urgencyToInt(oldUrgency)) {
            siftUp(static_cast<size_t>(pos));
        } else {
            siftDown(static_cast<size_t>(pos));
        }

        return true;
    }

    // Arbitrary patient removal in O(log n) time
    bool removePatient(int handle) {
        if (handle < 0 || static_cast<size_t>(handle) >= heapPosByHandle_.size()) {
            return false;
        }
        int pos = heapPosByHandle_[handle];
        if (pos == -1 || static_cast<size_t>(pos) >= heap_.size()) {
            return false;
        }

        size_t idx = static_cast<size_t>(pos);
        size_t lastIdx = heap_.size() - 1;

        if (idx == lastIdx) {
            heap_.pop_back();
            heapPosByHandle_[handle] = -1;
            return true;
        }

        TriageRecord replacement = heap_.back();
        heap_.pop_back();
        heapPosByHandle_[handle] = -1;

        heap_[idx] = replacement;
        heapPosByHandle_[replacement.internalHandle] = static_cast<int>(idx);

        if (idx > 0 && comp_(heap_[idx], heap_[parent(idx)])) {
            siftUp(idx);
        } else {
            siftDown(idx);
        }

        return true;
    }

    // Transactional bottom-up construction in O(n) time
    bool buildHeap(const std::vector<TriageRecord>& records) {
        if (records.empty()) {
            clear();
            return true;
        }

        // Phase 1: Validate all records and handles on temporary structures
        int maxHandle = -1;
        for (const auto& r : records) {
            if (r.internalHandle < 0 || r.internalHandle >= kMaxSupportedHandle) {
                return false; // Reject invalid handle without modifying state
            }
            if (!isValidUrgency(r.urgency)) {
                return false; // Reject invalid urgency without modifying state
            }
            if (r.internalHandle > maxHandle) {
                maxHandle = r.internalHandle;
            }
        }

        std::vector<int> tempPos(maxHandle + 1, -1);
        for (const auto& r : records) {
            if (tempPos[r.internalHandle] != -1) {
                return false; // Duplicate handle in batch: reject transactionally
            }
            tempPos[r.internalHandle] = 0; // Mark seen
        }

        // Phase 2: Assemble and heapify on temporary vector
        std::vector<TriageRecord> tempHeap = records;
        for (size_t i = 0; i < tempHeap.size(); ++i) {
            tempPos[tempHeap[i].internalHandle] = static_cast<int>(i);
        }

        if (tempHeap.size() > 1) {
            auto tempSwap = [&](size_t i, size_t j) {
                std::swap(tempHeap[i], tempHeap[j]);
                tempPos[tempHeap[i].internalHandle] = static_cast<int>(i);
                tempPos[tempHeap[j].internalHandle] = static_cast<int>(j);
            };

            auto tempSiftDown = [&](size_t i) {
                size_t n = tempHeap.size();
                while (leftChild(i) < n) {
                    size_t left = leftChild(i);
                    size_t right = rightChild(i);
                    size_t smallest = i;

                    if (comp_(tempHeap[left], tempHeap[smallest])) {
                        smallest = left;
                    }
                    if (right < n && comp_(tempHeap[right], tempHeap[smallest])) {
                        smallest = right;
                    }

                    if (smallest != i) {
                        tempSwap(i, smallest);
                        i = smallest;
                    } else {
                        break;
                    }
                }
            };

            for (size_t i = (tempHeap.size() / 2); i > 0; --i) {
                tempSiftDown(i - 1);
            }
        }

        // Phase 3: Atomically commit to live state
        heap_ = std::move(tempHeap);
        heapPosByHandle_ = std::move(tempPos);
        return true;
    }

    int getPositionByHandle(int handle) const noexcept {
        if (handle < 0 || static_cast<size_t>(handle) >= heapPosByHandle_.size()) {
            return -1;
        }
        return heapPosByHandle_[handle];
    }

    bool isPatientActive(int handle) const noexcept {
        return getPositionByHandle(handle) != -1;
    }

    const std::vector<TriageRecord>& getRawRecords() const noexcept {
        return heap_;
    }

    bool validateInvariants() const {
        size_t n = heap_.size();
        for (size_t i = 1; i < n; ++i) {
            size_t p = parent(i);
            if (comp_(heap_[i], heap_[p])) {
                return false;
            }
        }

        for (size_t i = 0; i < n; ++i) {
            int h = heap_[i].internalHandle;
            if (h < 0 || static_cast<size_t>(h) >= heapPosByHandle_.size()) {
                return false;
            }
            if (heapPosByHandle_[h] != static_cast<int>(i)) {
                return false;
            }
        }

        for (size_t h = 0; h < heapPosByHandle_.size(); ++h) {
            int pos = heapPosByHandle_[h];
            if (pos != -1) {
                if (pos < 0 || static_cast<size_t>(pos) >= n) {
                    return false;
                }
                if (heap_[pos].internalHandle != static_cast<int>(h)) {
                    return false;
                }
            }
        }

        return true;
    }
};
