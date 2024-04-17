#ifndef _SMART_POINTERS_H_
#define _SMART_POINTERS_H_

#include <iostream>
#include <memory>
#include <type_traits>

template <typename T>
struct EnableSharedFromThis;

struct BaseControlBlock;

template <typename T, typename Allocator = std::allocator<T>>
struct ControlBlockMakeShared;

template <typename T, typename Deleter = std::default_delete<T>,
          typename Allocator = std::allocator<T>>
struct ControlBlockDefault;

template <typename T>
class SharedPtr;

template <typename T>
class WeakPtr;

struct BaseControlBlock {
  int shared_count;
  int weak_count;
  BaseControlBlock() : shared_count(1), weak_count(0){};
  void AddShared() { ++shared_count; }
  void AddWeak() { ++weak_count; }
  virtual ~BaseControlBlock() = default;
  virtual void DeleteObject() = 0;
  virtual void DeallocateAll() = 0;
  void DecreaseShared() {
    --shared_count;
    if (shared_count == 0) {
      DeleteObject();
      if (weak_count == 0) {
        DeallocateAll();
      }
    }
  }
  void DecreaseWeak() {
    --weak_count;
    if (shared_count == 0 && weak_count == 0) {
      DeallocateAll();
    }
  }
  virtual void* GetObject() = 0;
};

template <typename T, typename Allocator>
struct ControlBlockMakeShared : public BaseControlBlock {
  using Allocator_traits = std::allocator_traits<Allocator>;
  using Block_allocator = typename std::allocator_traits<
      Allocator>::template rebind_alloc<ControlBlockMakeShared<T, Allocator>>;
  using Block_traits = typename std::allocator_traits<
      Allocator>::template rebind_traits<ControlBlockMakeShared<T, Allocator>>;
  char object[sizeof(T)];
  Allocator alloc;
  template <typename... Args>
  ControlBlockMakeShared(Allocator alloc, Args&&... args) : alloc(alloc) {
    Allocator_traits::construct(alloc, reinterpret_cast<T*>(object),
                                std::forward<Args>(args)...);
  }
  virtual void DeleteObject() override {
    Allocator_traits::destroy(alloc, reinterpret_cast<T*>(object));
  }
  virtual void DeallocateAll() override {
    Block_allocator block_alloc = alloc;
    Block_traits::deallocate(block_alloc, this, 1);
  }
  virtual void* GetObject() override { return object; }
};

template <typename T, typename Deleter, typename Allocator>
struct ControlBlockDefault : public BaseControlBlock {
  using Allocator_traits = std::allocator_traits<Allocator>;
  using Block_allocator =
      typename std::allocator_traits<Allocator>::template rebind_alloc<
          ControlBlockDefault<T, Deleter, Allocator>>;
  using Block_traits =
      typename std::allocator_traits<Allocator>::template rebind_traits<
          ControlBlockDefault<T, Deleter, Allocator>>;
  Deleter del;
  Allocator alloc;
  T* object;
  ControlBlockDefault(T* ptr) : object(ptr) {}
  ControlBlockDefault(Deleter del, Allocator alloc, T* object)
      : del(del), alloc(alloc), object(object) {}
  virtual void DeleteObject() override { del(object); }
  virtual void DeallocateAll() override {
    Block_allocator block_alloc = alloc;
    Block_traits::deallocate(block_alloc, this, 1);
  }
  virtual void* GetObject() override { return object; }
};

template <typename T>
class SharedPtr {
 private:
  BaseControlBlock* block_ = nullptr;

  template <typename>
  friend class SharedPtr;
  template <typename>
  friend class WeakPtr;
  template <typename>
  friend struct EnableSharedFromThis;

  SharedPtr(const WeakPtr<T>& other) : block_(other.block_) {
    if (block_ != nullptr) {
      block_->AddShared();
    }
    check_shared_from_this();
  }

