#ifndef _AWACORN_PROMISE
#define _AWACORN_PROMISE
#if __cplusplus >= 201101L
/**
 * Project awacorn 基于 MIT 协议开源。
 * Copyright(c) 凌 2023.
 */
#include <array>
#include <exception>
#include <memory>
#include <tuple>

#include "detail/capture.hpp"
#include "detail/function.hpp"
#include "detail/variant.hpp"
namespace awacorn {
/**
 * @brief promise 的状态。
 */
enum state_t {
  Pending = 0,    // 等待中。
  Fulfilled = 1,  // 已完成。
  Rejected = 2    // 已失败。
};
namespace detail {
struct basic_promise {
 protected:
  // Promise<T>.then(detail::function<Promise<Ret>(ArgType)>)
  // sub-implementation pm.then(detail::function<Promise<Ret>(ArgType)>)
  // sub-implementation
  template <typename Ret, typename ArgType,
            template <typename T> class PromiseT, typename _promise>
  struct __then_sub_impl {
    template <typename U>
    static PromiseT<Ret> apply(const std::shared_ptr<_promise>& pm, U&& fn) {
      PromiseT<Ret> t;
      detail::capture_helper<U> arg_fn = detail::capture(std::forward<U>(fn));
      pm->then([t, arg_fn](const ArgType& val) -> void {
        try {
          PromiseT<Ret> tmp = arg_fn.borrow()(val);
          tmp.then([t](const Ret& val) -> void { t.resolve(val); });
          tmp.error(
              [t](const std::exception_ptr& err) -> void { t.reject(err); });
        } catch (...) {
          t.reject(std::current_exception());
        }
      });
      pm->error([t](const std::exception_ptr& err) -> void { t.reject(err); });
      return t;
    }
  };
  // pm.then(detail::function<Promise<void>(ArgType)>) sub-implementation
  template <typename ArgType, template <typename T> class PromiseT,
            typename _promise>
  struct __then_sub_impl<void, ArgType, PromiseT, _promise> {
    template <typename U>
    static PromiseT<void> apply(const std::shared_ptr<_promise>& pm,
                                const U&& fn) {
      PromiseT<void> t;
      detail::capture_helper<U> arg_fn = detail::capture(std::forward<U>(fn));
      pm->then([t, arg_fn](const ArgType& val) -> void {
        try {
          PromiseT<void> tmp = arg_fn.borrow()(val);
          tmp.then([t]() -> void { t.resolve(); });
          tmp.error(
              [t](const std::exception_ptr& err) -> void { t.reject(err); });
        } catch (...) {
          t.reject(std::current_exception());
        }
      });
      pm->error([t](const std::exception_ptr& err) -> void { t.reject(err); });
      return t;
    }
  };
  // Promise<void>.then(detail::function<Promise<Ret>()>) sub-implementation
  // pm.then(detail::function<Promise<Ret>()>) sub-implementation
  template <typename Ret, template <typename T> class PromiseT,
            typename _promise>
  struct __then_sub_impl<Ret, void, PromiseT, _promise> {
    template <typename U>
    static PromiseT<Ret> apply(const std::shared_ptr<_promise>& pm, U&& fn) {
      PromiseT<Ret> t;
      detail::capture_helper<U> arg_fn = detail::capture(std::forward<U>(fn));
      pm->then([t, arg_fn]() mutable -> void {
        try {
          PromiseT<Ret> tmp = arg_fn.borrow()();
          tmp.then([t](const Ret& val) -> void { t.resolve(val); });
          tmp.error(
              [t](const std::exception_ptr& err) -> void { t.reject(err); });
        } catch (...) {
          t.reject(std::current_exception());
        }
      });
      pm->error([t](const std::exception_ptr& err) -> void { t.reject(err); });
      return t;
    }
  };
  // pm.then(detail::function<Promise<void>()>) sub-implementation
  template <template <typename T> class PromiseT, typename _promise>
  struct __then_sub_impl<void, void, PromiseT, _promise> {
    template <typename U>
    static PromiseT<void> apply(const std::shared_ptr<_promise>& pm, U&& fn) {
      PromiseT<void> t;
      detail::capture_helper<U> arg_fn = detail::capture(std::forward<U>(fn));
      pm->then([t, arg_fn]() mutable -> void {
        try {
          PromiseT<void> tmp = arg_fn.borrow()();
          tmp.then([t]() -> void { t.resolve(); });
          tmp.error(
              [t](const std::exception_ptr& err) -> void { t.reject(err); });
        } catch (...) {
          t.reject(std::current_exception());
        }
      });
      pm->error([t](const std::exception_ptr& err) -> void { t.reject(err); });
      return t;
    }
  };

