#include <iostream>

template <size_t N>
class StackStorage {
  private:
  char data_first[N];
  size_t shift = 0;

  public:
  StackStorage() {}
  StackStorage(const StackStorage&) = delete;
  StackStorage& operator=(const StackStorage&) = delete;

  char* allocate(size_t n, const size_t alignof_) {
    shift += (alignof_ - (reinterpret_cast<size_t>(data_first + shift) % alignof_)) % alignof_;
    char* data_second_copy = data_first + shift;
    shift += n;
    return data_second_copy;
  }
};

template <typename T, size_t N>
class StackAllocator {
  private:
  template <typename U, size_t M> friend class StackAllocator;
  StackStorage<N>* storage {};

  public:
  using value_type = T;

  template <typename U> struct rebind {typedef StackAllocator<U, N> other; };

  StackAllocator() = default;
  ~StackAllocator() = default;

  StackAllocator(StackStorage<N>& other_storage) : storage(&other_storage) {}

  template <typename U>
  StackAllocator(const StackAllocator<U, N>& other) : storage(other.storage) {}

  template <typename U>
  StackAllocator& operator=(const StackAllocator<U, N>& other_allocator) {
    storage = other_allocator.storage;
    return *this;
  }

  T* allocate(size_t const n) {
    return reinterpret_cast<T*>(storage->allocate(n * sizeof(T), alignof(T)));
  }

  void deallocate(T* const, size_t const) {}

  template <typename U, size_t M>
  bool operator==(StackAllocator<U, M> const& other_alloc) const {
    return storage == other_alloc.storage;
  }

  template <typename U, size_t M>
  bool operator!=(StackAllocator<U, M> const& other_alloc) const {
    return !(*this == other_alloc);
  }
};

template <typename T, typename Allocator = std::allocator<T>>
class List {
  private:
  size_t size_ = 0;
  struct BaseNode {
    BaseNode* next;
    BaseNode* prev;
    BaseNode() : next(nullptr), prev(nullptr) {}
    BaseNode(BaseNode* nxt, BaseNode* pr) : next(nxt), prev(pr) {}
  };

  struct Node : BaseNode {
    T value;
    Node() {};
    Node(BaseNode* pr, BaseNode* nxt, const T& value) : BaseNode(nxt, pr), value(value) {}
    Node(BaseNode* pr, BaseNode* nxt) : BaseNode(nxt, pr) {}
  };

  BaseNode fake;
  [[ no_unique_address ]] Allocator allocator;
  using AllocTraits = std::allocator_traits<Allocator>;
  using NodeAlloc = typename AllocTraits::template rebind_alloc<Node>;
  NodeAlloc node_allocator = allocator;
  using NodeAllocTraits = typename AllocTraits::template rebind_traits<Node>;

  void initialize_fake() {
    fake.prev = &fake;
    fake.next = &fake;
  }

  void clear_list(BaseNode* last) {
    while (last != &fake) {
      last = last->prev;
      NodeAllocTraits::destroy(node_allocator, static_cast<Node*>(last->next));
      NodeAllocTraits::deallocate(node_allocator, static_cast<Node*>(last->next), 1);
    }
  }

  public:

  ~List() {
    if (size_ == 0) return;
    Node* it = static_cast<Node*>(fake.next);
    for (size_t i = 0; i < size_; ++i) {
      Node* next = static_cast<Node*>(it->next);
      NodeAllocTraits::destroy(node_allocator, it);
      NodeAllocTraits::deallocate(node_allocator, it, 1);
      it = next;
    }
  }

  List() : List(Allocator()) {}

  List(Allocator allocator2) : allocator(allocator2) {
    size_ = 0;
    initialize_fake();
  }

  List(size_t amount) : List(amount, Allocator()) {}

  List(size_t amount, Allocator allocator2) : allocator(allocator2) {
    size_ = amount;
    initialize_fake();
    BaseNode* last = &fake;
    for (size_t i = 0; i < amount; ++i) {
      try {
        Node* new_node = NodeAllocTraits::allocate(node_allocator, 1);
        NodeAllocTraits::construct(node_allocator, new_node, last, &fake);
        last->next = new_node;
        fake.prev = new_node;
        last = new_node;
      } catch(...) {
        clear_list(last);
        throw;
      }
    }
  }

  List(size_t amount, const T& value) : List(amount, value, Allocator()) {}

