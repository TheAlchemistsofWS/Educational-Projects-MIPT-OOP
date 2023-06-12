#include <iostream>

struct BaseControlBlock {
  size_t countShared = 0;
  size_t countWeak = 0;

  BaseControlBlock(size_t count_shared, size_t count_weak)
    : countShared(count_shared), countWeak(count_weak) {}

  virtual void useDeleter() = 0;
  virtual void destroy() = 0;
  virtual ~BaseControlBlock() = default;
};

template <typename T, typename Alloc, typename Deleter>
struct ControlBlockRegular : BaseControlBlock {
  T* ptr;
  Alloc alloc;
  Deleter del;

  ControlBlockRegular(T* p, Deleter d, Alloc al)
    : BaseControlBlock(1, 0), ptr(p), alloc(al), del(d) {}

  void useDeleter() override {
    del(ptr);
  }

  void destroy() override {
    using AllocType =
      typename std::allocator_traits<Alloc>::template rebind_alloc<
        ControlBlockRegular<T, Alloc, Deleter>>;
    AllocType all = alloc;
    std::allocator_traits<AllocType>::deallocate(all, this, 1);
  }
};

template <typename T, typename Alloc>
struct ControlBlockMakeShared : BaseControlBlock {
  Alloc alloc;
  T ptr;

  template <typename... Args>
  ControlBlockMakeShared(Alloc al, Args&&... args)
    : BaseControlBlock(1, 0), alloc(al), ptr(std::forward<Args>(args)...) {}

  void useDeleter() override {
    using AllocType = typename std::allocator_traits<
      Alloc>::template rebind_alloc<ControlBlockMakeShared<T, Alloc>>;
    AllocType all = alloc;
    std::allocator_traits<AllocType>::destroy(all, &ptr);
  }

  void destroy() override {
    using AllocType = typename std::allocator_traits<
      Alloc>::template rebind_alloc<ControlBlockMakeShared<T, Alloc>>;
    AllocType all = alloc;
    std::allocator_traits<AllocType>::deallocate(all, this, 1);
  }
};

template <typename T, typename U>
using isBaseOrSame =
  std::enable_if_t<std::is_same_v<T, U> || std::is_base_of_v<T, U>>;

template <typename T>
class SharedPtr {
  public:
  SharedPtr() : data(nullptr), ptr(nullptr) {}

  template <typename U, typename Deleter, typename Alloc,
    typename = isBaseOrSame<T, U>>
  SharedPtr(U* p, Deleter d, Alloc alloc)
    : data(typename std::allocator_traits<Alloc>::template rebind_alloc<
    ControlBlockRegular<U, Alloc, Deleter>>(alloc)
             .allocate(1)),
      ptr(p) {
    new (data) ControlBlockRegular<U, Alloc, Deleter>(ptr, d, alloc);
  }

  template <typename U, typename = isBaseOrSame<T, U>>
  SharedPtr(U* p)
    : SharedPtr(p, std::default_delete<U>(), std::allocator<U>()) {}

  SharedPtr(const SharedPtr& other) : data(other.data), ptr(other.ptr) {
    if (data != nullptr) {
      ++data->countShared;
    }
  }

  template <typename U, typename = isBaseOrSame<T, U>>
  SharedPtr(const SharedPtr<U>& other) : data(other.data), ptr(other.ptr) {
    if (data != nullptr) {
      ++data->countShared;
    }
  }

  template <typename U, typename Deleter, typename = isBaseOrSame<T, U>>
  SharedPtr(U* p, Deleter d) : SharedPtr(p, d, std::allocator<U>()) {}

  template <typename U, typename = isBaseOrSame<T, U>>
  SharedPtr(SharedPtr<U>&& other) : data(other.data), ptr(other.ptr) {
    other.ptr = nullptr;
    other.data = nullptr;
  }

  SharedPtr(SharedPtr&& other) : data(other.data), ptr(other.ptr) {
    other.ptr = nullptr;
    other.data = nullptr;
  }

  template <typename U>
  SharedPtr<T>& operator=(U&& other) {
    this->template swap(SharedPtr<T>(std::forward<U>(other)));
    return *this;
  }

  ~SharedPtr() {
    if (data == nullptr) {
      return;
    }
    --data->countShared;
    if (data->countShared > 0) {
      return;
    }
    data->useDeleter();
    if (data->countWeak == 0) {
      data->destroy();
    }
  }

  size_t use_count() const {
    return (data != nullptr ? data->countShared : 0);
  }

