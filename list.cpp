
#include <memory>

template <size_t N>
class StackStorage {
public:
    char pool[N];
    void* top = &pool;
    size_t size = N;

    StackStorage() = default;

    StackStorage(StackStorage& another) : pool(another.pool), top(another.top), size(another.size) {};
};


template <typename T, size_t N>
class StackAllocator {
public:
    StackStorage<N>* memory;
    using value_type = T;

    template<typename U>
    StackAllocator(const StackAllocator<U, N>& another) : memory(another.memory) {};

    ~StackAllocator() = default;

    StackAllocator(StackStorage<N>& storage) : memory(&storage) {};

    template<typename U>
    StackAllocator& operator=(const StackAllocator<U, N>& another) {
        memory = another.memory;
        return *this;
    }

    template<typename U, size_t L>
    bool operator==(const StackAllocator<U, L>& another) const {
        return memory == another.memory;
    }

    template<typename U, size_t L>
    bool operator!=(const StackAllocator<U, L>& another) const {
        return memory != another.memory;
    }

    template<typename U>
    struct rebind {
        typedef StackAllocator<U, N> other;
    };

    T* allocate(int n) {
        T* piece = static_cast<T*>(std::align(alignof(T), sizeof(T) * n, memory->top, memory->size));
        memory->top = static_cast<T*>(memory->top) + n;
        return piece;
    }

    void deallocate(T* ptr, int n) {
        if (ptr + n == memory->top) {
            memory->top = ptr;
        }
    }
};

template<typename T, typename Allocator = std::allocator<T>>
class List {
public:
    struct BaseNode {
        BaseNode* next_;
        BaseNode* prev_;

        BaseNode() : next_(this), prev_(this) {};
    };

    struct Node: BaseNode {
        T val;
        Node() = default;

        explicit Node(const T& a) : val(a) {};
    };

    template<typename U>
    struct _Iterator {
        BaseNode* node_;

        using iterator_category = std::bidirectional_iterator_tag;
        using value_type = U;
        using pointer = U*;
        using reference = U&;
        using difference_type = int;

        _Iterator() = default;

        explicit _Iterator(BaseNode* node) :node_(node) {};

        ~_Iterator() = default;

        _Iterator(const _Iterator<U>& another) : node_(another.node_) {};

        _Iterator<U>& operator=(const _Iterator<U>& another) {
            _Iterator<U> new_iterator = another;
            std::swap(node_, new_iterator.node_);
            return *this;
        }

        _Iterator<U>& operator++() {
            node_ = node_->next_;
            return *this;
        }

        _Iterator<U>& operator--() {
            node_ = node_->prev_;
            return *this;
        }

        _Iterator<U> operator++(int) {
            _Iterator<U> new_iterator = *this;
            ++(*this);
            return new_iterator;
        }

        _Iterator<U> operator--(int) {
            _Iterator<U> new_iterator = *this;
            --(*this);
            return new_iterator;
        }

        bool operator==(const _Iterator<U> another) const {
            return node_ == another.node_;
        }

        bool operator!=(const _Iterator<U> another) const {
            return node_ != another.node_;
        }

        reference operator*() const {
            return static_cast<Node*>(node_)->val;
        }

        pointer operator->() const {
            return &(static_cast<Node*>(node_))->val;
        }

        operator _Iterator<const T>() const {
            return _Iterator<const T>(node_);
        }
    };

public:
    using iterator = _Iterator<T>;
    using const_iterator = _Iterator<const T>;
    using reverse_iterator = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

private:
    using AllocTraits = std::allocator_traits<Allocator>;
    using AllocatorNode = typename AllocTraits::template rebind_alloc<Node>;
    using AllocNodeTraits = std::allocator_traits<AllocatorNode>;

    size_t sz_ = 0;
    BaseNode head_;
    AllocatorNode alloc_;
    iterator end_;
    iterator begin_;

public:
    List(const Allocator& alloc = Allocator()) : alloc_(alloc), end_(&head_), begin_(&head_) {};

    List(size_t n, const T& val, const Allocator& alloc = Allocator()) : List(alloc) {
        try {
            for (size_t i = 0; i < n; ++i) {
                push_back(val);
            }
        }
        catch (...) {
                throw;
        }
    }

