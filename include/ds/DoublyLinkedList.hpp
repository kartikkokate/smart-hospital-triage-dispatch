#pragma once

#include <cstddef>
#include <utility>
#include <vector>

/**
 * @brief Custom doubly linked list with bidirectional node pointers.
 * 
 * OWNERSHIP & MEMORY POLICY:
 * - DoublyLinkedList owns all allocated DoublyLinkedNode<T> elements.
 * - Deep-copy semantics prevent shallow copy bugs and double-free vulnerabilities.
 * - Exception-safety: Copy constructor guarantees no memory leaks if node allocation
 *   or element copy throws (strong exception guarantee).
 * - Move semantics transfer head/tail pointers and size in O(1) time without allocations.
 * - Destructor iteratively unlinks and deallocates every node, ensuring O(1) stack space.
 * 
 * EMPTY-STATE BEHAVIOR:
 * - Empty list: head_ == nullptr, tail_ == nullptr, size_ == 0.
 * - Deletion on empty (pop_front, pop_back, remove_at) returns false.
 * - Accessors (front, back, find) return nullptr on empty list or missing key.
 * - Deletion of the final element cleanly resets both head_ and tail_ to nullptr.
 * 
 * COMPLEXITY:
 * - push_front: O(1) time, O(1) auxiliary space
 * - push_back:  O(1) time, O(1) auxiliary space
 * - pop_front:  O(1) time, O(1) auxiliary space
 * - pop_back:   O(1) time, O(1) auxiliary space
 * - insert_at:  O(n) time to index, O(1) insertion
 * - remove_value: O(n) scan, O(1) unlinking
 * - remove_at:    O(n) traversal, O(1) unlinking
 * - find:         O(n) linear search
 * - front/back:   O(1) time
 * - clear:        O(n) time, O(1) stack space
 */
template <typename T>
class DoublyLinkedList {
private:
    struct Node {
        T data;
        Node* prev;
        Node* next;

        explicit Node(const T& val) : data(val), prev(nullptr), next(nullptr) {}
        explicit Node(T&& val) : data(std::move(val)), prev(nullptr), next(nullptr) {}
    };

    Node* head_ = nullptr;
    Node* tail_ = nullptr;
    size_t size_ = 0;

public:
    DoublyLinkedList() = default;

    ~DoublyLinkedList() {
        clear();
    }

