#pragma once

#include <cstddef>
#include <utility>

/**
 * @brief Custom FIFO Queue implemented with head and tail linked nodes.
 * 
 * OWNERSHIP & MEMORY POLICY:
 * - CustomQueue owns all allocated Node elements.
 * - Deep copy prevents shared node pointer bugs and double-free vulnerabilities.
 * - Exception safety: Copy constructor guarantees no memory leaks if node allocation
 *   or element copy throws.
 * - Move semantics transfer head/tail pointers and size in O(1) time.
 * - Iterative destructor guarantees O(1) call stack usage even for large queues.
 * 
 * EMPTY-STATE & RESET INVARIANT:
 * - When empty: head_ == nullptr, tail_ == nullptr, size_ == 0, isEmpty() returns true.
 * - Dequeuing the final remaining element safely resets BOTH head_ and tail_ to nullptr.
 * - Subsequent enqueue operations cleanly reinitialize head_ and tail_ without stale pointers.
 * - Underflow operations (dequeue) return false without modifying output arguments.
 * 
 * COMPLEXITY:
 * - enqueue: O(1) time, O(1) auxiliary space
 * - dequeue: O(1) time, O(1) auxiliary space
 * - front:   O(1) time, O(1) auxiliary space
 * - size:    O(1) time
 * - isEmpty: O(1) time
 * - clear:   O(n) time, O(1) call stack space
 */
template <typename T>
class CustomQueue {
private:
    struct Node {
        T data;
        Node* next;

        explicit Node(const T& val) : data(val), next(nullptr) {}
        explicit Node(T&& val) : data(std::move(val)), next(nullptr) {}
    };

    Node* head_ = nullptr;
    Node* tail_ = nullptr;
    size_t size_ = 0;

public:
    CustomQueue() = default;

    ~CustomQueue() {
        clear();
    }

    // Explicit Exception-Safe Deep Copy Constructor (reconstructs exact FIFO order)
    CustomQueue(const CustomQueue& other) {
        try {
            Node* curr = other.head_;
            while (curr != nullptr) {
                enqueue(curr->data);
                curr = curr->next;
            }
        } catch (...) {
            clear();
            throw;
        }
    }

    // Deep Copy Assignment via copy-and-swap
    CustomQueue& operator=(const CustomQueue& other) {
        if (this != &other) {
            CustomQueue temp(other);
            swap(temp);
        }
        return *this;
    }

    // Move Constructor
    CustomQueue(CustomQueue&& other) noexcept 
        : head_(other.head_), tail_(other.tail_), size_(other.size_) {
        other.head_ = nullptr;
        other.tail_ = nullptr;
        other.size_ = 0;
    }

    // Move Assignment
    CustomQueue& operator=(CustomQueue&& other) noexcept {
        if (this != &other) {
            clear();
            head_ = other.head_;
            tail_ = other.tail_;
            size_ = other.size_;
            other.head_ = nullptr;
            other.tail_ = nullptr;
            other.size_ = 0;
        }
        return *this;
    }

    void swap(CustomQueue& other) noexcept {
        Node* tempHead = head_; head_ = other.head_; other.head_ = tempHead;
        Node* tempTail = tail_; tail_ = other.tail_; other.tail_ = tempTail;
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
        while (head_ != nullptr) {
            Node* temp = head_;
            head_ = head_->next;
            delete temp;
        }
        tail_ = nullptr;
        size_ = 0;
    }

    // O(1) FIFO enqueue
    void enqueue(const T& val) {
        Node* newNode = new Node(val);
        if (tail_ != nullptr) {
            tail_->next = newNode;
            tail_ = newNode;
        } else {
            head_ = newNode;
            tail_ = newNode;
        }
        size_++;
    }

    void enqueue(T&& val) {
        Node* newNode = new Node(std::move(val));
        if (tail_ != nullptr) {
            tail_->next = newNode;
            tail_ = newNode;
        } else {
            head_ = newNode;
            tail_ = newNode;
        }
        size_++;
    }

    // O(1) FIFO dequeue with underflow protection and reset invariant
    bool dequeue(T& outVal) {
        if (head_ == nullptr) {
            return false;
        }
        Node* temp = head_;
        outVal = std::move(temp->data);
        head_ = head_->next;
        if (head_ == nullptr) {
            tail_ = nullptr; // Crucial: reset tail when last element is removed
        }
        delete temp;
        size_--;
        return true;
    }

    bool dequeue() {
        if (head_ == nullptr) {
            return false;
        }
        Node* temp = head_;
        head_ = head_->next;
        if (head_ == nullptr) {
            tail_ = nullptr;
        }
        delete temp;
        size_--;
        return true;
    }

