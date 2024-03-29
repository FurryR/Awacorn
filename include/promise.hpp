#ifndef _AWACORN_PROMISE
#define _AWACORN_PROMISE
#if __cplusplus >= 201101L
/**
 * Project Awacorn 基于 MIT 协议开源。
 * Copyright(c) 凌 2023.
 */
#include <array>
#include <exception>
#include <memory>
#include <tuple>

#include "detail/capture.hpp"
#include "detail/function.hpp"
#include "variant.hpp"
namespace awacorn {
/**
 * @brief promise 的状态。
 */
enum status_t {
  pending = 0,    // 等待中。
  fulfilled = 1,  // 已完成。
  rejected = 2    // 已失败。
};
namespace detail {
struct basic_promise {
 protected:
  // Promise<T>.then(detail::function<Promise<Ret>(ArgType)>)
  // sub-implementation pm.then(detail::function<Promise<Ret>(ArgType)>)
  // sub-implementation
  template <typename Ret, typename ArgType,
            template <typename T> class PromiseT, typename _promise>
  struct _then_sub_impl {
    template <typename U>
    static PromiseT<Ret> apply(const std::shared_ptr<_promise>& pm, U&& fn) {
      PromiseT<Ret> t;
      auto arg_fn = detail::capture(std::forward<U>(fn));
      pm->then([t, arg_fn](ArgType&& val) mutable {
        try {
          auto tmp = arg_fn.borrow()(std::move(val));
          tmp.then([t](Ret&& val) { t.resolve(std::move(std::move(val))); });
          tmp.error(
              [t](std::exception_ptr&& err) { t.reject(std::move(err)); });
        } catch (...) {
          t.reject(std::current_exception());
        }
      });
      pm->error([t](std::exception_ptr&& err) { t.reject(std::move(err)); });
      return t;
    }
  };
  // pm.then(detail::function<Promise<void>(ArgType)>) sub-implementation
  template <typename ArgType, template <typename T> class PromiseT,
            typename _promise>
  struct _then_sub_impl<void, ArgType, PromiseT, _promise> {
    template <typename U>
    static PromiseT<void> apply(const std::shared_ptr<_promise>& pm, U&& fn) {
      PromiseT<void> t;
      auto arg_fn = detail::capture(std::forward<U>(fn));
      pm->then([t, arg_fn](ArgType&& val) mutable {
        try {
          auto tmp = arg_fn.borrow()(std::move(val));
          tmp.then([t]() { t.resolve(); });
          tmp.error(
              [t](std::exception_ptr&& err) { t.reject(std::move(err)); });
        } catch (...) {
          t.reject(std::current_exception());
        }
      });
      pm->error([t](std::exception_ptr&& err) { t.reject(std::move(err)); });
      return t;
    }
  };
  // Promise<void>.then(detail::function<Promise<Ret>()>) sub-implementation
  // pm.then(detail::function<Promise<Ret>()>) sub-implementation
  template <typename Ret, template <typename T> class PromiseT,
            typename _promise>
  struct _then_sub_impl<Ret, void, PromiseT, _promise> {
    template <typename U>
    static PromiseT<Ret> apply(const std::shared_ptr<_promise>& pm, U&& fn) {
      PromiseT<Ret> t;
      auto arg_fn = detail::capture(std::forward<U>(fn));
      pm->then([t, arg_fn]() mutable {
        try {
          auto tmp = arg_fn.borrow()();
          tmp.then([t](Ret&& val) { t.resolve(std::move(val)); });
          tmp.error(
              [t](std::exception_ptr&& err) { t.reject(std::move(err)); });
        } catch (...) {
          t.reject(std::current_exception());
        }
      });
      pm->error([t](std::exception_ptr&& err) { t.reject(std::move(err)); });
      return t;
    }
  };
  // pm.then(detail::function<Promise<void>()>) sub-implementation
  template <template <typename T> class PromiseT, typename _promise>
  struct _then_sub_impl<void, void, PromiseT, _promise> {
    template <typename U>
    static PromiseT<void> apply(const std::shared_ptr<_promise>& pm, U&& fn) {
      PromiseT<void> t;
      auto arg_fn = detail::capture(std::forward<U>(fn));
      pm->then([t, arg_fn]() mutable {
        try {
          auto tmp = arg_fn.borrow()();
          tmp.then([t]() { t.resolve(); });
          tmp.error(
              [t](std::exception_ptr&& err) { t.reject(std::move(err)); });
        } catch (...) {
          t.reject(std::current_exception());
        }
      });
      pm->error([t](std::exception_ptr&& err) { t.reject(std::move(err)); });
      return t;
    }
  };