 public:
  ~SharedPtr() {
    if (block_ != nullptr) {
      block_->DecreaseShared();
    }
  }
  SharedPtr(const SharedPtr& other) : block_(other.block_) {
    if (block_ != nullptr) {
      block_->AddShared();
    }
    check_shared_from_this();
  }
  SharedPtr<T>& operator=(const SharedPtr<T>& other) {
    SharedPtr<T> copy = other;
    swap(copy);
    return *this;
  }
  SharedPtr(SharedPtr&& other) {
    block_ = other.block_;
    other.block_ = nullptr;
    check_shared_from_this();
  }
  SharedPtr<T>& operator=(SharedPtr<T>&& other) {
    SharedPtr<T> copy = std::move(other);
    swap(copy);
    return *this;
  }

  template <typename X>
  SharedPtr(const SharedPtr<X>& other) : block_(other.block_) {
    if (block_ != nullptr) {
      block_->AddShared();
    }
    check_shared_from_this();
  }
  template <typename X>
  SharedPtr<T>& operator=(const SharedPtr<X>& other) {
    SharedPtr<T> copy = other;
    swap(copy);
    return *this;
  }
  template <typename X>
  SharedPtr(SharedPtr<X>&& other) {
    block_ = other.block_;
    other.block_ = nullptr;
    check_shared_from_this();
  }
  template <typename X>
  SharedPtr<T>& operator=(SharedPtr<X>&& other) {
    SharedPtr<T> copy = std::move(other);
    swap(copy);
    return *this;
  }

  int use_count() const { return block_->shared_count; }
  void reset() {
    if (block_ != nullptr) {
      block_->DecreaseShared();
    }
    block_ = nullptr;
  }
  template <typename X>
  void reset(X* ptr) {
    if (block_ != nullptr) {
      block_->DecreaseShared();
    }
    block_ = new ControlBlockDefault<X>(ptr);
  }

  T* GetObject() const {
    return block_ == nullptr ? nullptr
                             : reinterpret_cast<T*>(block_->GetObject());
  }
  T* get() const { return GetObject(); }
  T& operator*() const { return *GetObject(); }
  T* operator->() const { return GetObject(); }

  void swap(SharedPtr& other) { std::swap(block_, other.block_); }

  SharedPtr() : block_(nullptr) { check_shared_from_this(); };
  SharedPtr(T* ptr) : block_(new ControlBlockDefault<T>(ptr)) {
    check_shared_from_this();
  }
  SharedPtr(BaseControlBlock* block) : block_(block) {
    check_shared_from_this();
  }
  template <typename X, typename Deleter, typename Allocator>
  SharedPtr(X* ptr, Deleter del, Allocator alloc) {
    using Block_allocator =
        typename std::allocator_traits<Allocator>::template rebind_alloc<
            ControlBlockDefault<T, Deleter, Allocator>>;
    using Block_traits =
        typename std::allocator_traits<Allocator>::template rebind_traits<
            ControlBlockDefault<T, Deleter, Allocator>>;
    Block_allocator block_alloc = alloc;
    block_ = Block_traits::allocate(block_alloc, 1);
    // Block_traits::construct(
    //     block_alloc,
    //     reinterpret_cast<ControlBlockDefault<T, Deleter,
    //     Allocator>*>(block_), del, alloc, reinterpret_cast<T*>(ptr));
    new (reinterpret_cast<ControlBlockDefault<T, Deleter, Allocator>*>(block_))
        ControlBlockDefault<T, Deleter, Allocator>(del, alloc,
                                                   reinterpret_cast<T*>(ptr));
    check_shared_from_this();
  }
  template <typename X, typename Deleter>
  SharedPtr(X* ptr, Deleter del) : SharedPtr(ptr, del, std::allocator<T>()) {}
  void check_shared_from_this() {
    if constexpr (std::is_base_of_v<EnableSharedFromThis<T>, T>) {
      GetObject()->weak_ptr = *this;
    }
  }
};

