#pragma once

#include <cstddef>
#include <utility>
#include <iterator>

/**
 * @brief Custom singly linked list with head and tail pointers.
 * 
 * OWNERSHIP & MEMORY POLICY:
 * - SinglyLinkedList owns all allocated SinglyLinkedNode<T> elements.
 * - Deep-copy semantics prevent accidental shallow copying of owning pointers.
 * - Exception-safety: Copy constructor guarantees no memory leaks if node allocation
 *   or element copy throws (strong exception guarantee).
 * - Move semantics transfer node ownership in O(1) time without allocation.
 * - Destruction is strictly iterative (O(n) time, O(1) call stack), preventing
 *   recursion stack overflow on long linked chains.
 * 
 * EMPTY-STATE BEHAVIOR:
 * - An empty list has head_ == nullptr, tail_ == nullptr, and size_ == 0.
 * - Underflow operations (pop_front, remove_at) on an empty list return false.
 * - Value accessors (front, back, find) return nullptr on an empty list or missing key.
 * 
 * COMPLEXITY:
 * - push_front: O(1) time, O(1) auxiliary space
 * - push_back:  O(1) time, O(1) auxiliary space
 * - pop_front:  O(1) time, O(1) auxiliary space
 * - insert_after: O(n) time to reach index, O(1) insertion
 * - remove_value: O(n) time to find and unlink, O(1) deletion
 * - remove_at:    O(n) time to reach index, O(1) deletion
 * - find:         O(n) time linear scan
 * - front/back:   O(1) time
 * - size/empty:   O(1) time
 * - clear:        O(n) time, O(1) call stack space
 */
template <typename T>
class SinglyLinkedList {
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
    SinglyLinkedList() = default;

    ~SinglyLinkedList() {
        clear();
    }

    // Explicit Exception-Safe Deep Copy Constructor
    SinglyLinkedList(const SinglyLinkedList& other) {
        try {
            Node* curr = other.head_;
            while (curr != nullptr) {
                push_back(curr->data);
                curr = curr->next;
            }
        } catch (...) {
            clear();
            throw;
        }
    }

    // Deep Copy Assignment via copy-and-swap (provides strong exception safety)
    SinglyLinkedList& operator=(const SinglyLinkedList& other) {
        if (this != &other) {
            SinglyLinkedList temp(other);
            swap(temp);
        }
        return *this;
    }

    // Move Constructor
    SinglyLinkedList(SinglyLinkedList&& other) noexcept 
        : head_(other.head_), tail_(other.tail_), size_(other.size_) {
        other.head_ = nullptr;
        other.tail_ = nullptr;
        other.size_ = 0;
    }