  // Promise<T>.then implementation
  // pm.then(detail::function<Ret(ArgType)>) implementation
  template <typename Ret, typename ArgType,
            template <typename T> class PromiseT, typename _promise>
  struct __then_impl {
    template <typename U>
    static PromiseT<Ret> apply(const std::shared_ptr<_promise>& pm, U&& fn) {
      PromiseT<Ret> t;
      detail::capture_helper<U> arg_fn = detail::capture(std::forward<U>(fn));
      pm->then([t, arg_fn](const ArgType& val) mutable -> void {
        try {
          t.resolve(arg_fn.borrow()(val));
        } catch (...) {
          t.reject(std::current_exception());
        }
      });
      pm->error([t](const std::exception_ptr& err) -> void { t.reject(err); });
      return t;
    }
  };
  // pm.then(detail::function<Promise<Ret>(ArgType)>) implementation
  template <typename Ret, typename ArgType,
            template <typename T> class PromiseT, typename _promise>
  struct __then_impl<PromiseT<Ret>, ArgType, PromiseT, _promise> {
    template <typename U>
    static inline PromiseT<Ret> apply(const std::shared_ptr<_promise>& pm,
                                      U&& fn) {
      return __then_sub_impl<Ret, ArgType, PromiseT, _promise>::apply(
          pm, std::forward<U>(fn));
    }
  };
  // pm.then(detail::function<void(ArgType)>) implementation
  template <typename ArgType, template <typename T> class PromiseT,
            typename _promise>
  struct __then_impl<void, ArgType, PromiseT, _promise> {
    template <typename U>
    static PromiseT<void> apply(const std::shared_ptr<_promise>& pm, U&& fn) {
      PromiseT<void> t;
      detail::capture_helper<U> arg_fn = detail::capture(std::forward<U>(fn));
      pm->then([t, arg_fn](const ArgType& val) mutable -> void {
        try {
          arg_fn.borrow()(val);
          t.resolve();
        } catch (...) {
          t.reject(std::current_exception());
        }
      });
      pm->error([t](const std::exception_ptr& err) -> void { t.reject(err); });
      return t;
    }
  };

  // Promise<void>.then implementation
  // pm.then(detail::function<Ret()>) implementation
  template <typename Ret, template <typename T> class PromiseT,
            typename _promise>
  struct __then_impl<Ret, void, PromiseT, _promise> {
    template <typename U>
    static PromiseT<Ret> apply(const std::shared_ptr<_promise>& pm, U&& fn) {
      PromiseT<Ret> t;
      detail::capture_helper<U> arg_fn = detail::capture(std::forward<U>(fn));
      pm->then([t, arg_fn]() mutable -> void {
        try {
          t.resolve(arg_fn.borrow()());
        } catch (...) {
          t.reject(std::current_exception());
        }
      });
      pm->error([t](const std::exception_ptr& err) -> void { t.reject(err); });
      return t;
    }
  };
  // pm.then(detail::function<Promise<Ret>()>) implementation
  template <typename Ret, template <typename T> class PromiseT,
            typename _promise>
  struct __then_impl<PromiseT<Ret>, void, PromiseT, _promise> {
    template <typename U>
    static inline PromiseT<Ret> apply(const std::shared_ptr<_promise>& pm,
                                      U&& fn) {
      return __then_sub_impl<Ret, void, PromiseT, _promise>::apply(
          pm, std::forward<U>(fn));
    }
  };
  // pm.then(detail::function<void()>) implementation
  template <template <typename T> class PromiseT, typename _promise>
  struct __then_impl<void, void, PromiseT, _promise> {
    template <typename U>
    static PromiseT<void> apply(const std::shared_ptr<_promise>& pm, U&& fn) {
      PromiseT<void> t;
      detail::capture_helper<U> arg_fn = detail::capture(std::forward<U>(fn));
      pm->then([t, arg_fn]() mutable -> void {
        try {
          arg_fn.borrow()();
          t.resolve();
        } catch (...) {
          t.reject(std::current_exception());
        }
      });
      pm->error([t](const std::exception_ptr& err) -> void { t.reject(err); });
      return t;
    }
  };

  // Promise<T>.error(detail::function<Promise<Ret>(std::exception_ptr)>)
  // sub-implementation

  // pm.error(detail::function<Promise<Ret>(std::exception_ptr)>)
  // sub-implementation
  template <typename Ret, template <typename T> class PromiseT,
            typename _promise>
  struct __error_sub_impl {
    template <typename U>
    static PromiseT<Ret> apply(const std::shared_ptr<_promise>& pm, U&& fn) {
      PromiseT<Ret> t;
      detail::capture_helper<U> arg_fn = detail::capture(std::forward<U>(fn));
      pm->error([t, arg_fn](const std::exception_ptr& val) mutable -> void {
        try {
          PromiseT<Ret> tmp = arg_fn.borrow()(val);
          tmp.then([t](const Ret& val) -> void { t.resolve(val); });
          tmp.error(
              [t](const std::exception_ptr& err) -> void { t.reject(err); });
        } catch (...) {
          t.reject(std::current_exception());
        }
      });
      return t;
    }
  };
  // pm.error(detail::function<Promise<void>(std::exception_ptr)>)
  // sub-implementation
  template <template <typename T> class PromiseT, typename _promise>
  struct __error_sub_impl<void, PromiseT, _promise> {
    template <typename U>
    static PromiseT<void> apply(const std::shared_ptr<_promise>& pm, U&& fn) {
      PromiseT<void> t;
      detail::capture_helper<U> arg_fn = detail::capture(std::forward<U>(fn));
      pm->error([t, arg_fn](const std::exception_ptr& val) mutable -> void {
        try {
          PromiseT<void> tmp = arg_fn.borrow()(val);
          tmp.then([t]() -> void { t.resolve(); });
          tmp.error(
              [t](const std::exception_ptr& err) -> void { t.reject(err); });
        } catch (...) {
          t.reject(std::current_exception());
        }
      });
      return t;
    }
  };