  // Promise<T>.then implementation
  // pm.then(detail::function<Ret(ArgType)>) implementation
  template <typename Ret, typename ArgType,
            template <typename T> class PromiseT, typename _promise>
  struct _then_impl {
    template <typename U>
    static PromiseT<Ret> apply(const std::shared_ptr<_promise>& pm, U&& fn) {
      PromiseT<Ret> t;
      auto arg_fn = detail::capture(std::forward<U>(fn));
      pm->then([t, arg_fn](ArgType&& val) mutable {
        try {
          t.resolve(arg_fn.borrow()(std::move(val)));
        } catch (...) {
          t.reject(std::current_exception());
        }
      });
      pm->error([t](std::exception_ptr&& err) { t.reject(std::move(err)); });
      return t;
    }
  };
  // pm.then(detail::function<Promise<Ret>(ArgType)>) implementation
  template <typename Ret, typename ArgType,
            template <typename T> class PromiseT, typename _promise>
  struct _then_impl<PromiseT<Ret>, ArgType, PromiseT, _promise> {
    template <typename U>
    static inline PromiseT<Ret> apply(const std::shared_ptr<_promise>& pm,
                                      U&& fn) {
      return _then_sub_impl<Ret, ArgType, PromiseT, _promise>::apply(
          pm, std::forward<U>(fn));
    }
  };
  // pm.then(detail::function<void(ArgType)>) implementation
  template <typename ArgType, template <typename T> class PromiseT,
            typename _promise>
  struct _then_impl<void, ArgType, PromiseT, _promise> {
    template <typename U>
    static PromiseT<void> apply(const std::shared_ptr<_promise>& pm, U&& fn) {
      PromiseT<void> t;
      auto arg_fn = detail::capture(std::forward<U>(fn));
      pm->then([t, arg_fn](ArgType&& val) mutable {
        try {
          arg_fn.borrow()(std::move(val));
          t.resolve();
        } catch (...) {
          t.reject(std::current_exception());
        }
      });
      pm->error([t](std::exception_ptr&& err) { t.reject(std::move(err)); });
      return t;
    }
  };

  // Promise<void>.then implementation
  // pm.then(detail::function<Ret()>) implementation
  template <typename Ret, template <typename T> class PromiseT,
            typename _promise>
  struct _then_impl<Ret, void, PromiseT, _promise> {
    template <typename U>
    static PromiseT<Ret> apply(const std::shared_ptr<_promise>& pm, U&& fn) {
      PromiseT<Ret> t;
      auto arg_fn = detail::capture(std::forward<U>(fn));
      pm->then([t, arg_fn]() mutable {
        try {
          t.resolve(arg_fn.borrow()());
        } catch (...) {
          t.reject(std::current_exception());
        }
      });
      pm->error([t](std::exception_ptr&& err) { t.reject(std::move(err)); });
      return t;
    }
  };
  // pm.then(detail::function<Promise<Ret>()>) implementation
  template <typename Ret, template <typename T> class PromiseT,
            typename _promise>
  struct _then_impl<PromiseT<Ret>, void, PromiseT, _promise> {
    template <typename U>
    static inline PromiseT<Ret> apply(const std::shared_ptr<_promise>& pm,
                                      U&& fn) {
      return _then_sub_impl<Ret, void, PromiseT, _promise>::apply(
          pm, std::forward<U>(fn));
    }
  };
  // pm.then(detail::function<void()>) implementation
  template <template <typename T> class PromiseT, typename _promise>
  struct _then_impl<void, void, PromiseT, _promise> {
    template <typename U>
    static PromiseT<void> apply(const std::shared_ptr<_promise>& pm, U&& fn) {
      PromiseT<void> t;
      auto arg_fn = detail::capture(std::forward<U>(fn));
      pm->then([t, arg_fn]() mutable {
        try {
          arg_fn.borrow()();
          t.resolve();
        } catch (...) {
          t.reject(std::current_exception());
        }
      });
      // pm->error([t](std::exception_ptr&& err) { t.reject(std::move(err)); });
      return t;
    }
  };

