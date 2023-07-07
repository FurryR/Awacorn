#ifndef _AWACORN_EXPERIMENTAL_ASYNC
#define _AWACORN_EXPERIMENTAL_ASYNC
#include <type_traits>
#if __cplusplus >= 201101L
/**
 * Project Awacorn 基于 MIT 协议开源。
 * Copyright(c) 凌 2023.
 */
#include <memory>

#include "../detail/capture.hpp"
#include "../detail/function.hpp"
#include "../promise.hpp"
namespace awacorn {
namespace detail {
template <typename T>
struct _value {
  promise<T> get() const noexcept { return pm; }
  promise<void> apply() {
    if (fn) {
      pm = fn();
      fn = function<promise<T>()>();
    }
    return pm.then([](const T&) {});
  }
  template <typename U,
            typename = typename std::enable_if<!std::is_same<
                typename std::decay<U>::type, _value<T>>::value>::type>
  _value(U&& _fn) : fn(std::forward<U>(_fn)){};
  _value(const _value&) = delete;
  _value& operator=(const _value&) = delete;

 private:
  function<promise<T>()> fn;
  promise<T> pm;
};
template <>
struct _value<void> {
  promise<void> get() const noexcept { return pm; }
  promise<void> apply() {
    if (fn) {
      pm = fn();
      fn = function<promise<void>()>();
    }
    return pm.then([]() {});
  }
  template <typename U,
            typename = typename std::enable_if<!std::is_same<
                typename std::decay<U>::type, _value<void>>::value>::type>
  _value(U&& _fn) : fn(std::forward<U>(_fn)){};
  _value(const _value&) = delete;
  _value& operator=(const _value&) = delete;

 private:
  function<promise<void>()> fn;
  promise<void> pm;
};
};  // namespace detail
template <typename T>
struct context;
namespace stmt {
template <typename Ctx>
struct expr {
  template <typename>
  friend struct awacorn::context;
  promise<void> apply(context<Ctx>& ctx) {
    if (fn) {
      return fn(ctx);
    } else
      return resolve();
  }
  template <typename U,
            typename = typename std::enable_if<!std::is_same<
                typename std::decay<U>::type, expr<Ctx>>::value>::type>
  expr(U&& _fn) : fn(std::forward<U>(_fn)){};
  expr(const expr&) = delete;
  expr(expr&& v) : fn(std::move(v.fn)) {}
  detail::function<promise<void>(context<Ctx>&)> fn;
};
template <typename T>
struct value {
  template <
      typename U,
      typename = typename std::enable_if<
          (!std::is_same<typename std::decay<U>::type, value<T>>::value) &&
          (!std::is_constructible<typename std::decay<T>::type, U>::value)>::
          type>
  value(U&& fn)
      : ptr(std::make_shared<detail::_value<T>>(std::forward<U>(fn))){};
  value(const value<T>& v) : ptr(v.ptr){};
  value(value<T>&& v) : ptr(std::move(v.ptr)){};
  template <typename... Args,
            typename = typename std::enable_if<!std::is_same<
                typename std::decay<Args...>::type, value<T>>::value>::type>
  value(Args&&... args) {
    detail::capture_helper<T> _v =
        detail::capture(T(std::forward<Args>(args)...));
    ptr = std::make_shared<detail::_value<T>>(
        [_v]() mutable { return resolve<T>(std::move(_v.borrow())); });
  };

  template <typename U>
  auto get(U&& fn) const -> value<decltype(fn(std::declval<T>()))> {
    using Ret = decltype(fn(std::declval<T>()));
    detail::capture_helper<U> _fn = detail::capture(std::forward<U>(fn));
    std::shared_ptr<detail::_value<T>> _ptr = ptr;
    return value<Ret>([_ptr, _fn]() mutable {
      return _ptr->apply()
          .then([_ptr]() { return _ptr->get(); })
          .then([_fn](const T& val) { return _fn.borrow()(val); });
    });
  }

  template <typename Ctx>
  promise<void> apply(context<Ctx>&) const {
    return apply();
  }
  promise<void> apply() const { return ptr->apply(); }
  promise<T> get() const noexcept { return ptr->get(); }

  std::shared_ptr<detail::_value<T>> ptr;
};
template <>
struct value<void> {
  template <typename U,
            typename = typename std::enable_if<!std::is_same<
                typename std::decay<U>::type, value<void>>::value>::type>
  value(U&& fn)
      : ptr(std::make_shared<detail::_value<void>>(std::forward<U>(fn))){};
  value(const value<void>& v) : ptr(v.ptr){};
  value(value<void>&& v) : ptr(std::move(v.ptr)){};
  value() {
    ptr = std::make_shared<detail::_value<void>>(
        []() mutable { return resolve(); });
  }