  // Promise<T>.error implementation
  // pm.error(detail::function<Ret(std::exception_ptr)>) implementation
  template <typename Ret, template <typename T> class PromiseT,
            typename _promise>
  struct __error_impl {
    template <typename U>
    static PromiseT<Ret> apply(const std::shared_ptr<_promise>& pm, U&& fn) {
      PromiseT<Ret> t;
      detail::capture_helper<U> arg_fn = detail::capture(std::forward<U>(fn));
      pm->error([t, arg_fn](const std::exception_ptr& err) mutable -> void {
        try {
          t.resolve(arg_fn.borrow()(err));
        } catch (...) {
          t.reject(std::current_exception());
        }
      });
      return t;
    }
  };
  // pm.error(detail::function<Promise<Ret>(std::exception_ptr)>) implementation
  template <typename Ret, template <typename T> class PromiseT,
            typename _promise>
  struct __error_impl<PromiseT<Ret>, PromiseT, _promise> {
    template <typename U>
    static inline PromiseT<Ret> apply(const std::shared_ptr<_promise>& pm,
                                      U&& fn) {
      return __error_sub_impl<Ret, PromiseT, _promise>::apply(
          pm, std::forward<U>(fn));
    }
  };
  // pm.error(detail::function<void(std::exception_ptr)>) implementation
  template <template <typename T> class PromiseT, typename _promise>
  struct __error_impl<void, PromiseT, _promise> {
    template <typename U>
    static PromiseT<void> apply(const std::shared_ptr<_promise>& pm, U&& fn) {
      PromiseT<void> t;
      detail::capture_helper<U> arg_fn = detail::capture(std::forward<U>(fn));
      pm->error([t, arg_fn](const std::exception_ptr& err) mutable -> void {
        try {
          arg_fn.borrow()(err);
        } catch (...) {
          t.reject(std::current_exception());
          return;
        }
        t.resolve();
      });
      return t;
    }
  };

  // Promise<T>.finally(detail::function<Promise<Ret>()>) sub-implementation
  // pm.finally(detail::function<Promise<Ret>()>) sub-implementation
  template <typename Ret, template <typename T> class PromiseT,
            typename _promise>
  struct __finally_sub_impl {
    template <typename U>
    static PromiseT<Ret> apply(const std::shared_ptr<_promise>& pm, U&& fn) {
      PromiseT<Ret> t;
      detail::capture_helper<U> arg_fn = detail::capture(std::forward<U>(fn));
      pm->finally([t, arg_fn]() mutable -> void {
        try {
          PromiseT<Ret> tmp = arg_fn.borrow()();
          tmp.then([t](const Ret& val) -> void { t.resolve(val); });
          tmp.error(
              [t](const std::exception_ptr& err) -> void { t.reject(err); });
        } catch (...) {
          t.reject(std::current_exception());
        }
      });
      return t;
    }
  };
  // pm.finally(detail::function<Promise<void>()>) sub-implementation
  template <template <typename T> class PromiseT, typename _promise>
  struct __finally_sub_impl<void, PromiseT, _promise> {
    template <typename U>
    static PromiseT<void> apply(const std::shared_ptr<_promise>& pm, U&& fn) {
      PromiseT<void> t;
      detail::capture_helper<U> arg_fn = detail::capture(std::forward<U>(fn));
      pm->finally([t, arg_fn]() mutable -> void {
        try {
          PromiseT<void> tmp = arg_fn.borrow()();
          tmp.then([t]() -> void { t.resolve(); });
          tmp.error(
              [t](const std::exception_ptr& err) -> void { t.reject(err); });
        } catch (...) {
          t.reject(std::current_exception());
        }
      });
      return t;
    }
  };