  template <typename U, typename = isBaseOrSame<T, U>>
  void reset(U* other) {
    (other == nullptr) ? reset() : this->template swap(SharedPtr<T>(other));
  }

  void reset() {
    this->template swap(SharedPtr<T>());
  }

  template <typename U>
  void swap(U&& other) {
    std::swap(data, other.data);
    std::swap(ptr, other.ptr);
  }

  T* get() const {
    return ptr;
  }

  T& operator*() const {
    return *ptr;
  }

  T* operator->() const {
    return ptr;
  }

  private:
  template <typename U>
  friend class SharedPtr;

  template <typename U>
  friend class WeakPtr;

  BaseControlBlock* data;
  T* ptr;

  template <typename Alloc, typename... Args>
  SharedPtr(Alloc alloc, Args&&... args) {
    using AllocType = typename std::allocator_traits<
      Alloc>::template rebind_alloc<ControlBlockMakeShared<T, Alloc>>;
    AllocType allocMakeShared = alloc;

    auto* pAllocMakeShared = allocMakeShared.allocate(1);
    std::allocator_traits<AllocType>::construct(
      allocMakeShared, pAllocMakeShared, alloc,
      std::forward<Args>(args)...);
    data = pAllocMakeShared;
    ptr = nullptr;
  }

  SharedPtr(T* p, BaseControlBlock* counter) : data(counter), ptr(p) {
    ++counter->countShared;
  }

  template <typename U, typename... Args>
  friend SharedPtr<U> makeShared(Args&&... args);

  template <typename U, typename Alloc, typename... Args>
  friend SharedPtr<U> allocateShared(const Alloc&, Args&&...);
};

template <typename T>
class WeakPtr {
  public:
  WeakPtr() : data(nullptr), ptr(nullptr) {}

  WeakPtr(const WeakPtr& other) : data(other.data), ptr(other.ptr) {
    if (data != nullptr) {
      ++data->countWeak;
    }
  }

  template <typename U, typename = isBaseOrSame<T, U>>
  WeakPtr(const WeakPtr<U>& other) : data(other.data), ptr(other.ptr) {
    if (data != nullptr) {
      ++data->countWeak;
    }
  }

  template <typename U, typename = isBaseOrSame<T, U>>
  WeakPtr(WeakPtr<U>&& other) : data(other.data), ptr(other.ptr) {
    other.ptr = nullptr;
    other.data = nullptr;
  }

  WeakPtr(WeakPtr&& other) : data(other.data), ptr(other.ptr) {
    other.ptr = nullptr;
    other.data = nullptr;
  }

  template <typename U, typename = isBaseOrSame<T, U>>
  WeakPtr(const SharedPtr<U>& other) : data(other.data), ptr(other.ptr) {
    if (data != nullptr) {
      ++data->countWeak;
    }
  }

  ~WeakPtr() {
    if (data == nullptr) {
      return;
    }
    --data->countWeak;
    if (data->countWeak == 0 && data->countShared == 0) {
      data->destroy();
    }
  }

  template <typename U, typename = isBaseOrSame<T, U>>
  WeakPtr<T>& operator=(const SharedPtr<U>& other) {
    this->swap(WeakPtr<T>(other));
    return *this;
  }

  template <typename Other>
  WeakPtr<T>& operator=(Other&& other) {
    this->swap(WeakPtr<T>(std::forward<Other>(other)));
    return *this;
  }

  size_t use_count() const {
    return (data != nullptr ? data->countShared : 0);
  }

  template <class U>
  void swap(U&& other) {
    std::swap(data, other.data);
    std::swap(ptr, other.ptr);
  }

  T* get() const {
    return ptr;
  }

  T& operator*() const {
    return *ptr;
  }

  T* operator->() const {
    return ptr;
  }

  SharedPtr<T> lock() const {
    return expired() ? SharedPtr<T>() : SharedPtr<T>(ptr, data);
  }

  bool expired() const {
    return data->countShared == 0;
  }

  private:
  template <typename U>
  friend class WeakPtr;

  template <typename U>
  friend class SharedPtr;

  BaseControlBlock* data;
  T* ptr;
};

template <typename T, typename... Args>
SharedPtr<T> makeShared(Args&&... args) {
  return SharedPtr<T>(std::allocator<T>(), std::forward<Args>(args)...);
}

template <typename T, typename Alloc, typename... Args>
SharedPtr<T> allocateShared(const Alloc& alloc, Args&&... args) {
  return SharedPtr<T>(alloc, std::forward<Args>(args)...);
}