    // Explicit Exception-Safe Deep Copy Constructor
    DoublyLinkedList(const DoublyLinkedList& other) {
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

    // Deep Copy Assignment via copy-and-swap
    DoublyLinkedList& operator=(const DoublyLinkedList& other) {
        if (this != &other) {
            DoublyLinkedList temp(other);
            swap(temp);
        }
        return *this;
    }

    // Move Constructor
    DoublyLinkedList(DoublyLinkedList&& other) noexcept
        : head_(other.head_), tail_(other.tail_), size_(other.size_) {
        other.head_ = nullptr;
        other.tail_ = nullptr;
        other.size_ = 0;
    }

    // Move Assignment
    DoublyLinkedList& operator=(DoublyLinkedList&& other) noexcept {
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

    void swap(DoublyLinkedList& other) noexcept {
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
        if (head_ != nullptr) {
            head_->prev = newNode;
        } else {
            tail_ = newNode;
        }
        head_ = newNode;
        size_++;
    }

    void push_front(T&& val) {
        Node* newNode = new Node(std::move(val));
        newNode->next = head_;
        if (head_ != nullptr) {
            head_->prev = newNode;
        } else {
            tail_ = newNode;
        }
        head_ = newNode;
        size_++;
    }

    // O(1) back insertion
    void push_back(const T& val) {
        Node* newNode = new Node(val);
        newNode->prev = tail_;
        if (tail_ != nullptr) {
            tail_->next = newNode;
        } else {
            head_ = newNode;
        }
        tail_ = newNode;
        size_++;
    }

    void push_back(T&& val) {
        Node* newNode = new Node(std::move(val));
        newNode->prev = tail_;
        if (tail_ != nullptr) {
            tail_->next = newNode;
        } else {
            head_ = newNode;
        }
        tail_ = newNode;
        size_++;
    }

    // O(1) front removal
    bool pop_front(T& outVal) {
        if (head_ == nullptr) {
            return false;
        }
        Node* temp = head_;
        outVal = std::move(temp->data);
        head_ = head_->next;
        if (head_ != nullptr) {
            head_->prev = nullptr;
        } else {
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
        if (head_ != nullptr) {
            head_->prev = nullptr;
        } else {
            tail_ = nullptr;
        }
        delete temp;
        size_--;
        return true;
    }

    // O(1) back removal
    bool pop_back(T& outVal) {
        if (tail_ == nullptr) {
            return false;
        }
        Node* temp = tail_;
        outVal = std::move(temp->data);
        tail_ = tail_->prev;
        if (tail_ != nullptr) {
            tail_->next = nullptr;
        } else {
            head_ = nullptr;
        }
        delete temp;
        size_--;
        return true;
    }

    bool pop_back() {
        if (tail_ == nullptr) {
            return false;
        }
        Node* temp = tail_;
        tail_ = tail_->prev;
        if (tail_ != nullptr) {
            tail_->next = nullptr;
        } else {
            head_ = nullptr;
        }
        delete temp;
        size_--;
        return true;
    }

    // O(n) insert at 0-based index
    bool insert_at(size_t index, const T& val) {
        if (index > size_) {
            return false;
        }
        if (index == 0) {
            push_front(val);
            return true;
        }
        if (index == size_) {
            push_back(val);
            return true;
        }
        Node* curr;
        if (index < size_ / 2) {
            curr = head_;
            for (size_t i = 0; i < index; ++i) curr = curr->next;
        } else {
            curr = tail_;
            for (size_t i = size_ - 1; i > index; --i) curr = curr->prev;
        }

        Node* newNode = new Node(val);
        newNode->prev = curr->prev;
        newNode->next = curr;
        curr->prev->next = newNode;
        curr->prev = newNode;
        size_++;
        return true;
    }

    // O(n) remove first matching value
    bool remove_value(const T& val) {
        Node* curr = head_;
        while (curr != nullptr && !(curr->data == val)) {
            curr = curr->next;
        }
        if (curr == nullptr) {
            return false;
        }
        if (curr == head_) {
            return pop_front();
        }
        if (curr == tail_) {
            return pop_back();
        }
        curr->prev->next = curr->next;
        curr->next->prev = curr->prev;
        delete curr;
        size_--;
        return true;
    }

    // O(n) remove at 0-based index
    bool remove_at(size_t index, T& outVal) {
        if (index >= size_) {
            return false;
        }
        if (index == 0) {
            return pop_front(outVal);
        }
        if (index == size_ - 1) {
            return pop_back(outVal);
        }
        Node* curr;
        if (index < size_ / 2) {
            curr = head_;
            for (size_t i = 0; i < index; ++i) curr = curr->next;
        } else {
            curr = tail_;
            for (size_t i = size_ - 1; i > index; --i) curr = curr->prev;
        }
        outVal = std::move(curr->data);
        curr->prev->next = curr->next;
        curr->next->prev = curr->prev;
        delete curr;
        size_--;
        return true;
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

    bool validateIntegrity() const {
        if (head_ == nullptr && tail_ == nullptr && size_ == 0) return true;
        if ((head_ == nullptr) != (tail_ == nullptr)) return false;
        if (head_->prev != nullptr || tail_->next != nullptr) return false;

        size_t forwardCount = 0;
        Node* curr = head_;
        Node* prevNode = nullptr;
        while (curr != nullptr) {
            if (curr->prev != prevNode) return false;
            forwardCount++;
            prevNode = curr;
            curr = curr->next;
        }
        if (prevNode != tail_) return false;
        if (forwardCount != size_) return false;

        size_t backwardCount = 0;
        curr = tail_;
        Node* nextNode = nullptr;
        while (curr != nullptr) {
            if (curr->next != nextNode) return false;
            backwardCount++;
            nextNode = curr;
            curr = curr->prev;
        }
        return (backwardCount == size_);
    }

    std::vector<T> to_vector_forward() const {
        std::vector<T> result;
        result.reserve(size_);
        Node* curr = head_;
        while (curr != nullptr) {
            result.push_back(curr->data);
            curr = curr->next;
        }
        return result;
    }

    std::vector<T> to_vector_backward() const {
        std::vector<T> result;
        result.reserve(size_);
        Node* curr = tail_;
        while (curr != nullptr) {
            result.push_back(curr->data);
            curr = curr->prev;
        }
        return result;
    }
};