  // Promise<T>.finally implementation
  // pm.finally(detail::function<Ret()>) implementation
  template <typename Ret, template <typename T> class PromiseT,
            typename _promise>
  struct __finally_impl {
    template <typename U>
    static PromiseT<Ret> apply(const std::shared_ptr<_promise>& pm, U&& fn) {
      PromiseT<Ret> t;
      detail::capture_helper<U> arg_fn = detail::capture(std::forward<U>(fn));
      pm->finally([t, arg_fn]() mutable -> void {
        try {
          t.resolve(arg_fn.borrow()());
        } catch (...) {
          t.reject(std::current_exception());
        }
      });
      return t;
    }
  };
  // pm.finally(detail::function<Promise<Ret>()>) implementation
  template <typename Ret, template <typename T> class PromiseT,
            typename _promise>
  struct __finally_impl<PromiseT<Ret>, PromiseT, _promise> {
    template <typename U>
    static inline PromiseT<Ret> apply(const std::shared_ptr<_promise>& pm,
                                      U&& fn) {
      return __finally_sub_impl<Ret, PromiseT, _promise>::apply(
          pm, std::forward<U>(fn));
    }
  };
  // pm.finally(detail::function<void()>) implementation
  template <template <typename T> class PromiseT, typename _promise>
  struct __finally_impl<void, PromiseT, _promise> {
    template <typename U>
    static PromiseT<void> apply(const std::shared_ptr<_promise>& pm, U&& fn) {
      PromiseT<void> t;
      detail::capture_helper<U> arg_fn = detail::capture(std::forward<U>(fn));
      pm->finally([t, arg_fn]() mutable -> void {
        try {
          arg_fn.borrow()();
        } catch (...) {
          t.reject(std::current_exception());
          return;
        }
        t.resolve();
      });
      return t;
    }
  };
};
template <typename T, template <typename T2> class PromiseT>
struct remove_promise {
  using type = T;
};
template <typename T, template <typename T2> class PromiseT>
struct remove_promise<PromiseT<T>, PromiseT> {
  using type = T;
};
template <typename T, typename ReplaceT>
struct replace_void {
  using type = T;
};
template <typename ReplaceT>
struct replace_void<void, ReplaceT> {
  using type = ReplaceT;
};
};  // namespace detail

/**
 * @brief promise 对象。
 *
 * @tparam T Promise返回的值。
 */
template <typename T>
struct promise : detail::basic_promise {
  using value_type = typename std::decay<T>::type;

 private:
  struct _promise {
    using type = detail::function<void(const value_type&)>;
    using error_type = detail::function<void(const std::exception_ptr&)>;
    using finally_type = detail::function<void()>;
    void then(type&& fn) {
      if (_status == Fulfilled) {
        fn(get<value_type>(_val));
      } else if (_status == Pending) {
        if (_then) {
          detail::capture_helper<type> arg_then =
              detail::capture(std::move(_then));
          detail::capture_helper<type> arg_fn = detail::capture(std::move(fn));
          _then = [arg_then, arg_fn](const value_type& res) -> void {
            arg_then.borrow()(res);
            arg_fn.borrow()(res);
          };
        } else
          _then = std::move(fn);
      }
    }
    void error(error_type&& fn) {
      if (_status == Rejected) {
        fn(get<std::exception_ptr>(_val));
      } else if (_status == Pending) {
        if (_error) {
          detail::capture_helper<error_type> arg_error =
              detail::capture(std::move(_error));
          detail::capture_helper<error_type> arg_fn =
              detail::capture(std::move(fn));
          _error = [arg_error, arg_fn](const std::exception_ptr& err) -> void {
            arg_error.borrow()(err);
            arg_fn.borrow()(err);
          };
        } else
          _error = std::move(fn);
      }
    }
    void finally(finally_type&& fn) {
      if (_status != Pending) {
        fn();
      } else {
        if (_finally) {
          detail::capture_helper<finally_type> arg_finally =
              detail::capture(std::move(_finally));
          detail::capture_helper<finally_type> arg_fn =
              detail::capture(std::move(fn));
          _finally = [arg_finally, arg_fn]() -> void {
            arg_finally.borrow()();
            arg_fn.borrow()();
          };
        } else
          _finally = std::move(fn);
      }
    }
    void resolve(const value_type& val) {
      if (_status == Pending) {
        _status = Fulfilled;
        _val = val;
        if (_then) _then(get<value_type>(_val));
        if (_finally) _finally();
        reset();
      }
    }
    void resolve(value_type&& val) {
      if (_status == Pending) {
        _status = Fulfilled;
        _val = std::move(val);
        if (_then) _then(get<value_type>(_val));
        if (_finally) _finally();
        reset();
      }
    }
    void reject(const std::exception_ptr& err) {
      if (_status == Pending) {
        _status = Rejected;
        _val = err;
        if (_error) _error(get<std::exception_ptr>(_val));
        if (_finally) _finally();
        reset();
      }
    }
    constexpr state_t status() const noexcept { return _status; }
    _promise() : _status(Pending) {}
    _promise(const _promise& val) = delete;

   private:
    inline void reset() {
      _then = type();
      _error = error_type();
      _finally = finally_type();
    }
    state_t _status;
    type _then;
    error_type _error;
    finally_type _finally;
    variant<value_type, std::exception_ptr> _val;
  };
  std::shared_ptr<_promise> pm;

