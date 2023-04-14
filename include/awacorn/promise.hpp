#ifndef _AWACORN_PROMISE
#define _AWACORN_PROMISE
#if __cplusplus >= 201703L
#include <any>
#endif
#if __cplusplus >= 201101L
/**
 * Project Awacorn 基于 MIT 协议开源。
 * Copyright(c) 凌 2023.
 */
#include <memory>
#include <tuple>

#include "detail/function.hpp"
namespace awacorn {
#if __cplusplus >= 201703L
using bad_any_cast = std::bad_any_cast;
class any {
  std::any data;

 public:
  any() = default;
  template <typename T>
  any(T&& val) : data(std::forward<T>(val)) {}
  any(const any& v) : data(v.data) {}
  any(any&& v) : data(std::move(v.data)) {}
  any& operator=(const any& v) {
    data = v.data;
    return *this;
  }
  any& operator=(any&& v) {
    data = std::move(v.data);
    return *this;
  }
  inline const std::type_info& type() const { return data.type(); }
  inline bool has_value() const noexcept { return data.has_value(); }
  inline void reset() noexcept { data.reset(); }
  std::any as_any() const { return data; }
  template <typename T, typename... Args>
  inline T& emplace(Args&&... args) {
    return data.emplace<T>(std::forward<Args>(args)...);
  }
  template <typename T>
  T cast() const {
    return std::any_cast<T>(data);
  }
  void swap(any& other) noexcept { std::swap(other.data, data); }
};
#else
struct bad_any_cast : public std::bad_cast {
  virtual const char* what() const noexcept { return "bad any_cast"; }
};
/**
 * @brief 类型安全的 C++ 11 any 实现。
 * @see https://en.cppreference.com/w/cpp/utility/any
 */
class any {
  struct _m_base {
    virtual ~_m_base() = default;
    virtual const std::type_info& type() const = 0;
    virtual std::unique_ptr<_m_base> clone() const = 0;
#if __cplusplus >= 201711L
    virtual std::any as_any() const = 0;
#endif
  };
  template <typename T>
  struct _m_derived : _m_base {
    T data;
    std::unique_ptr<_m_base> clone() const override {
      return std::unique_ptr<_m_base>(new _m_derived<T>(data));
    }
    const std::type_info& type() const override { return typeid(T); }
#if __cplusplus >= 201711L
    std::any as_any() const override { return std::any(data); }
#endif
    _m_derived(const T& data) : data(data) {}
  };
  std::unique_ptr<_m_base> ptr;

