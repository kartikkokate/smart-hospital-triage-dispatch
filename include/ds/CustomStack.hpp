#pragma once

#include <cstddef>
#include <utility>

/**
 * @brief Custom LIFO Stack implemented with singly linked nodes.
 * 
 * OWNERSHIP & MEMORY POLICY:
 * - CustomStack owns all allocated Node elements.
 * - Deep copy prevents shared node pointer bugs and double-free vulnerabilities.
 * - Exception safety: Copy constructor guarantees no memory leaks if node allocation
 *   or element copy throws.
 * - Move semantics transfer ownership in O(1) time without allocations.
 * - Iterative destructor guarantees O(1) call stack usage even for deep stacks.
 * 
 * EMPTY-STATE & UNDERFLOW BEHAVIOR:
 * - When empty: top_ == nullptr, size_ == 0, isEmpty() returns true.
 * - pop(outVal) on an empty stack returns false without modifying outVal.
 * - top() on an empty stack returns nullptr.
 * 
 * COMPLEXITY:
 * - push:    O(1) time, O(1) auxiliary space
 * - pop:     O(1) time, O(1) auxiliary space
 * - top:     O(1) time, O(1) auxiliary space
 * - size:    O(1) time
 * - isEmpty: O(1) time
 * - clear:   O(n) time, O(1) call stack space
 */
template <typename T>
class CustomStack {
private:
    struct Node {
        T data;
        Node* next;

        explicit Node(const T& val) : data(val), next(nullptr) {}
        explicit Node(T&& val) : data(std::move(val)), next(nullptr) {}
    };

    Node* top_ = nullptr;
    size_t size_ = 0;

public:
    CustomStack() = default;

    ~CustomStack() {
        clear();
    }

    // Explicit Exception-Safe Deep Copy Constructor (reconstructs exact LIFO order)
    CustomStack(const CustomStack& other) {
        Node* headRev = nullptr;
        try {
            Node* curr = other.top_;
            while (curr != nullptr) {
                Node* revNode = new Node(curr->data);
                revNode->next = headRev;
                headRev = revNode;
                curr = curr->next;
            }

            while (headRev != nullptr) {
                Node* temp = headRev;
                push(std::move(temp->data));
                headRev = headRev->next;
                delete temp;
            }
        } catch (...) {
            while (headRev != nullptr) {
                Node* temp = headRev;
                headRev = headRev->next;
                delete temp;
            }
            clear();
            throw;
        }
    }

    // Deep Copy Assignment via copy-and-swap
    CustomStack& operator=(const CustomStack& other) {
        if (this != &other) {
            CustomStack temp(other);
            swap(temp);
        }
        return *this;
    }

    // Move Constructor
    CustomStack(CustomStack&& other) noexcept 
        : top_(other.top_), size_(other.size_) {
        other.top_ = nullptr;
        other.size_ = 0;
    }

    // Move Assignment
    CustomStack& operator=(CustomStack&& other) noexcept {
        if (this != &other) {
            clear();
            top_ = other.top_;
            size_ = other.size_;
            other.top_ = nullptr;
            other.size_ = 0;
        }
        return *this;
    }

    void swap(CustomStack& other) noexcept {
        Node* tempTop = top_; top_ = other.top_; other.top_ = tempTop;
        size_t tempSize = size_; size_ = other.size_; other.size_ = tempSize;
    }

    size_t size() const noexcept {
        return size_;
    }

    bool isEmpty() const noexcept {
        return size_ == 0;
    }

    bool empty() const noexcept {
        return size_ == 0;
    }

    // Iterative safe cleanup: O(n) time, O(1) stack space
    void clear() noexcept {
        while (top_ != nullptr) {
            Node* temp = top_;
            top_ = top_->next;
            delete temp;
        }
        size_ = 0;
    }

    // O(1) push
    void push(const T& val) {
        Node* newNode = new Node(val);
        newNode->next = top_;
        top_ = newNode;
        size_++;
    }

    void push(T&& val) {
        Node* newNode = new Node(std::move(val));
        newNode->next = top_;
        top_ = newNode;
        size_++;
    }

    // O(1) pop with underflow protection
    bool pop(T& outVal) {
        if (top_ == nullptr) {
            return false;
        }
        Node* temp = top_;
        outVal = std::move(temp->data);
        top_ = top_->next;
        delete temp;
        size_--;
        return true;
    }

    bool pop() {
        if (top_ == nullptr) {
            return false;
        }
        Node* temp = top_;
        top_ = top_->next;
        delete temp;
        size_--;
        return true;
    }

    // O(1) top access
    bool top(T& outVal) const {
        if (top_ == nullptr) {
            return false;
        }
        outVal = top_->data;
        return true;
    }

    T* top() {
        return top_ ? &(top_->data) : nullptr;
    }

    const T* top() const {
        return top_ ? &(top_->data) : nullptr;
    }
};