  List(size_t amount, const T& value, Allocator allocator2) : allocator(allocator2) {
    size_ = amount;
    initialize_fake();
    BaseNode* last = &fake;
    for (size_t i = 0; i < amount; ++i) {
      try {
        Node* new_node = NodeAllocTraits::allocate(node_allocator, 1);
        NodeAllocTraits::construct(node_allocator, new_node, last, &fake, value);
        last->next = new_node;
        fake.prev = new_node;
        last = new_node;
      } catch(...) {
        clear_list(last);
        throw;
      }
    }
  }
  List(const List& list) : allocator(AllocTraits::select_on_container_copy_construction(list.allocator)) {
    size_ = list.size();
    initialize_fake();
    BaseNode* last = &fake;
    const_iterator it = list.cbegin();
    for (size_t i = 0; i < size_; ++i) {
      try {
        Node* new_node = NodeAllocTraits::allocate(node_allocator, 1);
        NodeAllocTraits::construct(node_allocator, new_node, last, &fake, *it);
        last->next = new_node;
        fake.prev = new_node;
        last = new_node;
        ++it;
      } catch(...) {
        clear_list(last);
        throw;
      }
    }
  }

  List& operator=(const List& list) {
    if (AllocTraits::propagate_on_container_copy_assignment::value) {
      allocator = list.allocator;
    }
    List copy(allocator);
    const_iterator it = list.cbegin();
    for (size_t i = 0; i < list.size(); ++i) {
      copy.push_back(*it);
      ++it;
    }
    std::swap(size_, copy.size_);
    std::swap(fake, copy.fake);
    return *this;
  }

  Allocator get_allocator() { return allocator; }

  size_t size() const { return size_; }

  template <bool is_const>
  class Iterator {
    private:
    using node_type = std::conditional_t<is_const, const BaseNode*, BaseNode*>;
    using Node_type = std::conditional_t<is_const, const Node*, Node*>;
    node_type ptr;

    public:

    node_type get_ptr() const { return ptr; }
    using value_type = T;
    using reference = std::conditional_t<is_const, const T&, T&>;
    using iterator_category = std::bidirectional_iterator_tag;
    using difference_type = ptrdiff_t;

    Iterator() = default;
    ~Iterator() = default;

    Iterator(node_type base_node) : ptr(base_node) {}
    Iterator(const Iterator<false>& iterator) : ptr(iterator.get_ptr()) {}

    Iterator& operator=(const Iterator& iterator) {
      ptr = iterator.get_ptr();
      return *this;
    }

    reference operator*() { return static_cast<Node_type>(ptr)->value; }

    Iterator& operator++() {
      ptr = ptr->next;
      return *this;
    }
    Iterator operator++(int) {
      Iterator copy = *this;
      ptr = ptr->next;
      return copy;
    }
    Iterator& operator--() {
      ptr = ptr->prev;
      return *this;
    }
    Iterator operator--(int) {
      Iterator copy = *this;
      ptr = ptr->prev;
      return copy;
    }

    bool operator==(const Iterator& it) const { return ptr == it.get_ptr(); }
    bool operator!=(const Iterator& it) const { return ptr != it.get_ptr(); }
  };

  using iterator = Iterator<false>;
  using const_iterator = Iterator<true>;
  using reverse_iterator = std::reverse_iterator<iterator>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;

  iterator begin() { return iterator(fake.next); }
  const_iterator begin() const { return const_iterator(fake.next); }
  const_iterator cbegin() const { return const_iterator(fake.next); }

  iterator end() { return iterator(&fake); }
  const_iterator end() const { return const_iterator(&fake); }
  const_iterator cend() const { return const_iterator(&fake); }

  reverse_iterator rbegin() { return reverse_iterator(&fake); }
  const_reverse_iterator rbegin() const { return const_reverse_iterator(&fake); }
  reverse_iterator rend() { return reverse_iterator(fake.next); }
  const_reverse_iterator rend() const { return const_reverse_iterator(fake.next); }

  const_reverse_iterator crbegin() const { return const_reverse_iterator(&fake); }
  const_reverse_iterator crend() const { return const_reverse_iterator(fake.next); }

  void insert(const_iterator it, const T& val) {
    BaseNode* cur_vertex = const_cast<BaseNode*>(it.get_ptr());
    BaseNode* prev_vertex = cur_vertex->prev;
    Node* new_vertex = NodeAllocTraits::allocate(node_allocator, 1);
    NodeAllocTraits::construct(node_allocator, new_vertex, prev_vertex, cur_vertex, val);
    prev_vertex->next = new_vertex;
    cur_vertex->prev = new_vertex;
    ++size_;
  }

  void erase(const_iterator it) {
    auto ans = it.get_ptr()->next;
    auto prev = it.get_ptr()->prev;
    const_cast<BaseNode*>(prev)->next = ans;
    const_cast<BaseNode*>(ans)->prev = it.get_ptr()->prev;
    --size_;
    NodeAllocTraits::destroy(node_allocator, static_cast<Node*>(const_cast<BaseNode*>(it.get_ptr())));
    NodeAllocTraits::deallocate(node_allocator, static_cast<Node*>(const_cast<BaseNode*>(it.get_ptr())), 1);
  }
  void push_front(const T& value) { insert(begin(), value); }
  void push_back(const T& value) { insert(end(), value); }
  void pop_front() { erase(begin()); }
  void pop_back() { erase(--end()); }
};