 public:
  any() = default;
  template <typename T>
  any(T&& val)
      : ptr(std::unique_ptr<_m_base>(
            new _m_derived<typename std::decay<T>::type>(
                std::forward<T>(val)))) {}
  any(const any& val) : ptr(val.ptr ? val.ptr->clone() : nullptr) {}
  any(any&& val) : ptr(std::move(val.ptr)) {}
  any& operator=(const any& rhs) {
    if (rhs.ptr)
      ptr = rhs.ptr->clone();
    else
      ptr = nullptr;
    return *this;
  }
  any& operator=(any&& rhs) { return (ptr = std::move(rhs.ptr)), *this; }
  const std::type_info& type() const {
    if (ptr) return ptr->type();
    return typeid(void);
  }
  inline bool has_value() const noexcept { return !!ptr; }
  inline void reset() noexcept { ptr.reset(); }
#if __cplusplus >= 201711L
  std::any as_any() const override {
    if (has_value()) return ptr->as_any();
    return std::any();
  }
#endif
  template <typename T, typename... Args>
  T& emplace(Args&&... args) {
    using Decay = typename std::decay<T>::type;
    reset();
    _m_derived<Decay> t = new _m_derived<Decay>(std::forward<Args>(args)...);
    ptr = std::unique_ptr<_m_base>(t);
    return t->data;
  }
  template <typename T>
  T cast() const {
    using Decay = typename std::decay<T>::type;
    any::_m_derived<Decay>* _ptr =
        dynamic_cast<any::_m_derived<Decay>*>(ptr.get());
    if (_ptr) return _ptr->data;
    throw bad_any_cast();
  }
  void swap(any& other) noexcept { std::swap(other.ptr, ptr); }
};
#endif
struct basic_promise {
  /**
   * @brief promise 的状态。
   */
  enum state_t {
    Pending = 0,    // 等待中。
    Fulfilled = 1,  // 已完成。
    Rejected = 2    // 已失败。
  };

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
      pm->then([t, fn](const ArgType& val) -> void {
        try {
          PromiseT<Ret> tmp = fn(val);
          tmp.then([t](const Ret& val) -> void { t.resolve(val); });
          tmp.error([t](const any& err) -> void { t.reject(err); });
        } catch (const any& err) {
          t.reject(err);
        }
      });
      pm->error([t](const any& err) -> void { t.reject(err); });
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
      pm->then([t, fn](const ArgType& val) -> void {
        try {
          PromiseT<void> tmp = fn(val);
          tmp.then([t]() -> void { t.resolve(); });
          tmp.error([t](const any& err) -> void { t.reject(err); });
        } catch (const any& err) {
          t.reject(err);
        }
      });
      pm->error([t](const any& err) -> void { t.reject(err); });
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
      pm->then([t, fn]() -> void {
        try {
          PromiseT<Ret> tmp = fn();
          tmp.then([t](const Ret& val) -> void { t.resolve(val); });
          tmp.error([t](const any& err) -> void { t.reject(err); });
        } catch (const any& err) {
          t.reject(err);
        }
      });
      pm->error([t](const any& err) -> void { t.reject(err); });
      return t;
    }
  };
  // pm.then(detail::function<Promise<void>()>) sub-implementation
  template <template <typename T> class PromiseT, typename _promise>
  struct __then_sub_impl<void, void, PromiseT, _promise> {
    template <typename U>
    static PromiseT<void> apply(const std::shared_ptr<_promise>& pm, U&& fn) {
      PromiseT<void> t;
      pm->then([t, fn]() -> void {
        try {
          PromiseT<void> tmp = fn();
          tmp.then([t]() -> void { t.resolve(); });
          tmp.error([t](const any& err) -> void { t.reject(err); });
        } catch (const any& err) {
          t.reject(err);
        }
      });
      pm->error([t](const any& err) -> void { t.reject(err); });
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
      pm->then([t, fn](const ArgType& val) -> void {
        try {
          t.resolve(fn(val));
        } catch (const any& err) {
          t.reject(err);
        }
      });
      pm->error([t](const any& err) -> void { t.reject(err); });
      return t;
    }
  };
  // pm.then(detail::function<Promise<Ret>(ArgType)>) implementation
  template <typename Ret, typename ArgType,
            template <typename T> class PromiseT, typename _promise>
  struct __then_impl<PromiseT<Ret>, ArgType, PromiseT, _promise> {
    template <typename U>
    static PromiseT<Ret> apply(const std::shared_ptr<_promise>& pm, U&& fn) {
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
      pm->then([t, fn](const ArgType& val) -> void {
        try {
          fn(val);
          t.resolve();
        } catch (const any& err) {
          t.reject(err);
        }
      });
      pm->error([t](const any& err) -> void { t.reject(err); });
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
      pm->then([t, fn]() -> void {
        try {
          t.resolve(fn());
        } catch (const any& err) {
          t.reject(err);
        }
      });
      pm->error([t](const any& err) -> void { t.reject(err); });
      return t;
    }
  };
  // pm.then(detail::function<Promise<Ret>()>) implementation
  template <typename Ret, template <typename T> class PromiseT,
            typename _promise>
  struct __then_impl<PromiseT<Ret>, void, PromiseT, _promise> {
    template <typename U>
    static PromiseT<Ret> apply(const std::shared_ptr<_promise>& pm, U&& fn) {
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
      pm->then([t, fn]() -> void {
        try {
          fn();
          t.resolve();
        } catch (const any& err) {
          t.reject(err);
        }
      });
      pm->error([t](const any& err) -> void { t.reject(err); });
      return t;
    }
  };

  // Promise<T>.error(detail::function<Promise<Ret>(any)>)
  // sub-implementation

  // pm.error(detail::function<Promise<Ret>(any)>) sub-implementation
  template <typename Ret, template <typename T> class PromiseT,
            typename _promise>
  struct __error_sub_impl {
    template <typename U>
    static PromiseT<Ret> apply(const std::shared_ptr<_promise>& pm, U&& fn) {
      PromiseT<Ret> t;
      pm->error([t, fn](const any& val) -> void {
        try {
          PromiseT<Ret> tmp = fn(val);
          tmp.then([t](const Ret& val) -> void { t.resolve(val); });
          tmp.error([t](const any& err) -> void { t.reject(err); });
        } catch (const any& err) {
          t.reject(err);
        }
      });
      return t;
    }
  };
  // pm.error(detail::function<Promise<void>(any)>) sub-implementation
  template <template <typename T> class PromiseT, typename _promise>
  struct __error_sub_impl<void, PromiseT, _promise> {
    template <typename U>
    static PromiseT<void> apply(const std::shared_ptr<_promise>& pm, U&& fn) {
      PromiseT<void> t;
      pm->error([t, fn](const any& val) -> void {
        try {
          PromiseT<void> tmp = fn(val);
          tmp.then([t]() -> void { t.resolve(); });
          tmp.error([t](const any& err) -> void { t.reject(err); });
        } catch (const any& err) {
          t.reject(err);
        }
      });
      return t;
    }
  };

  // Promise<T>.error implementation
  // pm.error(detail::function<Ret(any)>) implementation
  template <typename Ret, template <typename T> class PromiseT,
            typename _promise>
  struct __error_impl {
    template <typename U>
    static PromiseT<Ret> apply(const std::shared_ptr<_promise>& pm, U&& fn) {
      PromiseT<Ret> t;
      pm->error([t, fn](const any& err) -> void {
        try {
          t.resolve(fn(err));
        } catch (const any& err) {
          t.reject(err);
        }
      });
      return t;
    }
  };
  // pm.error(detail::function<Promise<Ret>(any)>) implementation
  template <typename Ret, template <typename T> class PromiseT,
            typename _promise>
  struct __error_impl<PromiseT<Ret>, PromiseT, _promise> {
    template <typename U>
    static PromiseT<Ret> apply(const std::shared_ptr<_promise>& pm, U&& fn) {
      return __error_sub_impl<Ret, PromiseT, _promise>::apply(
          pm, std::forward<U>(fn));
    }
  };
  // pm.error(detail::function<void(any)>) implementation
  template <template <typename T> class PromiseT, typename _promise>
  struct __error_impl<void, PromiseT, _promise> {
    template <typename U>
    static PromiseT<void> apply(const std::shared_ptr<_promise>& pm, U&& fn) {
      PromiseT<void> t;
      pm->error([t, fn](const any& err) -> void {
        try {
          fn(err);
        } catch (const any& err) {
          t.reject(err);
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
      pm->finally([t, fn]() -> void {
        try {
          PromiseT<Ret> tmp = fn();
          tmp.then([t](const Ret& val) -> void { t.resolve(val); });
          tmp.error([t](const any& err) -> void { t.reject(err); });
        } catch (const any& err) {
          t.reject(err);
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
      pm->finally([t, fn]() -> void {
        try {
          PromiseT<void> tmp = fn();
          tmp.then([t]() -> void { t.resolve(); });
          tmp.error([t](const any& err) -> void { t.reject(err); });
        } catch (const any& err) {
          t.reject(err);
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
      pm->finally([t, fn]() -> void {
        try {
          t.resolve(fn());
        } catch (const any& err) {
          t.reject(err);
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
    static PromiseT<Ret> apply(const std::shared_ptr<_promise>& pm, U&& fn) {
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
      pm->finally([t, fn]() -> void {
        try {
          fn();
        } catch (const any& err) {
          t.reject(err);
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
/**
 * @brief promise 对象。
 *
 * @tparam T Promise返回的值。
 */
template <typename T>
struct promise : public basic_promise {
  using value_type = typename std::decay<T>::type;

 private:
  struct _promise {
    using type = detail::function<void(const value_type&)>;
    using error_type = detail::function<void(const any&)>;
    using finally_type = detail::function<void()>;
    void then(type&& fn) {
      _then = std::move(fn);
      if (_status == Fulfilled) {
        _then(*(value_type*)_val);
      }
    }
    void error(error_type&& fn) {
      _error = std::move(fn);
      if (_status == Rejected) {
        _error(*(any*)_val);
      }
    }
    void finally(finally_type&& fn) {
      _finally = std::move(fn);
      if (_status != Pending) {
        _finally();
      }
    }
    void resolve(const value_type& val) {
      if (_status == Pending) {
        _status = Fulfilled;
        new (_val) value_type(val);
        if (_then) _then((*(value_type*)_val));
        if (_finally) _finally();
      }
    }
    void resolve(value_type&& val) {
      if (_status == Pending) {
        _status = Fulfilled;
        new (_val) value_type(std::move(val));
        if (_then) _then((*(value_type*)_val));
        if (_finally) _finally();
      }
    }
    void reject(const any& err) {
      if (_status == Pending) {
        _status = Rejected;
        new (_val) any(err);
        if (_error) _error(*(any*)_val);
        if (_finally) _finally();
      }
    }
    void reject(any&& err) {
      if (_status == Pending) {
        _status = Rejected;
        new (_val) any(std::move(err));
        if (_error) _error(*(any*)_val);
        if (_finally) _finally();
      }
    }
    constexpr state_t status() const noexcept { return _status; }
    _promise() : _status(Pending) {}
    _promise(const _promise& val) = delete;
    ~_promise() {
      if (_status == Fulfilled) {
        ((value_type*)_val)->~value_type();
      } else if (_status == Rejected) {
        ((any*)_val)->~any();
      }
    }

   private:
    state_t _status;
    type _then;
    error_type _error;
    finally_type _finally;
    alignas(alignof(value_type) > alignof(any)
                ? alignof(value_type)
                : alignof(any)) unsigned char _val
        [sizeof(value_type) > sizeof(any) ? sizeof(value_type) : sizeof(any)];
  };
  std::shared_ptr<_promise> pm;

 public:
  /**
   * @brief 设定在 promise 完成后执行的函数。返回一个对这个函数的
   * Promise，当函数返回时，Promise 被 resolve。如果返回一个
   * Promise，则会等待这个 promise 完成以后再 resolve。
   *
   * @param fn 要执行的函数。
   * @return promise<Ret> 对函数的 promise
   */
  template <typename U>
  promise<typename remove_promise<
      decltype(std::declval<U>()(std::declval<value_type>())), promise>::type>
  then(U&& fn) const {
    using Ret = decltype(std::declval<U>()(std::declval<value_type>()));
    return __then_impl<Ret, value_type, promise, _promise>::apply(
        pm, std::forward<U>(fn));
  }
  /**
   * @brief 设定在 promise 发生错误后执行的函数。返回一个对这个函数的
   * Promise，当函数返回时，Promise 被 resolve。如果返回一个
   * Promise，则会等待这个 promise 完成以后再 resolve。
   *
   * @param fn 要执行的函数。
   * @return promise<Ret> 对函数的 Promise。
   */
  template <typename U>
  promise<typename remove_promise<
      decltype(std::declval<U>()(std::declval<any>())), promise>::type>
  error(U&& fn) const {
    using Ret = decltype(std::declval<U>()(std::declval<any>()));
    return __error_impl<Ret, promise, _promise>::apply(pm, std::forward<U>(fn));
  }
  /**
   * @brief 设定在 promise
   * 无论发生错误还是正确返回都会执行的函数。不关心 promise
   * 的返回值。返回一个对这个函数的 Promise，当函数返回时，Promise 被
   * resolve。如果返回一个 Promise，则会等待这个 promise 完成以后再 resolve。
   *
   * @param fn 要执行的函数。
   * @return promise<Ret> 对函数的 Promise。
   */
  template <typename U>
  promise<typename remove_promise<decltype(std::declval<U>()()), promise>::type>
  finally(U&& fn) const {
    using Ret = decltype(std::declval<U>()());
    return __finally_impl<Ret, promise, _promise>::apply(pm,
                                                         std::forward<U>(fn));
  }
  /**
   * @brief 完成此Promise。
   *
   * @param value 结果值。
   */
  void resolve(const value_type& value) const { pm->resolve(value); }
  void resolve(value_type&& value) const { pm->resolve(std::move(value)); }
  /**
   * @brief 拒绝此Promise。
   *
   * @param err 异常。
   */
  void reject(const any& value) const { pm->reject(value); }
  void reject(any&& value) const { pm->reject(std::move(value)); }
  /**
   * @brief 获得Promise的状态。
   *
   * @return Status Promise的状态
   */
  state_t status() const noexcept { return pm->status(); }
  explicit promise() : pm(new _promise()) {}
};
/**
 * @brief 没有值的 promise 对象。
 */
template <>
class promise<void> : public basic_promise {
  struct _promise {
    using type = detail::function<void()>;
    using error_type = detail::function<void(const any&)>;
    using finally_type = detail::function<void()>;
    void then(type&& fn) {
      // if (_then) {
      //   _then = [fn]() -> void {

      //   }
      // } else
      _then = std::move(fn);
      if (_status == Fulfilled) {
        _then();
      }
    }
    void error(error_type&& fn) {
      _error = std::move(fn);
      if (_status == Rejected) {
        _error(_val);
      }
    }
    void finally(finally_type&& fn) {
      _finally = std::move(fn);
      if (_status != Pending) {
        _finally();
      }
    }
    void resolve() {
      if (_status == Pending) {
        _status = Fulfilled;
        if (_then) _then();
        if (_finally) _finally();
      }
    }
    void reject(const any& err) {
      if (_status == Pending) {
        _status = Rejected;
        _val = err;
        if (_error) _error(_val);
        if (_finally) _finally();
      }
    }
    void reject(any&& err) {
      if (_status == Pending) {
        _status = Rejected;
        _val = std::move(err);
        if (_error) _error(_val);
        if (_finally) _finally();
      }
    }
    constexpr state_t status() const noexcept { return _status; }
    _promise(const _promise& val) = delete;
    _promise() : _status(Pending) {}

   private:
    state_t _status;
    type _then;
    error_type _error;
    finally_type _finally;
    any _val;
  };
  std::shared_ptr<_promise> pm;

 public:
  using value_type = void;
  /**
   * @brief 设定在 promise 完成后执行的函数。返回一个对这个函数的
   * Promise，当函数返回时，Promise 被 resolve。如果返回一个
   * Promise，则会等待这个 promise 完成以后再 resolve。
   *
   * @param fn 要执行的函数。
   * @return promise<Ret> 对函数的 promise
   */
  template <typename U>
  promise<typename remove_promise<decltype(std::declval<U>()()), promise>::type>
  then(U&& fn) const {
    using Ret = decltype(std::declval<U>()());
    return __then_impl<Ret, void, promise, _promise>::apply(
        pm, std::forward<U>(fn));
  }
  /**
   * @brief 设定在 promise 发生错误后执行的函数。返回一个对这个函数的
   * Promise，当函数返回时，Promise 被 resolve。如果返回一个
   * Promise，则会等待这个 promise 完成以后再 resolve。
   *
   * @param fn 要执行的函数。
   * @return promise<Ret> 对函数的 Promise。
   */
  template <typename U>
  promise<typename remove_promise<
      decltype(std::declval<U>()(std::declval<any>())), promise>::type>
  error(U&& fn) const {
    using Ret = decltype(std::declval<U>()(std::declval<any>()));
    return __error_impl<Ret, promise, _promise>::apply(pm, std::forward<U>(fn));
  }
  /**
   * @brief 设定在 promise
   * 无论发生错误还是正确返回都会执行的函数。不关心 promise
   * 的返回值。返回一个对这个函数的 Promise，当函数返回时，Promise 被
   * resolve。如果返回一个 Promise，则会等待这个 promise 完成以后再 resolve。
   *
   * @param fn 要执行的函数。
   * @return promise<Ret> 对函数的 Promise。
   */
  template <typename U>
  promise<typename remove_promise<decltype(std::declval<U>()()), promise>::type>
  finally(U&& fn) const {
    using Ret = decltype(std::declval<U>()());
    return __finally_impl<Ret, promise, _promise>::apply(pm,
                                                         std::forward<U>(fn));
  }
  /**
   * @brief 完成此Promise。
   *
   * @param value 结果值。
   */
  void resolve() const { pm->resolve(); }
  /**
   * @brief 拒绝此Promise。
   *
   * @param err 异常。
   */
  void reject(const any& value) const { pm->reject(value); }
  void reject(any&& value) const { pm->reject(std::move(value)); }
  /**
   * @brief 获得Promise的状态。
   *
   * @return Status Promise的状态
   */
  state_t status() const noexcept { return pm->status(); }
  explicit promise() : pm(new _promise()) {}
};
namespace detail {
template <typename ResultType, typename Tuple, size_t TOTAL, size_t N>
struct _promise_all {
  static void apply(const Tuple& t, const std::shared_ptr<ResultType>& result,
                    const std::shared_ptr<size_t>& done_count,
                    const promise<ResultType>& pm) {
    _promise_all<ResultType, Tuple, TOTAL, N - 1>::apply(t, result, done_count,
                                                         pm);
    std::get<N>(t).then(
        [result, done_count,
         pm](const typename std::tuple_element<N, ResultType>::type& value)
            -> void {
          std::get<N>(*result) = value;
          if ((++(*done_count)) == TOTAL) {
            pm.resolve(*result);
          }
        });
    std::get<N>(t).error([pm](const awacorn::any& v) -> void { pm.reject(v); });
  }
};
template <typename ResultType, typename Tuple, size_t TOTAL>
struct _promise_all<ResultType, Tuple, TOTAL, 0> {
  static void apply(const Tuple& t, const std::shared_ptr<ResultType>& result,
                    const std::shared_ptr<size_t>& done_count,
                    const promise<ResultType>& pm) {
    std::get<0>(t).then(
        [result, done_count,
         pm](const typename std::tuple_element<0, ResultType>::type& value)
            -> void {
          std::get<0>(*result) = value;
          if ((++(*done_count)) == TOTAL) {
            pm.resolve(*result);
          }
        });
    std::get<0>(t).error([pm](const awacorn::any& v) -> void { pm.reject(v); });
  }
};
template <typename ResultType, typename Tuple, size_t TOTAL, size_t N>
struct _promise_any {
  static void apply(const Tuple& t, const std::shared_ptr<size_t>& fail_count,
                    const promise<void>& pm) {
    _promise_any<ResultType, Tuple, TOTAL, N - 1>::apply(t, fail_count, pm);
    std::get<N>(t).then(
        [pm](const typename std::tuple_element<N, ResultType>::type&) -> void {
          pm.resolve();
        });
    std::get<N>(t).error([pm, fail_count](const awacorn::any& v) -> void {
      if ((++(*fail_count)) == TOTAL) {
        pm.reject(v);
      }
    });
  }
};
template <typename ResultType, typename Tuple, size_t TOTAL>
struct _promise_any<ResultType, Tuple, TOTAL, 0> {
  static void apply(const Tuple& t, const std::shared_ptr<size_t>& fail_count,
                    const promise<void>& pm) {
    std::get<0>(t).then(
        [pm](const typename std::tuple_element<0, ResultType>::type&) -> void {
          pm.resolve();
        });
    std::get<0>(t).error([pm, fail_count](const awacorn::any& v) -> void {
      if ((++(*fail_count)) == TOTAL) {
        pm.reject(v);
      }
    });
  }
};
template <typename Tuple, size_t TOTAL, size_t N>
struct _promise_race {
  static void apply(const Tuple& t, const promise<void>& pm) {
    _promise_race<Tuple, TOTAL, N - 1>::apply(t, pm);
    std::get<N>(t).finally([pm]() -> void { pm.resolve(); });
  }
};
template <typename Tuple, size_t TOTAL>
struct _promise_race<Tuple, TOTAL, 0> {
  static void apply(const Tuple& t, const promise<void>& pm) {
    std::get<0>(t).finally([pm]() -> void { pm.resolve(); });
  }
};
template <typename Tuple, size_t TOTAL, size_t N>
struct _promise_allSettled {
  static void apply(const Tuple& t, const std::shared_ptr<size_t>& done_count,
                    const promise<void>& pm) {
    _promise_allSettled<Tuple, TOTAL, N - 1>::apply(t, done_count, pm);
    std::get<N>(t).finally([done_count, pm]() -> void {
      if ((++(*done_count)) == TOTAL) {
        pm.resolve();
      }
    });
  }
};
template <typename Tuple, size_t TOTAL>
struct _promise_allSettled<Tuple, TOTAL, 0> {
  static void apply(const Tuple& t, const std::shared_ptr<size_t>& done_count,
                    const promise<void>& pm) {
    std::get<0>(t).finally([done_count, pm]() -> void {
      if ((++(*done_count)) == TOTAL) {
        pm.resolve();
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
 * @param arg Args 多个 Promise。跟返回类型有关。
 * @return promise<std::tuple<Args...>> 获得所有结果的 Promise。
 */
template <typename... Args>
static promise<std::tuple<Args...>> all(const promise<Args>&... arg) {
  using ResultType = std::tuple<Args...>;
  promise<ResultType> pm;
  std::shared_ptr<ResultType> result =
      std::shared_ptr<ResultType>(new ResultType());
  std::shared_ptr<size_t> done_count = std::shared_ptr<size_t>(new size_t(0));
  detail::_promise_all<
      ResultType, std::tuple<promise<Args>...>, sizeof...(Args),
      sizeof...(Args) - 1>::apply(std::forward_as_tuple(arg...), result,
                                  done_count, pm);
  return pm;
}
/**
 * @brief 在任意一个 promise 完成后 resolve。返回一个回调用 Promise。若全部
 * promise 都失败，则返回的 promise 也失败。 注意：因为无法确定哪个 promise
 * 先返回，而 promise::any 允许不同返回值的 Promise，故无法取得返回值。
 *
 * @tparam Args 返回类型
 * @param arg 多个 Promise。跟返回类型有关。
 * @return promise<void> 回调用 Promise。
 */
template <typename... Args>
static promise<void> any(const promise<Args>&... arg) {
  using ResultType = std::tuple<Args...>;
  promise<void> pm;
  std::shared_ptr<size_t> fail_count = std::shared_ptr<size_t>(new size_t(0));
  detail::_promise_any<
      ResultType, std::tuple<promise<Args>...>, sizeof...(Args),
      sizeof...(Args) - 1>::apply(std::forward_as_tuple(arg...), fail_count,
                                  pm);
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
static promise<void> race(const promise<Args>&... arg) {
  promise<void> pm;
  detail::_promise_race<std::tuple<promise<Args>...>, sizeof...(Args),
                        sizeof...(Args) -
                            1>::apply(std::forward_as_tuple(arg...), pm);
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
static promise<void> all_settled(const promise<Args>&... arg) {
  promise<void> pm;
  std::shared_ptr<size_t> done_count = std::shared_ptr<size_t>(new size_t(0));
  detail::_promise_allSettled<std::tuple<promise<Args>...>, sizeof...(Args),
                              sizeof...(Args) -
                                  1>::apply(std::forward_as_tuple(arg...),
                                            done_count, pm);
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
promise<Value> resolve(Value&& val) {
  promise<Value> tmp;
  tmp.resolve(std::forward<Value>(val));
  return tmp;
}
promise<void> resolve() {
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
promise<Value> resolve(const promise<Value>& val) {
  return val;
}
template <typename Value>
promise<Value> resolve(promise<Value>&& val) {
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
promise<Value> reject(const awacorn::any& err) {
  promise<Value> tmp;
  tmp.reject(err);
  return tmp;
}
template <typename Value>
promise<Value> reject(awacorn::any&& err) {
  promise<Value> tmp;
  tmp.reject(std::move(err));
  return tmp;
}
};  // namespace awacorn
#endif
#endif
