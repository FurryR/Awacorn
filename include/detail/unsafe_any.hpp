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
  unsafe_any() : _ptr(nullptr) {}
  template <typename T>
  unsafe_any(T&& v)
      : _ptr(new _derived<typename std::decay<T>::type>(std::forward<T>(v))) {}
  unsafe_any(const unsafe_any& v) : _ptr(v._ptr ? v._ptr->clone() : nullptr) {}
  unsafe_any(unsafe_any&& v) : _ptr(std::move(v._ptr)) {}
  unsafe_any& operator=(const unsafe_any& v) {
    _ptr = v._ptr ? v._ptr->clone() : nullptr;
    return *this;
  }
  unsafe_any& operator=(unsafe_any&& v) {
    _ptr = std::move(v._ptr);
    return *this;
  }
  template <typename T>
  friend T unsafe_cast(const unsafe_any& v) noexcept;

 private:
  struct _base {
    virtual void* get() noexcept = 0;
    virtual std::unique_ptr<_base> clone() const = 0;
    virtual ~_base() {}
  };
  template <typename T>
  struct _derived : _base {
    void* get() noexcept override { return &data; }
    std::unique_ptr<_base> clone() const override {
      return std::unique_ptr<_base>(new _derived(data));
    }
    _derived(T&& v) : data(std::move(v)){};
    _derived(const T& v) : data(v){};

   private:
    T data;
  };
  std::unique_ptr<_base> _ptr;
};
template <typename T>
T unsafe_cast(const unsafe_any& v) noexcept {
  return *((T*)v._ptr->get());
}
};  // namespace detail
};  // namespace awacorn
#endif
#endif