 public:
  /**
   * @brief 设定在 promise 完成后执行的函数。返回一个对这个函数的
   * Promise，当函数返回时，Promise 被 resolve。如果返回一个
   * Promise，则会等待这个 promise 完成以后再 resolve。
   *
   * @tparam U 函数类型。
   * @param fn 要执行的函数。
   * @return promise<typename detail::remove_promise<
      decltype(fn(std::declval<value_type>())), promise>::type 对函数的 promise
   */
  template <typename U>
  inline auto then(U&& fn) const -> promise<typename detail::remove_promise<
      decltype(fn(std::declval<value_type>())), promise>::type> {
    using Ret = decltype(fn(std::declval<value_type>()));
    return __then_impl<Ret, value_type, promise, _promise>::apply(
        pm, std::forward<U>(fn));
  }
  /**
   * @brief 设定在 promise 发生错误后执行的函数。返回一个对这个函数的
   * Promise，当函数返回时，Promise 被 resolve。如果返回一个
   * Promise，则会等待这个 promise 完成以后再 resolve。
   *
   * @tparam U 函数类型。
   * @param fn 要执行的函数。
   * @return promise<typename detail::remove_promise<
      decltype(fn(std::declval<std::exception_ptr>())),
      promise>::type> 对函数的 Promise。
   */
  template <typename U>
  inline auto error(U&& fn) const -> promise<typename detail::remove_promise<
      decltype(fn(std::declval<std::exception_ptr>())), promise>::type> {
    using Ret = decltype(fn(std::declval<std::exception_ptr>()));
    return __error_impl<Ret, promise, _promise>::apply(pm, std::forward<U>(fn));
  }
  /**
   * @brief 设定在 promise
   * 无论发生错误还是正确返回都会执行的函数。不关心 promise
   * 的返回值。返回一个对这个函数的 Promise，当函数返回时，Promise 被
   * resolve。如果返回一个 Promise，则会等待这个 promise 完成以后再 resolve。
   *
   * @tparam U 函数类型。
   * @param fn 要执行的函数。
   * @return promise<typename detail::remove_promise<decltype(fn()),
                                                 promise>::type> 对函数的
   Promise。
   */
  template <typename U>
  inline auto finally(U&& fn) const -> promise<
      typename detail::remove_promise<decltype(fn()), promise>::type> {
    using Ret = decltype(fn());
    return __finally_impl<Ret, promise, _promise>::apply(pm,
                                                         std::forward<U>(fn));
  }
  /**
   * @brief 完成此Promise。
   *
   * @param value 结果值。
   */
  inline void resolve(const value_type& value) const { pm->resolve(value); }
  inline void resolve(value_type&& value) const {
    pm->resolve(std::move(value));
  }
  /**
   * @brief 拒绝此Promise。
   *
   * @param err 异常。
   */
  inline void reject(const std::exception_ptr& value) const {
    pm->reject(value);
  }
  /**
   * @brief 获得Promise的状态。
   *
   * @return Status Promise的状态
   */
  inline state_t status() const noexcept { return pm->status(); }
  explicit promise() : pm(new _promise()) {}
};
/**
 * @brief 没有值的 promise 对象。
 */
template <>
class promise<void> : detail::basic_promise {
  struct _promise {
    using type = detail::function<void()>;
    using error_type = detail::function<void(const std::exception_ptr&)>;
    using finally_type = detail::function<void()>;
    void then(type&& fn) {
      if (_status == Fulfilled) {
        fn();
      } else if (_status == Pending) {
        if (_then) {
          detail::capture_helper<type> arg_then =
              detail::capture(std::move(_then));
          detail::capture_helper<type> arg_fn = detail::capture(std::move(fn));
          _then = [arg_then, arg_fn]() -> void {
            arg_then.borrow()();
            arg_fn.borrow()();
          };
        } else
          _then = std::move(fn);
      }
    }
    void error(error_type&& fn) {
      if (_status == Rejected) {
        fn(_val);
      } else if (_status == Pending) {
        if (_error) {
          detail::capture_helper<error_type> arg_error =
              detail::capture(std::move(_error));
          detail::capture_helper<error_type> arg_fn =
              detail::capture(std::move(fn));
          _error = [arg_error, arg_fn](const std::exception_ptr& err) -> void {
            arg_error.borrow()(err);
            arg_fn.borrow()(err);
          };
        } else
          _error = std::move(fn);
      }
    }
    void finally(finally_type&& fn) {
      if (_status != Pending) {
        fn();
      } else {
        if (_finally) {
          detail::capture_helper<finally_type> arg_finally =
              detail::capture(std::move(_finally));
          detail::capture_helper<finally_type> arg_fn =
              detail::capture(std::move(fn));
          _finally = [arg_finally, arg_fn]() -> void {
            arg_finally.borrow()();
            arg_fn.borrow()();
          };
        } else
          _finally = std::move(fn);
      }
    }
    void resolve() {
      if (_status == Pending) {
        _status = Fulfilled;
        if (_then) _then();
        if (_finally) _finally();
        reset();
      }
    }
    void reject(const std::exception_ptr& err) {
      if (_status == Pending) {
        _status = Rejected;
        _val = err;
        if (_error) _error(_val);
        if (_finally) _finally();
        reset();
      }
    }
    constexpr state_t status() const noexcept { return _status; }
    _promise(const _promise& val) = delete;
    _promise() : _status(Pending) {}

   private:
    void reset() {
      _then = type();
      _error = error_type();
      _finally = finally_type();
    }
    state_t _status;
    type _then;
    error_type _error;
    finally_type _finally;
    std::exception_ptr _val;
  };
  std::shared_ptr<_promise> pm;