template <typename T, typename Allocator, typename... Args>
SharedPtr<T> allocateShared(const Allocator& alloc, Args&&... args) {
  using Block_allocator = typename std::allocator_traits<
      Allocator>::template rebind_alloc<ControlBlockMakeShared<T, Allocator>>;
  using Block_traits = typename std::allocator_traits<
      Allocator>::template rebind_traits<ControlBlockMakeShared<T, Allocator>>;
  Block_allocator block_alloc = alloc;
  ControlBlockMakeShared<T, Allocator>* block =
      Block_traits::allocate(block_alloc, 1);
  // Block_traits::construct(block_alloc, block, alloc,
  //                         std::forward<Args>(args)...);
  new (block)
      ControlBlockMakeShared<T, Allocator>(alloc, std::forward<Args>(args)...);
  SharedPtr<T> ptr = block;
  return ptr;
}

template <typename T, typename... Args>
SharedPtr<T> makeShared(Args&&... args) {
  SharedPtr<T> ptr =
      allocateShared<T>(std::allocator<T>(), std::forward<Args>(args)...);
  return ptr;
}

template <typename T>
class WeakPtr {
 private:
  BaseControlBlock* block_;

  template <typename>
  friend class SharedPtr;
  template <typename>
  friend class WeakPtr;
  template <typename>
  friend struct EnableSharedFromThis;

 public:
  ~WeakPtr() {
    if (block_ != nullptr) {
      block_->DecreaseWeak();
    }
  }
  template <typename X>
  WeakPtr(const SharedPtr<X>& other) : block_(other.block_) {
    if (block_ != nullptr) {
      block_->AddWeak();
    }
  }

  WeakPtr() : block_(nullptr) {}

  WeakPtr(const WeakPtr& other) : block_(other.block_) {
    if (block_ != nullptr) {
      block_->AddWeak();
    }
  }
  WeakPtr<T>& operator=(const WeakPtr<T>& other) {
    WeakPtr<T> copy = other;
    swap(copy);
    return *this;
  }
  WeakPtr(WeakPtr&& other) {
    block_ = other.block_;
    other.block_ = nullptr;
  }
  WeakPtr<T>& operator=(WeakPtr<T>&& other) {
    WeakPtr<T> copy = std::move(other);
    swap(copy);
    return *this;
  }

  template <typename X>
  WeakPtr(const WeakPtr<X>& other) : block_(other.block_) {
    if (block_ != nullptr) {
      block_->AddWeak();
    }
  }
  template <typename X>
  WeakPtr<T>& operator=(const WeakPtr<X>& other) {
    WeakPtr<T> copy = other;
    swap(copy);
    return *this;
  }
  template <typename X>
  WeakPtr(WeakPtr<X>&& other) {
    block_ = other.block_;
    other.block = nullptr;
  }
  template <typename X>
  WeakPtr<T>& operator=(WeakPtr<X>&& other) {
    WeakPtr<T> copy = std::move(other);
    swap(copy);
    return *this;
  }

  T* GetObject() const {
    return block_ == nullptr ? nullptr
                             : reinterpret_cast<T*>(block_->GetObject());
  }
  T* get() const { return GetObject(); }
  T& operator*() const { return *GetObject(); }
  T* operator->() const { return GetObject(); }

  int use_count() const { return block_->shared_count; }
  bool expired() const {
    return block_ == nullptr ? true : block_->shared_count == 0;
  }
  SharedPtr<T> lock() const {
    return expired() ? SharedPtr<T>() : SharedPtr<T>(*this);
  }
  void swap(WeakPtr& other) { std::swap(block_, other.block_); }
};

template <typename T>
struct EnableSharedFromThis {
 private:
  WeakPtr<T> weak_ptr;

  template <typename>
  friend class SharedPtr;
  template <typename>
  friend class WeakPtr;

 public:
  SharedPtr<T> shared_from_this() {
    return weak_ptr.block_ == nullptr ? throw std::bad_weak_ptr()
                                      : weak_ptr.lock();
  }
  WeakPtr<T> week_from_this() { return weak_ptr; }
};

#endif