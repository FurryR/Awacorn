#ifndef _AWACORN_ANY_
#define _AWACORN_ANY_
#if __cplusplus >= 201101L
/**
 * Project Awacorn 基于 MIT 协议开源。
 * Copyright(c) 凌 2022.
 */
#include <memory>
namespace awacorn {
namespace detail {
struct unsafe_any {
  unsafe_any()
      : _ptr(nullptr, [](void*) {}),
        _clone([](void*) -> void* { return nullptr; }) {}
  template <typename T>
  unsafe_any(T&& v)
      : _ptr(new typename std::decay<T>::type(std::forward<T>(v)),
             [](void* ptr) { delete (typename std::decay<T>::type*)ptr; }),
        _clone([](void* ptr) {
          return (void*)new typename std::decay<T>::type(
              *((typename std::decay<T>::type*)ptr));
        }) {}
  unsafe_any(const unsafe_any& v)
      : _ptr(v._clone(v._ptr.get()), v._ptr.get_deleter()), _clone(v._clone) {}
  unsafe_any(unsafe_any&& v) : _ptr(std::move(v._ptr)), _clone(v._clone) {
    v._clone = [](void*) -> void* { return nullptr; };
  }
  unsafe_any& operator=(const unsafe_any& v) {
    _ptr = std::unique_ptr<void, void (*)(void*)>(v._clone(v._ptr.get()),
                                                  v._ptr.get_deleter());
    _clone = v._clone;
    return *this;
  }
  unsafe_any& operator=(unsafe_any&& v) {
    _ptr = std::move(v._ptr);
    _clone = v._clone;
    v._clone = [](void*) -> void* { return nullptr; };
    return *this;
  }
  template <typename T>
  friend T unsafe_cast(const unsafe_any& v) noexcept;

 private:
  std::unique_ptr<void, void (*)(void*)> _ptr;
  void* (*_clone)(void*);
};
template <typename T>
T unsafe_cast(const unsafe_any& v) noexcept {
  return *static_cast<T*>(v._ptr.get());
}
};  // namespace detail
};  // namespace awacorn
#endif
#endif