  // Promise<T>.error(detail::function<Promise<Ret>(std::exception_ptr)>)
  // sub-implementation

  // pm.error(detail::function<Promise<Ret>(std::exception_ptr)>)
  // sub-implementation
  template <typename Ret, template <typename T> class PromiseT,
            typename _promise>
  struct _error_sub_impl {
    template <typename U>
    static PromiseT<Ret> apply(const std::shared_ptr<_promise>& pm, U&& fn) {
      PromiseT<Ret> t;
      auto arg_fn = detail::capture(std::forward<U>(fn));
      pm->error([t, arg_fn](std::exception_ptr&& val) mutable {
        try {
          auto tmp = arg_fn.borrow()(std::move(val));
          tmp.then([t](Ret&& val) { t.resolve(std::move(val)); });
          tmp.error(
              [t](std::exception_ptr&& err) { t.reject(std::move(err)); });
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
  struct _error_sub_impl<void, PromiseT, _promise> {
    template <typename U>
    static PromiseT<void> apply(const std::shared_ptr<_promise>& pm, U&& fn) {
      PromiseT<void> t;
      auto arg_fn = detail::capture(std::forward<U>(fn));
      pm->error([t, arg_fn](std::exception_ptr&& val) mutable {
        try {
          auto tmp = arg_fn.borrow()(std::move(val));
          tmp.then([t]() { t.resolve(); });
          tmp.error(
              [t](std::exception_ptr&& err) { t.reject(std::move(err)); });
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
  struct _error_impl {
    template <typename U>
    static PromiseT<Ret> apply(const std::shared_ptr<_promise>& pm, U&& fn) {
      PromiseT<Ret> t;
      auto arg_fn = detail::capture(std::forward<U>(fn));
      pm->error([t, arg_fn](std::exception_ptr&& err) mutable {
        try {
          t.resolve(arg_fn.borrow()(std::move(err)));
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
  struct _error_impl<PromiseT<Ret>, PromiseT, _promise> {
    template <typename U>
    static inline PromiseT<Ret> apply(const std::shared_ptr<_promise>& pm,
                                      U&& fn) {
      return _error_sub_impl<Ret, PromiseT, _promise>::apply(
          pm, std::forward<U>(fn));
    }
  };
  // pm.error(detail::function<void(std::exception_ptr)>) implementation
  template <template <typename T> class PromiseT, typename _promise>
  struct _error_impl<void, PromiseT, _promise> {
    template <typename U>
    static PromiseT<void> apply(const std::shared_ptr<_promise>& pm, U&& fn) {
      PromiseT<void> t;
      auto arg_fn = detail::capture(std::forward<U>(fn));
      pm->error([t, arg_fn](std::exception_ptr&& err) mutable {
        try {
          arg_fn.borrow()(std::move(err));
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
  struct _finally_sub_impl {
    template <typename U>
    static PromiseT<Ret> apply(const std::shared_ptr<_promise>& pm, U&& fn) {
      PromiseT<Ret> t;
      auto arg_fn = detail::capture(std::forward<U>(fn));
      pm->finally([t, arg_fn]() mutable {
        try {
          auto tmp = arg_fn.borrow()();
          tmp.then([t](Ret&& val) { t.resolve(std::move(val)); });
          tmp.error(
              [t](std::exception_ptr&& err) { t.reject(std::move(err)); });
        } catch (...) {
          t.reject(std::current_exception());
        }
      });
      return t;
    }
  };
  // pm.finally(detail::function<Promise<void>()>) sub-implementation
  template <template <typename T> class PromiseT, typename _promise>
  struct _finally_sub_impl<void, PromiseT, _promise> {
    template <typename U>
    static PromiseT<void> apply(const std::shared_ptr<_promise>& pm, U&& fn) {
      PromiseT<void> t;
      auto arg_fn = detail::capture(std::forward<U>(fn));
      pm->finally([t, arg_fn]() mutable {
        try {
          auto tmp = arg_fn.borrow()();
          tmp.then([t]() { t.resolve(); });
          tmp.error(
              [t](std::exception_ptr&& err) { t.reject(std::move(err)); });
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
  struct _finally_impl {
    template <typename U>
    static PromiseT<Ret> apply(const std::shared_ptr<_promise>& pm, U&& fn) {
      PromiseT<Ret> t;
      auto arg_fn = detail::capture(std::forward<U>(fn));
      pm->finally([t, arg_fn]() mutable {
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
  struct _finally_impl<PromiseT<Ret>, PromiseT, _promise> {
    template <typename U>
    static inline PromiseT<Ret> apply(const std::shared_ptr<_promise>& pm,
                                      U&& fn) {
      return _finally_sub_impl<Ret, PromiseT, _promise>::apply(
          pm, std::forward<U>(fn));
    }
  };
  // pm.finally(detail::function<void()>) implementation
  template <template <typename T> class PromiseT, typename _promise>
  struct _finally_impl<void, PromiseT, _promise> {
    template <typename U>
    static PromiseT<void> apply(const std::shared_ptr<_promise>& pm, U&& fn) {
      PromiseT<void> t;
      auto arg_fn = detail::capture(std::forward<U>(fn));
      pm->finally([t, arg_fn]() mutable {
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
struct extract_from {
  using type = T;
};
template <typename T, template <typename T2> class PromiseT>
struct extract_from<PromiseT<T>, PromiseT> {
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
  class _promise {
    status_t pm_status;
    variant<T, std::exception_ptr> val;
    detail::function<void(T&&)> then_cb;
    // NOTE: 虽然说 cppreference 的示例中对 exception_ptr 使用值传递，
    // 但引用计数的自增/自减是需要承担原子操作的开销的 (锁 bus)
    // 所以不增加引用计数的右值引用传递方式可能更好。
    detail::function<void(std::exception_ptr&&)> error_cb;
    detail::function<void()> finally_cb;

   public:
    _promise() = default;
    _promise(const _promise&) = delete;
    ~_promise() {
      if (pm_status == rejected && !val.valueless_by_exception()) {
        // Aborted due to unhandled rejection
        std::abort();
      }
    }
    void then(detail::function<void(T&&)>&& _then_cb) {
      if (pm_status == fulfilled && (!val.valueless_by_exception())) {
        _then_cb(std::move(get<0>(val)));
        val = variant<T, std::exception_ptr>();
      } else if (pm_status == pending && (!then_cb)) {
        then_cb = std::move(_then_cb);
      } else if (pm_status != rejected)
        throw std::logic_error(
            "Registered callback but the result has been already moved.");
    }
    void error(detail::function<void(std::exception_ptr&&)>&& _error_cb) {
      if (pm_status == rejected && (!val.valueless_by_exception())) {
        _error_cb(std::move(get<1>(val)));
        val = variant<T, std::exception_ptr>();
      } else if (pm_status == pending && (!error_cb)) {
        error_cb = std::move(_error_cb);
      } else if (pm_status != fulfilled)
        throw std::logic_error(
            "Registered callback but the result has been already moved.");
    }
    void finally(detail::function<void()>&& _finally_cb) {
      if (pm_status == pending) {
        if (!finally_cb)
          finally_cb = std::move(_finally_cb);
        else
          throw std::logic_error(
              "Registered callback but the result has been already moved.");
      } else {
        _finally_cb();
      }
    }
    void resolve(const T& value) {
      val = value;
      pm_status = fulfilled;
      if (then_cb) {
        then_cb(std::move(get<0>(val)));
        val = variant<T, std::exception_ptr>();
        then_cb = nullptr;
      }
      if (finally_cb) {
        finally_cb();
        finally_cb = nullptr;
      }
    }
    void resolve(T&& value) {
      val = std::move(value);
      pm_status = fulfilled;
      if (then_cb) {
        then_cb(std::move(get<0>(val)));
        val = variant<T, std::exception_ptr>();
        then_cb = nullptr;
        error_cb = nullptr;
      }
      if (finally_cb) {
        finally_cb();
        finally_cb = nullptr;
      }
    }
    void reject(const std::exception_ptr& value) {
      val = value;
      pm_status = rejected;
      if (error_cb) {
        error_cb(std::move(get<1>(val)));
        val = variant<T, std::exception_ptr>();
        then_cb = nullptr;
        error_cb = nullptr;
      }
      if (finally_cb) {
        finally_cb();
        finally_cb = nullptr;
      }
    }
    void reject(std::exception_ptr&& value) {
      val = value;
      pm_status = rejected;
      if (error_cb) {
        error_cb(std::move(get<1>(val)));
        val = variant<T, std::exception_ptr>();
        then_cb = nullptr;
        error_cb = nullptr;
      }
      if (finally_cb) {
        finally_cb();
        finally_cb = nullptr;
      }
    }
    inline constexpr status_t status() const noexcept { return pm_status; }
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
  inline auto then(U&& fn) const -> promise<typename detail::extract_from<
      decltype(fn(std::declval<value_type>())), promise>::type> {
    using Ret = decltype(fn(std::declval<value_type>()));
    return _then_impl<Ret, value_type, promise, _promise>::apply(
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
  inline auto error(U&& fn) const -> promise<typename detail::extract_from<
      decltype(fn(std::declval<std::exception_ptr>())), promise>::type> {
    using Ret = decltype(fn(std::declval<std::exception_ptr>()));
    return _error_impl<Ret, promise, _promise>::apply(pm, std::forward<U>(fn));
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
  inline auto finally(U&& fn) const
      -> promise<typename detail::extract_from<decltype(fn()), promise>::type> {
    using Ret = decltype(fn());
    return _finally_impl<Ret, promise, _promise>::apply(pm,
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
  inline void reject(std::exception_ptr&& value) const {
    pm->reject(std::move(value));
  }
  /**
   * @brief 获得Promise的状态。
   *
   * @return Status Promise的状态
   */
  inline status_t status() const noexcept { return pm->status(); }
  explicit promise() : pm(new _promise()) {}
  promise(const promise& v) : pm(v.pm) {}
  promise(promise&& v) : pm(std::move(v.pm)) {}
  promise& operator=(const promise& v) {
    pm = v.pm;
    return *this;
  }
  promise& operator=(promise&& v) noexcept {
    pm = std::move(v.pm);
    return *this;
  }
};
/**
 * @brief 没有值的 promise 对象。
 */
template <>
class promise<void> : detail::basic_promise {
  class _promise {
    status_t pm_status;
    std::exception_ptr val;
    detail::function<void()> then_cb;
    // NOTE: 虽然说 cppreference 的示例中对 exception_ptr 使用值传递，
    // 但引用计数的自增/自减是需要承担原子操作的开销的 (锁 bus)
    // 所以不增加引用计数的右值引用传递方式可能更好。
    detail::function<void(std::exception_ptr&&)> error_cb;
    detail::function<void()> finally_cb;

   public:
    _promise() = default;
    _promise(const _promise&) = delete;
    ~_promise() {
      if (pm_status == rejected && val) {
        // Aborted due to unhandled rejection
        std::abort();
      }
    }
    inline void then(detail::function<void()>&& _then_cb) {
      if (pm_status == fulfilled) {
        _then_cb();
      } else if (pm_status == pending && (!then_cb)) {
        then_cb = std::move(_then_cb);
      } else if (pm_status != rejected)
        throw std::logic_error(
            "Registered callback but the result has been already moved.");
    }
    inline void error(
        detail::function<void(std::exception_ptr&&)>&& _error_cb) {
      if (pm_status == rejected && val) {
        _error_cb(std::move(val));
        val = nullptr;
      } else if (pm_status == pending && (!error_cb)) {
        error_cb = std::move(_error_cb);
      } else if (pm_status != fulfilled)
        throw std::logic_error(
            "Registered callback but the result has been already moved.");
    }
    inline void finally(detail::function<void()>&& _finally_cb) {
      if (pm_status == pending) {
        if (!finally_cb)
          finally_cb = std::move(_finally_cb);
        else
          throw std::logic_error(
              "Registered callback but the result has been already moved.");
      } else {
        _finally_cb();
      }
    }
    void resolve() {
      pm_status = fulfilled;
      if (then_cb) {
        then_cb();
        then_cb = nullptr;
      }
      if (finally_cb) finally_cb();
    }
    void reject(const std::exception_ptr& value) {
      val = value;
      pm_status = rejected;
      if (error_cb) {
        error_cb(std::move(val));
        val = nullptr;
        then_cb = nullptr;
        error_cb = nullptr;
      }
      if (finally_cb) {
        finally_cb();
        finally_cb = nullptr;
      }
    }
    void reject(std::exception_ptr&& value) {
      val = value;
      pm_status = rejected;
      if (error_cb) {
        error_cb(std::move(val));
        val = nullptr;
        then_cb = nullptr;
        error_cb = nullptr;
      }
      if (finally_cb) {
        finally_cb();
        finally_cb = nullptr;
      }
    }
    inline constexpr status_t status() const noexcept { return pm_status; }
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
  inline auto then(U&& fn) const
      -> promise<typename detail::extract_from<decltype(fn()), promise>::type> {
    using Ret = decltype(fn());
    return _then_impl<Ret, void, promise, _promise>::apply(pm,
                                                           std::forward<U>(fn));
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
  inline auto error(U&& fn) const -> promise<typename detail::extract_from<
      decltype(fn(std::declval<std::exception_ptr>())), promise>::type> {
    using Ret = decltype(fn(std::declval<std::exception_ptr>()));
    return _error_impl<Ret, promise, _promise>::apply(pm, std::forward<U>(fn));
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
  inline auto finally(U&& fn) const
      -> promise<typename detail::extract_from<decltype(fn()), promise>::type> {
    using Ret = decltype(fn());
    return _finally_impl<Ret, promise, _promise>::apply(pm,
                                                        std::forward<U>(fn));
  }
  /**
   * @brief 完成此Promise。
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
  inline void reject(std::exception_ptr&& value) const {
    pm->reject(std::move(value));
  }
  /**
   * @brief 获得Promise的状态。
   *
   * @return Status Promise的状态
   */
  inline status_t status() const noexcept { return pm->status(); }
  explicit promise() : pm(new _promise()) {}
  promise(const promise& v) : pm(v.pm) {}
  promise(promise&& v) : pm(std::move(v.pm)) {}
  promise& operator=(const promise& v) {
    pm = v.pm;
    return *this;
  }
  promise& operator=(promise&& v) noexcept {
    pm = std::move(v.pm);
    return *this;
  }
};
namespace detail {
template <typename ResultType, std::size_t N>
struct promise_all {
  template <typename T, typename... Args>
  static inline void apply(const promise<ResultType>& pm,
                           const std::shared_ptr<ResultType>& result,
                           const std::shared_ptr<std::size_t>& done_count,
                           const promise<T>& current, Args&&... args) {
    promise_all<ResultType, N - 1>::apply(pm, result, done_count,
                                          std::forward<Args>(args)...);
    current
        .then([result, done_count, pm](T&& value) {
          if (result) {
            std::get<N>(*result) = std::move(value);
            if ((++(*done_count)) == std::tuple_size<ResultType>::value) {
              pm.resolve(std::move(*result));
            }
          }
        })
        .error([pm](std::exception_ptr&& v) { pm.reject(std::move(v)); });
  }
  template <typename... Args>
  static inline void apply(const promise<ResultType>& pm,
                           const std::shared_ptr<ResultType>& result,
                           const std::shared_ptr<std::size_t>& done_count,
                           const promise<void>& current, Args&&... args) {
    promise_all<ResultType, N - 1>::apply(pm, result, done_count,
                                          std::forward<Args>(args)...);
    current
        .then([result, done_count, pm]() {
          if (result) {
            std::get<N>(*result) = monostate();
            if ((++(*done_count)) == std::tuple_size<ResultType>::value) {
              pm.resolve(std::move(*result));
            }
          }
        })
        .error([pm](std::exception_ptr&& v) { pm.reject(std::move(v)); });
  }
  static inline void apply(const promise<ResultType>&,
                           const std::shared_ptr<ResultType>&,
                           const std::shared_ptr<std::size_t>&) {}
};
template <std::size_t TOTAL, std::size_t N>
struct promise_any {
  template <typename T, typename ResultType, typename... Args>
  static inline void apply(
      const promise<ResultType>& pm,
      const std::shared_ptr<std::array<std::exception_ptr, TOTAL>>& exce,
      const std::shared_ptr<std::size_t>& fail_count, const promise<T>& current,
      Args&&... args) {
    promise_any<TOTAL, N - 1>::apply(pm, exce, fail_count,
                                     std::forward<Args>(args)...);
    current.then([pm](T&& v) { pm.resolve(ResultType(std::move(v))); })
        .error([pm, fail_count, exce](std::exception_ptr&& v) {
          (*exce)[N] = std::move(v);
          if ((++(*fail_count)) == TOTAL) {
            pm.reject(std::make_exception_ptr(std::move(*exce)));
          }
        });
  }
  template <typename ResultType, typename... Args>
  static inline void apply(
      const promise<ResultType>& pm,
      const std::shared_ptr<std::array<std::exception_ptr, TOTAL>>& exce,
      const std::shared_ptr<std::size_t>& fail_count,
      const promise<void>& current, Args&&... args) {
    promise_any<TOTAL, N - 1>::apply(pm, exce, fail_count,
                                     std::forward<Args>(args)...);
    current.then([pm]() { pm.resolve(ResultType(monostate())); })
        .error([pm, fail_count, exce](std::exception_ptr&& v) {
          (*exce)[N] = std::move(v);
          if ((++(*fail_count)) == TOTAL) {
            pm.reject(std::make_exception_ptr(std::move(*exce)));
          }
        });
  }
  template <typename ResultType>
  static void apply(
      const promise<ResultType>&,
      const std::shared_ptr<std::array<std::exception_ptr, TOTAL>>&,
      const std::shared_ptr<std::size_t>&) {}
};
struct promise_race {
  template <typename ResultType, typename T, typename... Args>
  static inline void apply(const promise<ResultType>& pm,
                           const promise<T>& current, Args&&... args) {
    promise_race::apply(pm, std::forward<Args>(args)...);
    current.then([pm](T&& v) { pm.resolve(ResultType(std::move(v))); })
        .error([pm](std::exception_ptr&& v) { pm.reject(std::move(v)); });
  }
  template <typename ResultType, typename... Args>
  static inline void apply(const promise<ResultType>& pm,
                           const promise<void>& current, Args&&... args) {
    promise_race::apply(pm, std::forward<Args>(args)...);
    current.then([pm]() { pm.resolve(ResultType(monostate())); })
        .error([pm](std::exception_ptr&& v) { pm.reject(std::move(v)); });
  }
  template <typename ResultType>
  static void apply(const promise<ResultType>&) {}
};
template <typename ResultType, std::size_t N>
struct promise_all_settled {
  template <typename T, typename... Args>
  static inline void apply(const promise<ResultType>& pm,
                           const std::shared_ptr<ResultType>& result,
                           const std::shared_ptr<std::size_t>& done_count,
                           const promise<T>& current, Args&&... args) {
    promise_all_settled<ResultType, N - 1>::apply(pm, result, done_count,
                                                  std::forward<Args>(args)...);
    current.finally([result, done_count, current, pm]() mutable {
      if (result) {
        std::get<N>(*result) = std::move(current);
        if ((++(*done_count)) == std::tuple_size<ResultType>::value) {
          pm.resolve(std::move(*result));
        }
      }
    });
  }
  static inline void apply(const promise<ResultType>&,
                           const std::shared_ptr<ResultType>&,
                           const std::shared_ptr<std::size_t>&) {}
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
  auto result = std::make_shared<ResultType>();
  auto done_count = std::make_shared<std::size_t>(0);
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
 * @return promise<awacorn::unique_variant<
    typename detail::replace_void<Args, monostate>::type...>> 回调用 Promise。
 */
template <typename... Args>
static inline promise<awacorn::unique_variant<
    typename detail::replace_void<Args, monostate>::type...>>
any(const promise<Args>&... args) {
  promise<awacorn::unique_variant<
      typename detail::replace_void<Args, monostate>::type...>>
      pm;
  auto fail_count = std::make_shared<std::size_t>(0);
  auto exce =
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
 * @return promise<awacorn::unique_variant<
    typename detail::replace_void<Args, monostate>::type...>> 回调用 Promise。
 */
template <typename... Args>
static inline promise<awacorn::unique_variant<
    typename detail::replace_void<Args, monostate>::type...>>
race(const promise<Args>&... args) {
  promise<awacorn::unique_variant<
      typename detail::replace_void<Args, monostate>::type...>>
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
 * @return promise<std::tuple<promise<Args>...>> 回调用 Promise。
 */
template <typename... Args>
static inline promise<std::tuple<promise<Args>...>> all_settled(
    const promise<Args>&... args) {
  using ResultType = std::tuple<promise<Args>...>;
  promise<ResultType> pm;
  auto result = std::make_shared<ResultType>();
  auto done_count = std::make_shared<std::size_t>(0);
  detail::promise_all_settled<ResultType, sizeof...(Args) - 1>::apply(
      pm, result, done_count, args...);
  return pm;
}
}  // namespace gather
/**
 * @brief 返回一个已经 fulfilled 的 Promise。
 *
 * @tparam Value promise 类型
 * @param val 可选，Promise的值。
 * @return promise<Value> 已经 fulfilled 的 promise
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
 * @brief 返回一个 rejected 的 Promise。
 *
 * @tparam Value promise 类型
 * @param err 错误内容。
 * @return promise<Value> 已经 rejected 的 promise
 */
template <typename Value>
inline promise<Value> reject(const std::exception_ptr& err) {
  promise<Value> tmp;
  tmp.reject(err);
  return tmp;
}
template <typename Value>
inline promise<Value> reject(std::exception_ptr&& err) {
  promise<Value> tmp;
  tmp.reject(std::move(err));
  return tmp;
}
};  // namespace awacorn
#endif
#endif