  template <typename U>
  auto get(U&& fn) const -> value<decltype(fn())> {
    using Ret = decltype(fn());
    detail::capture_helper<U> _fn = detail::capture(std::forward<U>(fn));
    std::shared_ptr<detail::_value<void>> _ptr = ptr;
    return value<Ret>([_ptr, _fn]() mutable {
      return _ptr->apply().then([_ptr]() { return _ptr->get(); }).then([_fn]() {
        return _fn.borrow()();
      });
    });
  }

  template <typename Ctx>
  promise<void> apply(context<Ctx>&) const {
    return apply();
  }
  promise<void> apply() const { return ptr->apply(); }
  promise<void> get() const noexcept { return ptr->get(); }

  std::shared_ptr<detail::_value<void>> ptr;
};
};  // namespace stmt
template <typename T>
struct context;
namespace detail {
template <typename T>
struct basic_context : public std::enable_shared_from_this<context<T>> {
  template <typename U>
  static stmt::value<U> await(const stmt::value<promise<U>>& v) {
    return stmt::value<U>([v]() {
      return v.apply().then([v]() {
        return v.get().then([](const promise<U>& v) { return v; });
      });
    });
  }
  static stmt::expr<T> error(const stmt::value<std::exception_ptr>& v) {
    return stmt::expr<T>([v](context<T>& ctx) {
      return v.apply()
          .then([v]() { return v.get(); })
          .then([&ctx](const std::exception_ptr& v) { ctx.reject(v); });
    });
  }
};
};  // namespace detail
template <typename T>
struct context : public detail::basic_context<T> {
  template <typename U>
  context& operator<<(U&& v) {
    detail::capture_helper<U> _v = detail::capture(std::forward<U>(v));
    std::shared_ptr<context<T>> ptr = this->shared_from_this();
    chain = chain.then([ptr, _v]() mutable { return _v.borrow().apply(*ptr); });
    return *this;
  }
  static stmt::expr<T> ret(const stmt::value<T>& v) {
    return stmt::expr<T>([v](context<T>& ctx) {
      return v.apply().then([v]() { return v.get(); }).then([&ctx](const T& v) {
        ctx.resolve(v);
      });
    });
  }

 private:
  template <typename U>
  friend promise<T> async(U&&);
  static std::shared_ptr<context<T>> create() {
    return std::shared_ptr<context<T>>(new context<T>());
  }
  promise<T> get_result() const { return result; }
  void resolve(const T& v) const { result.resolve(v); }
  void resolve(T&& v) const { result.resolve(std::move(v)); }
  void reject(const std::exception_ptr& v) const { result.reject(v); }
  void reject(std::exception_ptr&& v) const { result.reject(std::move(v)); }
  context() { chain.resolve(); }
  promise<T> result;
  promise<void> chain;
};
template <>
struct context<void> : public detail::basic_context<void> {
  template <typename U>
  context& operator<<(U&& v) {
    detail::capture_helper<U> _v = detail::capture(std::forward<U>(v));
    std::shared_ptr<context<void>> ptr = this->shared_from_this();
    chain = chain.then([ptr, _v]() mutable { return _v.borrow().apply(*ptr); });
    return *this;
  }
  static stmt::expr<void> ret(const stmt::value<void>& v) {
    return stmt::expr<void>([v](context<void>& ctx) {
      return v.apply().then([v]() { return v.get(); }).then([&ctx]() {
        ctx.resolve();
      });
    });
  }

 private:
  template <typename T, typename U>
  friend promise<T> async(U&&);
  static std::shared_ptr<context<void>> create() {
    return std::shared_ptr<context<void>>(new context<void>());
  }
  promise<void> get_result() const { return result; }
  void resolve() const { result.resolve(); }
  void reject(const std::exception_ptr& v) const { result.reject(v); }
  void reject(std::exception_ptr&& v) const { result.reject(std::move(v)); }
  context() { chain.resolve(); };
  promise<void> result;
  promise<void> chain;
};
/**
 * @brief 开始异步函数。
 *
 * @tparam T 函数的返回类型。
 * @tparam U 实际逻辑函数的类型。
 * @param fn 实际逻辑函数。
 * @return promise<T> 返回的 promise。
 */
template <typename T, typename U>
promise<T> async(U&& fn) {
  std::shared_ptr<context<T>> ctx = context<T>::create();
  fn(*ctx);
  return ctx->get_result();
}
}  // namespace awacorn
#endif
#endif