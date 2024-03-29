#ifndef _AWACORN_FUNCTION_
#define _AWACORN_FUNCTION_
#if __cplusplus >= 201101L
/**
 * Project Awacorn 基于 MIT 协议开源。
 * Copyright(c) 凌 2022.
 */
#include <functional>
#include <memory>
namespace awacorn {
namespace detail {
template <typename>
class function;
template <typename Ret, typename... Args>
class function<Ret(Args...)> {
  struct _m_base {
    virtual Ret operator()(Args...) = 0;
    virtual ~_m_base() = default;
  };
  template <typename T>
  struct _m_derived : _m_base {
    using value_type = typename std::decay<T>::type;
    value_type fn;

    _m_derived(const value_type& fn) : fn(fn) {}
    _m_derived(value_type&& fn) : fn(std::move(fn)) {}
    Ret operator()(Args... args) override {
      return fn(std::forward<Args>(args)...);
    }
  };
  std::unique_ptr<_m_base> ptr;

 public:
  function() = default;
  function(std::nullptr_t) {}
  /**
   * @brief 由可调用对象构造函数对象。
   *
   * @tparam U 可调用对象的类型
   * @param fn 可调用对象的实例。
   */
  template <typename U>
  function(U&& fn)
      : ptr(std::unique_ptr<_m_base>(new _m_derived<U>(std::forward<U>(fn)))) {
    static_assert(
        std::is_same<decltype(std::declval<U>()(std::declval<Args>()...)),
                     Ret>::value,
        "T is not the same as Ret(Args...)");
  }
  function(const function&) = delete;
  function& operator=(const function&) = delete;
  function(function&& fn) : ptr(std::move(fn.ptr)) {}
  function& operator=(function&& fn) {
    return (ptr = std::move(fn.ptr)), *this;
  }
  /**
   * @brief 将此函数对象跟 fn 交换。
   *
   * @param fn 目标函数对象。
   */
  inline void swap(function& fn) noexcept { std::swap(ptr, fn.ptr); }
  /**
   * @brief 判断函数是否已被初始化。
   *
   * @return true 函数已被初始化
   * @return false 函数未被初始化
   */
  inline operator bool() const noexcept { return !!ptr; }
  /**
   * @brief 调用函数。
   *
   * @param args 参数列表。
   * @return Ret 函数的返回值。
   */
  inline Ret operator()(Args&&... args) const {
    if (*this) return (*ptr)(std::forward<Args>(args)...);
    throw std::bad_function_call();
  }
};
};  // namespace detail
};  // namespace awacorn
#endif
#endif