 public:
  using value_type = void;
  /**
   * @brief 设定在 promise 完成后执行的函数。返回一个对这个函数的
   * Promise，当函数返回时，Promise 被 resolve。如果返回一个
   * Promise，则会等待这个 promise 完成以后再 resolve。
   *
   * @tparam U 函数类型。
   * @param fn 要执行的函数。
   * @return promise<typename detail::remove_promise<decltype(fn()),
                                                 promise>::type> 对函数的
   promise
   */
  template <typename U>
  inline auto then(U&& fn) const -> promise<
      typename detail::remove_promise<decltype(fn()), promise>::type> {
    using Ret = decltype(fn());
    return __then_impl<Ret, void, promise, _promise>::apply(
        pm, std::forward<U>(fn));
  }
  /**
   * @brief 设定在 promise 发生错误后执行的函数。返回一个对这个函数的
   * Promise，当函数返回时，Promise 被 resolve。如果返回一个
   * Promise，则会等待这个 promise 完成以后再 resolve。
   *
   * @tparam U 函数类型。
   * @param fn 要执行的函数。
   * @return promise<typename detail::remove_promise<
      decltype(fn(std::declval<std::exception_ptr>())),
      promise>::type> 对函数的 Promise。
   */
  template <typename U>
  inline auto error(U&& fn) const -> promise<typename detail::remove_promise<
      decltype(fn(std::declval<std::exception_ptr>())), promise>::type> {
    using Ret = decltype(fn(std::declval<std::exception_ptr>()));
    return __error_impl<Ret, promise, _promise>::apply(pm, std::forward<U>(fn));
  }
  /**
   * @brief 设定在 promise
   * 无论发生错误还是正确返回都会执行的函数。不关心 promise
   * 的返回值。返回一个对这个函数的 Promise，当函数返回时，Promise 被
   * resolve。如果返回一个 Promise，则会等待这个 promise 完成以后再 resolve。
   *
   * @tparam U 函数类型。
   * @param fn 要执行的函数。
   * @return promise<typename detail::remove_promise<decltype(fn()),
                                                 promise>::type> 对函数的
   Promise。
   */
  template <typename U>
  inline auto finally(U&& fn) const -> promise<
      typename detail::remove_promise<decltype(fn()), promise>::type> {
    using Ret = decltype(fn());
    return __finally_impl<Ret, promise, _promise>::apply(pm,
                                                         std::forward<U>(fn));
  }
  /**
   * @brief 完成此Promise。
   *
   * @param value 结果值。
   */
  inline void resolve() const { pm->resolve(); }
  /**
   * @brief 拒绝此Promise。
   *
   * @param err 异常。
   */
  inline void reject(const std::exception_ptr& value) const {
    pm->reject(value);
  }
  /**
   * @brief 获得Promise的状态。
   *
   * @return Status Promise的状态
   */
  inline state_t status() const noexcept { return pm->status(); }
  explicit promise() : pm(new _promise()) {}
};
namespace detail {
template <typename ResultType, size_t N>
struct promise_all {
  template <typename T, typename... Args>
  static inline void apply(const promise<ResultType>& pm,
                           const std::shared_ptr<ResultType>& result,
                           const std::shared_ptr<size_t>& done_count,
                           const promise<T>& current, Args&&... args) {
    promise_all<ResultType, N - 1>::apply(pm, result, done_count,
                                          std::forward<Args>(args)...);
    current.then([result, done_count, pm](const T& value) -> void {
      if (result) {
        std::get<N>(*result) = value;
        if ((++(*done_count)) == std::tuple_size<ResultType>::value) {
          pm.resolve(std::move(*result));
        }
      }
    });
    current.error([pm](const std::exception_ptr& v) -> void { pm.reject(v); });
  }
  template <typename... Args>
  static inline void apply(const promise<ResultType>& pm,
                           const std::shared_ptr<ResultType>& result,
                           const std::shared_ptr<size_t>& done_count,
                           const promise<void>& current, Args&&... args) {
    promise_all<ResultType, N - 1>::apply(pm, result, done_count,
                                          std::forward<Args>(args)...);
    current.then([result, done_count, pm]() -> void {
      if (result) {
        std::get<N>(*result) = monostate();
        if ((++(*done_count)) == std::tuple_size<ResultType>::value) {
          pm.resolve(std::move(*result));
        }
      }
    });
    current.error([pm](const std::exception_ptr& v) -> void { pm.reject(v); });
  }
};
template <typename ResultType>
struct promise_all<ResultType, 0> {
  template <typename T>
  static inline void apply(const promise<ResultType>& pm,
                           const std::shared_ptr<ResultType>& result,
                           const std::shared_ptr<size_t>& done_count,
                           const promise<T>& current) {
    current.then([result, done_count, pm](const T& value) -> void {
      if (result) {
        std::get<0>(*result) = value;
        if ((++(*done_count)) == std::tuple_size<ResultType>::value) {
          pm.resolve(std::move(*result));
        }
      }
    });
    current.error([pm](const std::exception_ptr& v) -> void { pm.reject(v); });
  }
  static inline void apply(const promise<ResultType>& pm,
                           const std::shared_ptr<ResultType>& result,
                           const std::shared_ptr<size_t>& done_count,
                           const promise<void>& current) {
    current.then([result, done_count, pm]() -> void {
      if (result) {
        std::get<0>(*result) = monostate();
        if ((++(*done_count)) == std::tuple_size<ResultType>::value) {
          pm.resolve(std::move(*result));
        }
      }
    });
    current.error([pm](const std::exception_ptr& v) -> void { pm.reject(v); });
  }
};
template <size_t TOTAL, size_t N>
struct promise_any {
  template <typename T, typename ResultType, typename... Args>
  static inline void apply(
      const promise<ResultType>& pm,
      const std::shared_ptr<std::array<std::exception_ptr, TOTAL>>& exce,
      const std::shared_ptr<size_t>& fail_count, const promise<T>& current,
      Args&&... args) {
    promise_any<TOTAL, N - 1>::apply(pm, exce, fail_count,
                                     std::forward<Args>(args)...);
    current.then([pm](const T& v) { pm.resolve(v); });
    current.error([pm, fail_count, exce](const std::exception_ptr& v) {
      (*exce)[N] = v;
      if ((++(*fail_count)) == TOTAL) {
        pm.reject(std::make_exception_ptr(std::move(*exce)));
      }
    });
  }
  template <typename ResultType, typename... Args>
  static inline void apply(
      const promise<ResultType>& pm,
      const std::shared_ptr<std::array<std::exception_ptr, TOTAL>>& exce,
      const std::shared_ptr<size_t>& fail_count, const promise<void>& current,
      Args&&... args) {
    promise_any<TOTAL, N - 1>::apply(pm, exce, fail_count,
                                     std::forward<Args>(args)...);
    current.then([pm]() { pm.resolve(monostate()); });
    current.error([pm, fail_count, exce](const std::exception_ptr& v) {
      (*exce)[N] = v;
      if ((++(*fail_count)) == TOTAL) {
        pm.reject(std::make_exception_ptr(std::move(*exce)));
      }
    });
  }
};
template <size_t TOTAL>
struct promise_any<TOTAL, 0> {
  template <typename ResultType, typename T>
  static void apply(
      const promise<ResultType>& pm,
      const std::shared_ptr<std::array<std::exception_ptr, TOTAL>>& exce,
      const std::shared_ptr<size_t>& fail_count, const promise<T>& current) {
    current.then([pm](const T& v) { pm.resolve(v); });
    current.error([pm, fail_count, exce](const std::exception_ptr& v) {
      (*exce)[0] = v;
      if ((++(*fail_count)) == TOTAL) {
        pm.reject(std::make_exception_ptr(std::move(*exce)));
      }
    });
  }
  template <typename ResultType>
  static void apply(
      const promise<ResultType>& pm,
      const std::shared_ptr<std::array<std::exception_ptr, TOTAL>>& exce,
      const std::shared_ptr<size_t>& fail_count, const promise<void>& current) {
    current.then([pm]() { pm.resolve(monostate()); });
    current.error([pm, fail_count, exce](const std::exception_ptr& v) {
      (*exce)[0] = v;
      if ((++(*fail_count)) == TOTAL) {
        pm.reject(std::make_exception_ptr(std::move(*exce)));
      }
    });
  }
};
struct promise_race {
  template <typename ResultType, typename T, typename... Args>
  static inline void apply(const promise<ResultType>& pm,
                           const promise<T>& current, Args&&... args) {
    promise_race::apply(pm, std::forward<Args>(args)...);
    current.then([pm](const T& v) { pm.resolve(v); });
    current.error([pm](const std::exception_ptr& v) { pm.reject(v); });
  }
  template <typename ResultType, typename... Args>
  static inline void apply(const promise<ResultType>& pm,
                           const promise<void>& current, Args&&... args) {
    promise_race::apply(pm, std::forward<Args>(args)...);
    current.then([pm]() { pm.resolve(monostate()); });
    current.error([pm](const std::exception_ptr& v) { pm.reject(v); });
  }
  template <typename ResultType, typename T>
  static void apply(const promise<ResultType>& pm, const promise<T>& current) {
    current.then([pm](const T& v) { pm.resolve(v); });
    current.error([pm](const std::exception_ptr& v) { pm.reject(v); });
  }
  template <typename ResultType>
  static void apply(const promise<ResultType>& pm,
                    const promise<void>& current) {
    current.then([pm]() { pm.resolve(monostate()); });
    current.error([pm](const std::exception_ptr& v) { pm.reject(v); });
  }
};
template <typename ResultType, size_t N>
struct promise_all_settled {
  template <typename T, typename... Args>
  static inline void apply(const promise<ResultType>& pm,
                           const std::shared_ptr<ResultType>& result,
                           const std::shared_ptr<size_t>& done_count,
                           const promise<T>& current, Args&&... args) {
    promise_all_settled<ResultType, N - 1>::apply(pm, result, done_count,
                                                  std::forward<Args>(args)...);
    current.finally([result, done_count, current, pm]() -> void {
      if (result) {
        std::get<N>(*result) = current;
        if ((++(*done_count)) == std::tuple_size<ResultType>::value) {
          pm.resolve(std::move(*result));
        }
      }
    });
  }
};
template <typename ResultType>
struct promise_all_settled<ResultType, 0> {
  template <typename T>
  static inline void apply(const promise<ResultType>& pm,
                           const std::shared_ptr<ResultType>& result,
                           const std::shared_ptr<size_t>& done_count,
                           const promise<T>& current) {
    current.finally([result, done_count, current, pm]() -> void {
      if (result) {
        std::get<0>(*result) = current;
        if ((++(*done_count)) == std::tuple_size<ResultType>::value) {
          pm.resolve(std::move(*result));
        }
      }
    });
  }
};
};  // namespace detail
namespace gather {
/**
 * @brief 等待所有 promise 完成后才 resolve。当有 promise 失败，立刻
 * reject。返回一个获得所有结果的 Promise。
 *
 * @tparam Args 返回类型
 * @param args Args 多个 Promise。跟返回类型有关。
 * @return promise<std::tuple<Args...>> 获得所有结果的 Promise。
 */
template <typename... Args>
static inline promise<
    std::tuple<typename detail::replace_void<Args, monostate>::type...>>
all(const promise<Args>&... args) {
  using ResultType =
      std::tuple<typename detail::replace_void<Args, monostate>::type...>;
  promise<ResultType> pm;
  std::shared_ptr<ResultType> result = std::make_shared<ResultType>();
  std::shared_ptr<size_t> done_count = std::make_shared<size_t>(0);
  detail::promise_all<ResultType, sizeof...(Args) - 1>::apply(
      pm, result, done_count, args...);
  return pm;
}
/**
 * @brief 在任意一个 promise 完成后 resolve。返回一个回调用 Promise。若全部
 * promise 都失败，则返回的 promise 也失败。
 *
 * @tparam Args 返回类型
 * @param arg 多个 Promise。跟返回类型有关。
 * @return promise<awacorn::variant<Args...>> 回调用 Promise。
 */
template <typename... Args>
static inline promise<
    awacorn::variant<typename detail::replace_void<Args, monostate>::type...>>
any(const promise<Args>&... args) {
  promise<
      awacorn::variant<typename detail::replace_void<Args, monostate>::type...>>
      pm;
  std::shared_ptr<size_t> fail_count = std::make_shared<size_t>(0);
  std::shared_ptr<std::array<std::exception_ptr, sizeof...(Args)>> exce =
      std::make_shared<std::array<std::exception_ptr, sizeof...(Args)>>();
  detail::promise_any<sizeof...(Args), sizeof...(Args) - 1>::apply(
      pm, exce, fail_count, args...);
  return pm;
}
/**
 * @brief 在任意一个 promise 完成或失败后 resolve。返回一个回调用
 * Promise。注意：因为无法确定哪个 promise 先返回，而 promise::race
 * 允许不同返回值的 Promise，故无法取得返回值。并且，Promise::race
 * 无论是否成功都会 resolve。
 *
 * @tparam Args 返回类型
 * @param arg 多个 Promise。跟返回类型有关。
 * @return promise<void> 回调用 Promise。
 */
template <typename... Args>
static inline promise<
    awacorn::variant<typename detail::replace_void<Args, monostate>::type...>>
race(const promise<Args>&... args) {
  promise<
      awacorn::variant<typename detail::replace_void<Args, monostate>::type...>>
      pm;
  detail::promise_race::apply(pm, args...);
  return pm;
}
/**
 * @brief 在所有 promise 都完成或失败后 resolve。返回一个回调用
 * Promise。注意：因为无法确定哪个 promise 先返回，而 promise::all_settled
 * 允许不同返回值的 Promise，故无法取得返回值。
 *
 * @tparam Args 返回类型
 * @param arg 多个 Promise。跟返回类型有关。
 * @return promise<void> 回调用 Promise。
 */
template <typename... Args>
static inline promise<std::tuple<promise<Args>...>> all_settled(
    const promise<Args>&... args) {
  using ResultType = std::tuple<promise<Args>...>;
  promise<ResultType> pm;
  std::shared_ptr<ResultType> result = std::make_shared<ResultType>();
  std::shared_ptr<size_t> done_count = std::make_shared<size_t>(0);
  detail::promise_all_settled<ResultType, sizeof...(Args) - 1>::apply(
      pm, result, done_count, args...);
  return pm;
}
}  // namespace gather
/**
 * @brief 返回一个已经 Fulfilled 的 Promise。
 *
 * @tparam Value promise 类型
 * @param val 可选，Promise的值。
 * @return promise<Value> 已经 Fulfilled 的 promise
 */
template <typename Value>
inline promise<Value> resolve(Value&& val) {
  promise<Value> tmp;
  tmp.resolve(std::forward<Value>(val));
  return tmp;
}
inline promise<void> resolve() {
  promise<void> tmp;
  tmp.resolve();
  return tmp;
}
/**
 * @brief 返回 promise 本身。
 *
 * @tparam Value promise 类型
 * @return promise<Value> promise 本身
 */
template <typename Value>
inline promise<Value> resolve(const promise<Value>& val) {
  return val;
}
template <typename Value>
inline promise<Value> resolve(promise<Value>&& val) {
  return val;
}
/**
 * @brief 返回一个 Rejected 的 Promise。
 *
 * @tparam Value promise 类型
 * @param err 错误内容。
 * @return promise<Value> 已经 Rejected 的 promise
 */
template <typename Value>
inline promise<Value> reject(const std::exception_ptr& err) {
  promise<Value> tmp;
  tmp.reject(err);
  return tmp;
}
};  // namespace awacorn
#endif
#endif