    List(size_t n, const Allocator& alloc = Allocator()) : List(alloc) {
        try {
            for (size_t i = 0; i < n; ++i) {
                push_back();
            }
        }
        catch (...) {
            throw;
        }
    }

    ~List() {
        while (size() > 0) {
            pop_back();
        }
    }

    List(const List& another) : List(AllocTraits::select_on_container_copy_construction(another.alloc_)) {
        try {
            for (const auto& elem: another) {
                push_back(elem);
            }
        }
        catch (...) {
            while (size() > 0) {
                pop_back();
            }
            throw;
        }
    }

    List<T, Allocator>& operator=(const List<T, Allocator>& another) {
        size_t old_sz = sz_;
        BaseNode old_head = head_;
        AllocatorNode old_alloc = alloc_;
        iterator old_end = end_;
        iterator old_begin = begin_;
        try {
            if (AllocNodeTraits::propagate_on_container_copy_assignment::value) {
                alloc_ = another.get_allocator();
            }
            sz_ = 0;
            head_ = BaseNode();
            end_ = iterator(&head_);
            begin_ = ++end();
            for (const auto& elem: another) {
                push_back(elem);
            }
        } catch (...) {
            while (sz_ > 0)
                pop_back();
            sz_ = old_sz;
            head_ = old_head;
            alloc_ = old_alloc;
            end_ = iterator(&head_);
            begin_ = ++end();
            throw;
        }
        for (iterator it = old_begin; it != old_end; ++it) {
            AllocNodeTraits::destroy(old_alloc, static_cast<Node*>(it.node_));
        }
        AllocNodeTraits::deallocate(old_alloc, static_cast<Node*>(old_begin.node_), old_sz);
        return *this;
    }

    Allocator get_allocator() const {
        return alloc_;
    }

    size_t size() const {
        return sz_;
    }

    iterator begin() {
        return begin_;
    }

    iterator end() {
        return end_;
    }

    const_iterator begin() const {
        return begin_;
    }

    const_iterator end() const {
        return end_;
    }

    const_iterator cbegin() const {
        return const_iterator(begin_);
    }

    const_iterator cend() const {
        return const_iterator(end_);
    }

    reverse_iterator rend() {
        return reverse_iterator(begin_);
    }

    reverse_iterator rbegin() {
        return reverse_iterator(end_);
    }

    const_reverse_iterator rend() const {
        return reverse_iterator(begin_);
    }

    const_reverse_iterator rbegin() const {
        return reverse_iterator(end_);
    }

    const_reverse_iterator crend() const {
        return  const_reverse_iterator(begin_);
    }

    const_reverse_iterator crbegin() const {
        return const_reverse_iterator(end_);
    }

    void insert(const_iterator ptr, const T& val) {
        Node* new_node = AllocNodeTraits::allocate(alloc_, 1);
        AllocNodeTraits::construct(alloc_, new_node, val);

        new_node->next_ = ptr.node_;
        new_node->prev_ = ptr.node_->prev_;
        ptr.node_->prev_ = new_node;
        new_node->prev_->next_ = new_node;

        ++sz_;
        begin_ = ++end();
    }

    void insert(const_iterator ptr) {
        Node* new_node = AllocNodeTraits::allocate(alloc_, 1);
        AllocNodeTraits::construct(alloc_, new_node);

        new_node->next_ = ptr.node_;
        new_node->prev_ = ptr.node_->prev_;
        ptr.node_->prev_ = new_node;
        new_node->prev_->next_ = new_node;

        ++sz_;
        begin_ = ++end();
    }

    void erase(const_iterator ptr) {
        Node* old_node = static_cast<Node*>(ptr.node_);
        ptr.node_->prev_->next_ = ptr.node_->next_;
        ptr.node_->next_->prev_ = ptr.node_->prev_;

        AllocNodeTraits::destroy(alloc_, old_node);
        AllocNodeTraits::deallocate(alloc_, old_node, 1);

        --sz_;
        begin_ = ++end();
    }

    void push_back(const T& val) {
        insert(end(), val);
    }

    void push_back() {
        insert(end());
    }

    void push_front(const T& val) {
        insert(begin(), val);
    }

    void push_front() {
        insert(begin());
    }

    void pop_back() {
        erase(--end());
    }

    void pop_front() {
        erase(begin());
    }
};