    // O(1) front inspection
    bool front(T& outVal) const {
        if (head_ == nullptr) {
            return false;
        }
        outVal = head_->data;
        return true;
    }

    T* front() {
        return head_ ? &(head_->data) : nullptr;
    }

    const T* front() const {
        return head_ ? &(head_->data) : nullptr;
    }

    // O(n) encapsulated removal of first matching element (e.g. appointment cancellation)
    bool remove(const T& val) {
        if (head_ == nullptr) {
            return false;
        }
        if (head_->data == val) {
            return dequeue();
        }
        Node* curr = head_;
        while (curr->next != nullptr && !(curr->next->data == val)) {
            curr = curr->next;
        }
        if (curr->next == nullptr) {
            return false; // Not found
        }
        Node* target = curr->next;
        curr->next = target->next;
        if (target == tail_) {
            tail_ = curr;
        }
        delete target;
        size_--;
        return true;
    }

    // Encapsulated check if queue contains value
    bool contains(const T& val) const {
        Node* curr = head_;
        while (curr != nullptr) {
            if (curr->data == val) {
                return true;
            }
            curr = curr->next;
        }
        return false;
    }

    // Encapsulated 0-based index lookup (-1 if not found)
    int indexOf(const T& val) const {
        Node* curr = head_;
        int idx = 0;
        while (curr != nullptr) {
            if (curr->data == val) {
                return idx;
            }
            curr = curr->next;
            idx++;
        }
        return -1;
    }

    // Encapsulated insert at index (used to restore exact FIFO position on rolled back operations)
    bool insertAt(size_t index, const T& val) {
        if (index > size_) {
            return false;
        }
        if (index == 0) {
            Node* newNode = new Node(val);
            newNode->next = head_;
            head_ = newNode;
            if (tail_ == nullptr) {
                tail_ = newNode;
            }
            size_++;
            return true;
        }
        if (index == size_) {
            enqueue(val);
            return true;
        }
        Node* curr = head_;
        for (size_t i = 0; i < index - 1; ++i) {
            curr = curr->next;
        }
        Node* newNode = new Node(val);
        newNode->next = curr->next;
        curr->next = newNode;
        size_++;
        return true;
    }

    // Detaches the node matching val from queue without freeing its memory.
    // Returns pointer to detached Node (caller must either call freeNode or reinsertNodeAt).
    // outIndex is set to the 0-based position where the node was located (-1 if not found).
    Node* detachNode(const T& val, int& outIndex) noexcept {
        outIndex = -1;
        if (head_ == nullptr) {
            return nullptr;
        }
        if (head_->data == val) {
            outIndex = 0;
            Node* target = head_;
            head_ = head_->next;
            if (head_ == nullptr) {
                tail_ = nullptr;
            }
            target->next = nullptr;
            size_--;
            return target;
        }

        Node* curr = head_;
        int idx = 0;
        while (curr->next != nullptr && !(curr->next->data == val)) {
            curr = curr->next;
            idx++;
        }

        if (curr->next == nullptr) {
            return nullptr; // Not found
        }

        outIndex = idx + 1;
        Node* target = curr->next;
        curr->next = target->next;
        if (target == tail_) {
            tail_ = curr;
        }
        target->next = nullptr;
        size_--;
        return target;
    }

    // Reinserts an already-allocated node at the specified 0-based index.
    // Strictly non-allocating (noexcept guarantee).
    void reinsertNodeAt(size_t index, Node* node) noexcept {
        if (!node) return;
        if (index == 0 || head_ == nullptr) {
            node->next = head_;
            head_ = node;
            if (tail_ == nullptr) {
                tail_ = node;
            }
            size_++;
            return;
        }

        if (index >= size_) {
            node->next = nullptr;
            if (tail_ != nullptr) {
                tail_->next = node;
                tail_ = node;
            } else {
                head_ = node;
                tail_ = node;
            }
            size_++;
            return;
        }

        Node* curr = head_;
        for (size_t i = 0; i < index - 1; ++i) {
            curr = curr->next;
        }
        node->next = curr->next;
        curr->next = node;
        size_++;
    }

    static void freeNode(Node* node) noexcept {
        delete node;
    }

    // Encapsulated traversal without exposing internal Node pointers
    template <typename Function>
    void forEach(Function&& func) const {
        Node* curr = head_;
        while (curr != nullptr) {
            func(curr->data);
            curr = curr->next;
        }
    }
};