    // Move Assignment
    SinglyLinkedList& operator=(SinglyLinkedList&& other) noexcept {
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

    void swap(SinglyLinkedList& other) noexcept {
        Node* tempHead = head_; head_ = other.head_; other.head_ = tempHead;
        Node* tempTail = tail_; tail_ = other.tail_; other.tail_ = tempTail;
        size_t tempSize = size_; size_ = other.size_; other.size_ = tempSize;
    }

    size_t size() const noexcept {
        return size_;
    }

    bool empty() const noexcept {
        return size_ == 0;
    }

    // Iterative safe cleanup: O(n) time, O(1) call stack space
    void clear() noexcept {
        Node* curr = head_;
        while (curr != nullptr) {
            Node* nextNode = curr->next;
            delete curr;
            curr = nextNode;
        }
        head_ = nullptr;
        tail_ = nullptr;
        size_ = 0;
    }

    // O(1) front insertion
    void push_front(const T& val) {
        Node* newNode = new Node(val);
        newNode->next = head_;
        head_ = newNode;
        if (tail_ == nullptr) {
            tail_ = newNode;
        }
        size_++;
    }

    void push_front(T&& val) {
        Node* newNode = new Node(std::move(val));
        newNode->next = head_;
        head_ = newNode;
        if (tail_ == nullptr) {
            tail_ = newNode;
        }
        size_++;
    }

    // O(1) back insertion using tail_
    void push_back(const T& val) {
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

    void push_back(T&& val) {
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

    // O(1) front removal with underflow protection
    bool pop_front(T& outVal) {
        if (head_ == nullptr) {
            return false;
        }
        Node* temp = head_;
        outVal = std::move(temp->data);
        head_ = head_->next;
        if (head_ == nullptr) {
            tail_ = nullptr;
        }
        delete temp;
        size_--;
        return true;
    }

    bool pop_front() {
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

    // O(n) insert after 0-based index
    bool insert_after(size_t index, const T& val) {
        if (index >= size_) {
            return false;
        }
        Node* curr = head_;
        for (size_t i = 0; i < index; ++i) {
            curr = curr->next;
        }
        Node* newNode = new Node(val);
        newNode->next = curr->next;
        curr->next = newNode;
        if (curr == tail_) {
            tail_ = newNode;
        }
        size_++;
        return true;
    }

    // O(n) insert keeping list sorted by comparator (O(1) if inserted at front)
    template <typename Comparator = std::less<T>>
    void insert_sorted(const T& val, Comparator comp = Comparator()) {
        if (head_ == nullptr || comp(val, head_->data)) {
            push_front(val);
            return;
        }
        Node* curr = head_;
        while (curr->next != nullptr && !comp(val, curr->next->data)) {
            curr = curr->next;
        }
        Node* newNode = new Node(val);
        newNode->next = curr->next;
        curr->next = newNode;
        if (curr == tail_) {
            tail_ = newNode;
        }
        size_++;
    }

    // O(n) removal of first matching value
    bool remove_value(const T& val) {
        if (head_ == nullptr) {
            return false;
        }
        if (head_->data == val) {
            return pop_front();
        }
        Node* curr = head_;
        while (curr->next != nullptr && !(curr->next->data == val)) {
            curr = curr->next;
        }
        if (curr->next == nullptr) {
            return false;
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

    // O(n) removal at 0-based index
    bool remove_at(size_t index, T& outVal) {
        if (index >= size_) {
            return false;
        }
        if (index == 0) {
            return pop_front(outVal);
        }
        Node* curr = head_;
        for (size_t i = 0; i < index - 1; ++i) {
            curr = curr->next;
        }
        Node* target = curr->next;
        outVal = std::move(target->data);
        curr->next = target->next;
        if (target == tail_) {
            tail_ = curr;
        }
        delete target;
        size_--;
        return true;
    }

    // O(n) search returning pointer to data
    const T* find(const T& val) const {
        Node* curr = head_;
        while (curr != nullptr) {
            if (curr->data == val) {
                return &(curr->data);
            }
            curr = curr->next;
        }
        return nullptr;
    }

    T* find(const T& val) {
        Node* curr = head_;
        while (curr != nullptr) {
            if (curr->data == val) {
                return &(curr->data);
            }
            curr = curr->next;
        }
        return nullptr;
    }

    T* front() {
        return head_ ? &(head_->data) : nullptr;
    }

    const T* front() const {
        return head_ ? &(head_->data) : nullptr;
    }

    T* back() {
        return tail_ ? &(tail_->data) : nullptr;
    }

    const T* back() const {
        return tail_ ? &(tail_->data) : nullptr;
    }

    class ConstIterator {
    private:
        const Node* curr_;
    public:
        using iterator_category = std::forward_iterator_tag;
        using value_type = T;
        using difference_type = std::ptrdiff_t;
        using pointer = const T*;
        using reference = const T&;

        explicit ConstIterator(const Node* node) noexcept : curr_(node) {}
        reference operator*() const noexcept { return curr_->data; }
        pointer operator->() const noexcept { return &(curr_->data); }
        ConstIterator& operator++() noexcept { curr_ = curr_->next; return *this; }
        ConstIterator operator++(int) noexcept { ConstIterator tmp = *this; curr_ = curr_->next; return tmp; }
        bool operator==(const ConstIterator& other) const noexcept { return curr_ == other.curr_; }
        bool operator!=(const ConstIterator& other) const noexcept { return curr_ != other.curr_; }
    };

    class Iterator {
    private:
        Node* curr_;
    public:
        using iterator_category = std::forward_iterator_tag;
        using value_type = T;
        using difference_type = std::ptrdiff_t;
        using pointer = T*;
        using reference = T&;

        explicit Iterator(Node* node) noexcept : curr_(node) {}
        reference operator*() const noexcept { return curr_->data; }
        pointer operator->() const noexcept { return &(curr_->data); }
        Iterator& operator++() noexcept { curr_ = curr_->next; return *this; }
        Iterator operator++(int) noexcept { Iterator tmp = *this; curr_ = curr_->next; return tmp; }
        bool operator==(const Iterator& other) const noexcept { return curr_ == other.curr_; }
        bool operator!=(const Iterator& other) const noexcept { return curr_ != other.curr_; }
    };

    Iterator begin() noexcept { return Iterator(head_); }
    Iterator end() noexcept { return Iterator(nullptr); }
    ConstIterator begin() const noexcept { return ConstIterator(head_); }
    ConstIterator end() const noexcept { return ConstIterator(nullptr); }
    ConstIterator cbegin() const noexcept { return ConstIterator(head_); }
    ConstIterator cend() const noexcept { return ConstIterator(nullptr); }
};
