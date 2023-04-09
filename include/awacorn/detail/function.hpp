#ifndef _AWACORN_FUNCTION_
#define _AWACORN_FUNCTION_
#include <functional>
#include <memory>
namespace awacorn {
namespace detail {
template <typename>
class function {};
template <typename Ret, typename... Args>
class function<Ret(Args...)> {
  struct _m_base {
    virtual Ret operator()(Args...) const = 0;
    virtual const std::type_info& target_type() const noexcept = 0;
    virtual void* target() noexcept = 0;
    virtual const void* target() const noexcept = 0;
    virtual ~_m_base() = default;
  };
  template <typename T>
  class _m_derived : public _m_base {
    using value_type = typename std::decay<T>::type;
    value_type fn;

   public:
    _m_derived(const value_type& fn) : fn(fn) {}
    _m_derived(value_type&& fn) : fn(std::move(fn)) {}
    const std::type_info& target_type() const noexcept override {
      return typeid(value_type);
    }
    void* target() noexcept override {
      if (typeid(T) != typeid(value_type)) return nullptr;
      return &fn;
    }
    const void* target() const noexcept override {
      if (typeid(T) != typeid(value_type)) return nullptr;
      return &fn;
    }
    Ret operator()(Args... args) const override {
      return fn(std::forward<Args>(args)...);
    }
  };
  std::unique_ptr<_m_base> ptr;

 public:
  function() = default;
  template <typename U>
  function(U&& fn)
      : ptr(std::unique_ptr<_m_base>(new _m_derived<U>(std::forward<U>(fn)))) {}
  function(const function&) = delete;
  function(function&& fn) : ptr(std::move(fn.ptr)) {}
  function& operator=(function&& fn) {
    return (ptr = std::move(fn.ptr)), *this;
  }
  void swap(function& fn) noexcept { std::swap(ptr, fn.ptr); }
  operator bool() const noexcept { return !!ptr; }
  const std::type_info& target_type() const noexcept {
    if (*this) return ptr->target_type();
    return typeid(void);
  }
  template <typename T>
  T* target() noexcept {
    if (*this) return (T*)ptr->template target<T>();
    return nullptr;
  }
  template <typename T>
  const T* target() const noexcept {
    if (*this) return (T*)ptr->template target<T>();
    return nullptr;
  }
  inline Ret operator()(Args... args) {
    if (*this) return (*ptr)(std::forward<Args>(args)...);
    throw std::bad_function_call();
  }
};
};  // namespace detail
};  // namespace awacorn
#endif
