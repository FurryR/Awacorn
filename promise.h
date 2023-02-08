#ifndef _PROMISE_H
#define _PROMISE_H
/**
 * Project Awacorn 基于 MIT 协议开源。
 * Copyright(c) 凌 2022.
 */
#include <functional>
#include <memory>
#include <tuple>
namespace Promise {
/**
 * @brief 类型安全的最小 any 实现。
 */
typedef class Unknown {
  typedef struct Base {
    virtual ~Base() = default;
    virtual const std::type_info& type() const = 0;
    virtual std::unique_ptr<Base> clone() const = 0;
  } Base;
  template <typename T>
  struct Derived : Base {
    T data;
    virtual std::unique_ptr<Base> clone() const override {
      return std::unique_ptr<Base>(new Derived<T>(data));
    }
    virtual const std::type_info& type() const override { return typeid(T); }
    Derived(const T& data) : data(data) {}
  };
  std::unique_ptr<Base> ptr;

 public:
  Unknown() = default;
  template <typename T>
  Unknown(const T& val) : ptr(std::unique_ptr<Base>(new Derived<T>(val))) {}
  Unknown(const Unknown& val) : ptr(val.ptr ? val.ptr->clone() : nullptr) {}
  Unknown& operator=(const Unknown& rhs) { return *new (this) Unknown(rhs); }
  const std::type_info& type() const { return ptr->type(); }
  template <typename T>
  const T& cast() const {
    Unknown::Derived<T>* _ptr = dynamic_cast<Unknown::Derived<T>*>(ptr.get());
    if (_ptr) return _ptr->data;
    throw std::bad_cast();
  }
} Unknown;

/**
 * @brief Promise的状态。
 */
typedef enum Status {
  Pending = 0,   // 等待中。
  Resolved = 1,  // 已完成。
  Rejected = 2   // 已失败。
} Status;

// Promise<T>.then(std::function<Promise<Ret>(ArgType)>) sub-implementation
// pm.then(std::function<Promise<Ret>(ArgType)>) sub-implementation
template <typename Ret, typename ArgType, template <typename T> class PromiseT,
          typename _Promise>
struct _ThenPromiseImpl {
  static PromiseT<Ret> apply(
      const std::shared_ptr<_Promise>& pm,
      const std::function<PromiseT<Ret>(const ArgType&)>& fn) {
    PromiseT<Ret> t;
    pm->then([t, fn](const ArgType& val) -> void {
      try {
        PromiseT<Ret> tmp = fn(val);
        tmp.then([t](const Ret& val) -> void { t.resolve(val); });
        tmp.error([t](const Unknown& err) -> void { t.reject(err); });
      } catch (const Unknown& err) {
        t.reject(err);
      }
    });
    pm->error([t](const Unknown& err) -> void { t.reject(err); });
    return t;
  }
};
// pm.then(std::function<Promise<void>(ArgType)>) sub-implementation
template <typename ArgType, template <typename T> class PromiseT,
          typename _Promise>
struct _ThenPromiseImpl<void, ArgType, PromiseT, _Promise> {
  static PromiseT<void> apply(
      const std::shared_ptr<_Promise>& pm,
      const std::function<PromiseT<void>(const ArgType&)>& fn) {
    PromiseT<void> t;
    pm->then([t, fn](const ArgType& val) -> void {
      try {
        PromiseT<void> tmp = fn(val);
        tmp.then([t]() -> void { t.resolve(); });
        tmp.error([t](const Unknown& err) -> void { t.reject(err); });
      } catch (const Unknown& err) {
        t.reject(err);
      }
    });
    pm->error([t](const Unknown& err) -> void { t.reject(err); });
    return t;
  }
};
// Promise<void>.then(std::function<Promise<Ret>()>) sub-implementation
// pm.then(std::function<Promise<Ret>()>) sub-implementation
template <typename Ret, template <typename T> class PromiseT, typename _Promise>
struct _ThenPromiseImpl<Ret, void, PromiseT, _Promise> {
  static PromiseT<Ret> apply(const std::shared_ptr<_Promise>& pm,
                             const std::function<PromiseT<Ret>()>& fn) {
    PromiseT<Ret> t;
    pm->then([t, fn]() -> void {
      try {
        PromiseT<Ret> tmp = fn();
        tmp.then([t](const Ret& val) -> void { t.resolve(val); });
        tmp.error([t](const Unknown& err) -> void { t.reject(err); });
      } catch (const Unknown& err) {
        t.reject(err);
      }
    });
    pm->error([t](const Unknown& err) -> void { t.reject(err); });
    return t;
  }
};
// pm.then(std::function<Promise<void>()>) sub-implementation
template <template <typename T> class PromiseT, typename _Promise>
struct _ThenPromiseImpl<void, void, PromiseT, _Promise> {
  static PromiseT<void> apply(const std::shared_ptr<_Promise>& pm,
                              const std::function<PromiseT<void>()>& fn) {
    PromiseT<void> t;
    pm->then([t, fn]() -> void {
      try {
        PromiseT<void> tmp = fn();
        tmp.then([t]() -> void { t.resolve(); });
        tmp.error([t](const Unknown& err) -> void { t.reject(err); });
      } catch (const Unknown& err) {
        t.reject(err);
      }
    });
    pm->error([t](const Unknown& err) -> void { t.reject(err); });
    return t;
  }
};

// Promise<T>.then implementation
// pm.then(std::function<Ret(ArgType)>) implementation
template <typename Ret, typename ArgType, template <typename T> class PromiseT,
          typename _Promise>
struct _ThenImpl {
  static PromiseT<Ret> apply(const std::shared_ptr<_Promise>& pm,
                             const std::function<Ret(const ArgType&)>& fn) {
    PromiseT<Ret> t;
    pm->then([t, fn](const ArgType& val) -> void {
      try {
        t.resolve(fn(val));
      } catch (const Unknown& err) {
        t.reject(err);
      }
    });
    pm->error([t](const Unknown& err) -> void { t.reject(err); });
    return t;
  }
};
// pm.then(std::function<Promise<Ret>(ArgType)>) implementation
template <typename Ret, typename ArgType, template <typename T> class PromiseT,
          typename _Promise>
struct _ThenImpl<PromiseT<Ret>, ArgType, PromiseT, _Promise> {
  static PromiseT<Ret> apply(
      const std::shared_ptr<_Promise>& pm,
      const std::function<PromiseT<Ret>(const ArgType&)>& fn) {
    return _ThenPromiseImpl<Ret, ArgType, PromiseT, _Promise>::apply(pm, fn);
  }
};
// pm.then(std::function<void(ArgType)>) implementation
template <typename ArgType, template <typename T> class PromiseT,
          typename _Promise>
struct _ThenImpl<void, ArgType, PromiseT, _Promise> {
  static PromiseT<void> apply(const std::shared_ptr<_Promise>& pm,
                              const std::function<void(const ArgType&)>& fn) {
    PromiseT<void> t;
    pm->then([t, fn](const ArgType& val) -> void {
      try {
        fn(val);
        t.resolve();
      } catch (const Unknown& err) {
        t.reject(err);
      }
    });
    pm->error([t](const Unknown& err) -> void { t.reject(err); });
    return t;
  }
};

// Promise<void>.then implementation
// pm.then(std::function<Ret()>) implementation
template <typename Ret, template <typename T> class PromiseT, typename _Promise>
struct _ThenImpl<Ret, void, PromiseT, _Promise> {
  static PromiseT<Ret> apply(const std::shared_ptr<_Promise>& pm,
                             const std::function<Ret()>& fn) {
    PromiseT<Ret> t;
    pm->then([t, fn]() -> void {
      try {
        t.resolve(fn());
      } catch (const Unknown& err) {
        t.reject(err);
      }
    });
    pm->error([t](const Unknown& err) -> void { t.reject(err); });
    return t;
  }
};
// pm.then(std::function<Promise<Ret>()>) implementation
template <typename Ret, template <typename T> class PromiseT, typename _Promise>
struct _ThenImpl<PromiseT<Ret>, void, PromiseT, _Promise> {
  static PromiseT<Ret> apply(const std::shared_ptr<_Promise>& pm,
                             const std::function<PromiseT<Ret>()>& fn) {
    return _ThenPromiseImpl<Ret, void, PromiseT, _Promise>::apply(pm, fn);
  }
};
// pm.then(std::function<void()>) implementation
template <template <typename T> class PromiseT, typename _Promise>
struct _ThenImpl<void, void, PromiseT, _Promise> {
  static PromiseT<void> apply(const std::shared_ptr<_Promise>& pm,
                              const std::function<void()>& fn) {
    PromiseT<void> t;
    pm->then([t, fn]() -> void {
      try {
        fn();
        t.resolve();
      } catch (const Unknown& err) {
        t.reject(err);
      }
    });
    pm->error([t](const Unknown& err) -> void { t.reject(err); });
    return t;
  }
};

// Promise<T>.error(std::function<Promise<Ret>(Unknown)>)
// sub-implementation

// pm.error(std::function<Promise<Ret>(Unknown)>) sub-implementation
template <typename Ret, template <typename T> class PromiseT, typename _Promise>
struct _ErrorPromiseImpl {
  static PromiseT<Ret> apply(
      const std::shared_ptr<_Promise>& pm,
      const std::function<PromiseT<Ret>(const Unknown&)>& fn) {
    PromiseT<Ret> t;
    pm->error([t, fn](const Unknown& val) -> void {
      try {
        PromiseT<Ret> tmp = fn(val);
        tmp.then([t](const Ret& val) -> void { t.resolve(val); });
        tmp.error([t](const Unknown& err) -> void { t.reject(err); });
      } catch (const Unknown& err) {
        t.reject(err);
      }
    });
    return t;
  }
};
// pm.error(std::function<Promise<void>(Unknown)>) sub-implementation
template <template <typename T> class PromiseT, typename _Promise>
struct _ErrorPromiseImpl<void, PromiseT, _Promise> {
  static PromiseT<void> apply(
      const std::shared_ptr<_Promise>& pm,
      const std::function<PromiseT<void>(const Unknown&)>& fn) {
    PromiseT<void> t;
    pm->error([t, fn](const Unknown& val) -> void {
      try {
        PromiseT<void> tmp = fn(val);
        tmp.then([t]() -> void { t.resolve(); });
        tmp.error([t](const Unknown& err) -> void { t.reject(err); });
      } catch (const Unknown& err) {
        t.reject(err);
      }
    });
    return t;
  }
};

// Promise<T>.error implementation
// pm.error(std::function<Ret(Unknown)>) implementation
template <typename Ret, template <typename T> class PromiseT, typename _Promise>
struct _ErrorImpl {
  static PromiseT<Ret> apply(const std::shared_ptr<_Promise>& pm,
                             const std::function<Ret(const Unknown&)>& fn) {
    PromiseT<Ret> t;
    pm->error([t, fn](const Unknown& err) -> void {
      try {
        t.resolve(fn(err));
      } catch (const Unknown& err) {
        t.reject(err);
      }
    });
    return t;
  }
};
// pm.error(std::function<Promise<Ret>(Unknown)>) implementation
template <typename Ret, template <typename T> class PromiseT, typename _Promise>
struct _ErrorImpl<PromiseT<Ret>, PromiseT, _Promise> {
  static PromiseT<Ret> apply(
      const std::shared_ptr<_Promise>& pm,
      const std::function<PromiseT<Ret>(const Unknown&)>& fn) {
    return _ErrorPromiseImpl<Ret, PromiseT, _Promise>::apply(pm, fn);
  }
};
// pm.error(std::function<void(Unknown)>) implementation
template <template <typename T> class PromiseT, typename _Promise>
struct _ErrorImpl<void, PromiseT, _Promise> {
  static PromiseT<void> apply(const std::shared_ptr<_Promise>& pm,
                              const std::function<void(const Unknown&)>& fn) {
    PromiseT<void> t;
    pm->error([t, fn](const Unknown& err) -> void {
      try {
        fn(err);
      } catch (const Unknown& err) {
        t.reject(err);
        return;
      }
      t.resolve();
    });
    return t;
  }
};

// Promise<T>.finally(std::function<Promise<Ret>()>) sub-implementation
// pm.finally(std::function<Promise<Ret>()>) sub-implementation
template <typename Ret, template <typename T> class PromiseT, typename _Promise>
struct _FinallyPromiseImpl {
  static PromiseT<Ret> apply(const std::shared_ptr<_Promise>& pm,
                             const std::function<PromiseT<Ret>()>& fn) {
    PromiseT<Ret> t;
    pm->finally([t, fn]() -> void {
      try {
        PromiseT<Ret> tmp = fn();
        tmp.then([t](const Ret& val) -> void { t.resolve(val); });
        tmp.error([t](const Unknown& err) -> void { t.reject(err); });
      } catch (const Unknown& err) {
        t.reject(err);
      }
    });
    return t;
  }
};
// pm.finally(std::function<Promise<void>()>) sub-implementation
template <template <typename T> class PromiseT, typename _Promise>
struct _FinallyPromiseImpl<void, PromiseT, _Promise> {
  static PromiseT<void> apply(const std::shared_ptr<_Promise>& pm,
                              const std::function<PromiseT<void>()>& fn) {
    PromiseT<void> t;
    pm->finally([t, fn]() -> void {
      try {
        PromiseT<void> tmp = fn();
        tmp.then([t]() -> void { t.resolve(); });
        tmp.error([t](const Unknown& err) -> void { t.reject(err); });
      } catch (const Unknown& err) {
        t.reject(err);
      }
    });
    return t;
  }
};

// Promise<T>.finally implementation
// pm.finally(std::function<Ret()>) implementation
template <typename Ret, template <typename T> class PromiseT, typename _Promise>
struct _FinallyImpl {
  static PromiseT<Ret> apply(const std::shared_ptr<_Promise>& pm,
                             const std::function<Ret()>& fn) {
    PromiseT<Ret> t;
    pm->finally([t, fn]() -> void {
      try {
        t.resolve(fn());
      } catch (const Unknown& err) {
        t.reject(err);
      }
    });
    return t;
  }
};
// pm.finally(std::function<Promise<Ret>()>) implementation
template <typename Ret, template <typename T> class PromiseT, typename _Promise>
struct _FinallyImpl<PromiseT<Ret>, PromiseT, _Promise> {
  static PromiseT<Ret> apply(const std::shared_ptr<_Promise>& pm,
                             const std::function<PromiseT<Ret>()>& fn) {
    return _FinallyPromiseImpl<Ret, PromiseT, _Promise>::apply(pm, fn);
  }
};
// pm.finally(std::function<void()>) implementation
template <template <typename T> class PromiseT, typename _Promise>
struct _FinallyImpl<void, PromiseT, _Promise> {
  static PromiseT<void> apply(const std::shared_ptr<_Promise>& pm,
                              const std::function<void()>& fn) {
    PromiseT<void> t;
    pm->finally([t, fn]() -> void {
      try {
        fn();
      } catch (const Unknown& err) {
        t.reject(err);
        return;
      }
      t.resolve();
    });
    return t;
  }
};

template <typename T, template <typename T2> class PromiseT>
struct remove_promise {
  typedef T type;
};
template <typename T, template <typename T2> class PromiseT>
struct remove_promise<PromiseT<T>, PromiseT> {
  typedef T type;
};
/**
 * @brief Promise 对象。
 *
 * @tparam T Promise返回的值。
 */
template <typename T>
class Promise {
  struct _Promise {
    typedef std::function<void(const T&)> type;
    typedef std::function<void(const Unknown&)> error_type;
    typedef std::function<void()> finally_type;
    void then(const type& fn) {
      _then = fn;
      if (_status == Resolved) {
        _then(*(T*)_val);
      }
    }
    void error(const error_type& fn) {
      _error = fn;
      if (_status == Rejected) {
        _error(*(Unknown*)_val);
      }
    }
    void finally(const finally_type& fn) {
      _finally = fn;
      if (_status != Pending) {
        _finally();
      }
    }
    void resolve(const T& val) {
      if (_status == Pending) {
        _status = Resolved;
        (*(T*)_val) = val;
        if (_then) _then(val);
        if (_finally) _finally();
      }
    }
    void reject(const Unknown& err) {
      if (_status == Pending) {
        _status = Rejected;
        *(Unknown*)_val = err;
        if (_error) _error(err);
        if (_finally) _finally();
      }
    }
    constexpr Status status() const noexcept { return _status; }
    _Promise() : _status(Pending) {}
    _Promise(const _Promise& val) = delete;
    ~_Promise() {
      if (_status == Resolved) {
        ((T*)_val)->~T();
      } else if (_status == Rejected) {
        ((Unknown*)_val)->~Unknown();
      }
    }

   private:
    Status _status;
    type _then;
    error_type _error;
    finally_type _finally;
    alignas(alignof(T) > alignof(Unknown) ? sizeof(T)
                                          : sizeof(Unknown)) unsigned char _val
        [sizeof(T) > sizeof(Unknown) ? sizeof(T) : sizeof(Unknown)];
  };
  std::shared_ptr<_Promise> pm;

 public:
  typedef T type;
  /**
   * @brief 设定在 Promise 完成后执行的函数。返回一个对这个函数的
   * Promise，当函数返回时，Promise 被 resolve。如果返回一个
   * Promise，则会等待这个 Promise 完成以后再 resolve。
   *
   * @param fn 要执行的函数。
   * @return Promise<Ret> 对函数的 Promise
   */
  template <typename U>
  Promise<typename remove_promise<
      decltype(std::declval<U>()(std::declval<T>())), Promise>::type>
  then(const U& fn) const {
    using Ret = decltype(std::declval<U>()(std::declval<T>()));
    return _ThenImpl<Ret, T, Promise, _Promise>::apply(pm, fn);
  }
  /**
   * @brief 设定在 Promise 发生错误后执行的函数。返回一个对这个函数的
   * Promise，当函数返回时，Promise 被 resolve。如果返回一个
   * Promise，则会等待这个 Promise 完成以后再 resolve。
   *
   * @param fn 要执行的函数。
   * @return Promise<Ret> 对函数的 Promise。
   */
  template <typename U>
  Promise<typename remove_promise<
      decltype(std::declval<U>()(std::declval<Unknown>())), Promise>::type>
  error(const U& fn) const {
    using Ret = decltype(std::declval<U>()(std::declval<Unknown>()));
    return _ErrorImpl<Ret, Promise, _Promise>::apply(pm, fn);
  }
  /**
   * @brief 设定在 Promise
   * 无论发生错误还是正确返回都会执行的函数。不关心 Promise
   * 的返回值。返回一个对这个函数的 Promise，当函数返回时，Promise 被
   * resolve。如果返回一个 Promise，则会等待这个 Promise 完成以后再 resolve。
   *
   * @param fn 要执行的函数。
   * @return Promise<Ret> 对函数的 Promise。
   */
  template <typename U>
  Promise<typename remove_promise<decltype(std::declval<U>()()), Promise>::type>
  finally(const U& fn) const {
    using Ret = decltype(std::declval<U>()());
    return _FinallyImpl<Ret, Promise, _Promise>::apply(pm, fn);
  }
  /**
   * @brief 完成此Promise。
   *
   * @param value 结果值。
   */
  void resolve(const T& value) const { pm->resolve(value); }
  /**
   * @brief 拒绝此Promise。
   *
   * @param err 异常。
   */
  void reject(const Unknown& value) const { pm->reject(value); }
  /**
   * @brief 获得Promise的状态。
   *
   * @return Status Promise的状态
   */
  Status status() const noexcept { return pm->status(); }
  explicit Promise() : pm(new _Promise()) {}
};
/**
 * @brief 没有值的 Promise 对象。
 *
 */
template <>
class Promise<void> {
  struct _Promise {
    typedef std::function<void()> type;
    typedef std::function<void(const Unknown&)> error_type;
    typedef std::function<void()> finally_type;
    void then(const type& fn) {
      _then = fn;
      if (_status == Resolved) {
        _then();
      }
    }
    void error(const error_type& fn) {
      _error = fn;
      if (_status == Rejected) {
        _error(_val);
      }
    }
    void finally(const finally_type& fn) {
      _finally = fn;
      if (_status != Pending) {
        _finally();
      }
    }
    void resolve() {
      if (_status == Pending) {
        _status = Resolved;
        if (_then) _then();
        if (_finally) _finally();
      }
    }
    void reject(const Unknown& err) {
      if (_status == Pending) {
        _status = Rejected;
        _val = err;
        if (_error) _error(err);
        if (_finally) _finally();
      }
    }
    constexpr Status status() const noexcept { return _status; }
    _Promise(const _Promise& val) = delete;
    _Promise() : _status(Pending) {}

   private:
    Status _status;
    type _then;
    error_type _error;
    finally_type _finally;
    Unknown _val;
  };
  std::shared_ptr<_Promise> pm;

 public:
  typedef void type;
  /**
   * @brief 设定在 Promise 完成后执行的函数。返回一个对这个函数的
   * Promise，当函数返回时，Promise 被 resolve。如果返回一个
   * Promise，则会等待这个 Promise 完成以后再 resolve。
   *
   * @param fn 要执行的函数。
   * @return Promise<Ret> 对函数的 Promise
   */
  template <typename U>
  Promise<typename remove_promise<decltype(std::declval<U>()()), Promise>::type>
  then(const U& fn) const {
    using Ret = decltype(std::declval<U>()());
    return _ThenImpl<Ret, void, Promise, _Promise>::apply(pm, fn);
  }
  /**
   * @brief 设定在 Promise 发生错误后执行的函数。返回一个对这个函数的
   * Promise，当函数返回时，Promise 被 resolve。如果返回一个
   * Promise，则会等待这个 Promise 完成以后再 resolve。
   *
   * @param fn 要执行的函数。
   * @return Promise<Ret> 对函数的 Promise。
   */
  template <typename U>
  Promise<typename remove_promise<
      decltype(std::declval<U>()(std::declval<Unknown>())), Promise>::type>
  error(const U& fn) const {
    using Ret = decltype(std::declval<U>()(std::declval<Unknown>()));
    return _ErrorImpl<Ret, Promise, _Promise>::apply(pm, fn);
  }
  /**
   * @brief 设定在 Promise
   * 无论发生错误还是正确返回都会执行的函数。不关心 Promise
   * 的返回值。返回一个对这个函数的 Promise，当函数返回时，Promise 被
   * resolve。如果返回一个 Promise，则会等待这个 Promise 完成以后再 resolve。
   *
   * @param fn 要执行的函数。
   * @return Promise<Ret> 对函数的 Promise。
   */
  template <typename U>
  Promise<typename remove_promise<decltype(std::declval<U>()()), Promise>::type>
  finally(const U& fn) const {
    using Ret = decltype(std::declval<U>()());
    return _FinallyImpl<Ret, Promise, _Promise>::apply(pm, fn);
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
  void reject(const Unknown& value) const { pm->reject(value); }
  /**
   * @brief 获得Promise的状态。
   *
   * @return Status Promise的状态
   */
  Status status() const noexcept { return pm->status(); }
  explicit Promise() : pm(new _Promise()) {}
};
/**
 * @brief 返回一个已经 Resolved 的 Promise。
 *
 * @tparam Value Promise 类型
 * @param val 可选，Promise的值。
 * @return Promise<Value> 已经 Resolved 的 Promise
 */
template <typename Value>
Promise<Value> resolve(const Value& val) {
  Promise<Value> tmp;
  tmp.resolve(val);
  return tmp;
}
Promise<void> resolve() {
  Promise<void> tmp;
  tmp.resolve();
  return tmp;
}
/**
 * @brief 返回 Promise 本身。
 *
 * @tparam Value Promise 类型
 * @return Promise<Value> Promise 本身
 */
template <typename Value>
Promise<Value> resolve(const Promise<Value>& val) {
  return val;
}
/**
 * @brief 返回一个 Rejected 的 Promise。
 *
 * @tparam Value Promise 类型
 * @param err 错误内容。
 * @return Promise<Value> 已经 Rejected 的 Promise
 */
template <typename Value>
Promise<Value> reject(const Unknown& err) {
  Promise<Value> tmp;
  tmp.reject(err);
  return tmp;
}
template <typename ResultType, typename Tuple, size_t TOTAL, size_t N>
struct _promise_all {
  static void apply(const Tuple& t, const std::shared_ptr<ResultType>& result,
                    const std::shared_ptr<size_t>& done_count,
                    const Promise<ResultType>& pm) {
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
    std::get<N>(t).error(
        [pm](const Unknown&) -> void { pm.reject(Unknown()); });
  }
};
template <typename ResultType, typename Tuple, size_t TOTAL>
struct _promise_all<ResultType, Tuple, TOTAL, 0> {
  static void apply(const Tuple& t, const std::shared_ptr<ResultType>& result,
                    const std::shared_ptr<size_t>& done_count,
                    const Promise<ResultType>& pm) {
    std::get<0>(t).then(
        [result, done_count,
         pm](const typename std::tuple_element<0, ResultType>::type& value)
            -> void {
          std::get<0>(*result) = value;
          if ((++(*done_count)) == TOTAL) {
            pm.resolve(*result);
          }
        });
    std::get<0>(t).error(
        [pm](const Unknown&) -> void { pm.reject(Unknown()); });
  }
};
/**
 * @brief 等待所有 Promise 完成后才 resolve。当有 Promise 失败，立刻
 * reject。返回一个获得所有结果的 Promise。
 *
 * @tparam Args 返回类型
 * @param arg Args 多个 Promise。跟返回类型有关。
 * @return Promise<std::tuple<Args...>> 获得所有结果的 Promise。
 */
template <typename... Args>
Promise<std::tuple<Args...>> all(const Promise<Args>&... arg) {
  using ResultType = std::tuple<Args...>;
  Promise<ResultType> pm;
  std::shared_ptr<ResultType> result =
      std::shared_ptr<ResultType>(new ResultType());
  std::shared_ptr<size_t> done_count = std::shared_ptr<size_t>(new size_t(0));
  _promise_all<ResultType, std::tuple<Promise<Args>...>, sizeof...(Args),
               sizeof...(Args) - 1>::apply(std::forward_as_tuple(arg...),
                                           result, done_count, pm);
  return pm;
}
template <typename ResultType, typename Tuple, size_t TOTAL, size_t N>
struct _promise_any {
  static void apply(const Tuple& t, const std::shared_ptr<size_t>& fail_count,
                    const Promise<void>& pm) {
    _promise_any<ResultType, Tuple, TOTAL, N - 1>::apply(t, fail_count, pm);
    std::get<N>(t).then(
        [pm](const typename std::tuple_element<N, ResultType>::type&) -> void {
          pm.resolve();
        });
    std::get<N>(t).error([pm, fail_count](const Unknown&) -> void {
      if ((++(*fail_count)) == TOTAL) {
        pm.reject(Unknown());
      }
    });
  }
};
template <typename ResultType, typename Tuple, size_t TOTAL>
struct _promise_any<ResultType, Tuple, TOTAL, 0> {
  static void apply(const Tuple& t, const std::shared_ptr<size_t>& fail_count,
                    const Promise<void>& pm) {
    std::get<0>(t).then(
        [pm](const typename std::tuple_element<0, ResultType>::type&) -> void {
          pm.resolve();
        });
    std::get<0>(t).error([pm, fail_count](const Unknown&) -> void {
      if ((++(*fail_count)) == TOTAL) {
        pm.reject(Unknown());
      }
    });
  }
};
/**
 * @brief 在任意一个 Promise 完成后 resolve。返回一个回调用 Promise。若全部
 * Promise 都失败，则返回的 Promise 也失败。 注意：因为无法确定哪个 Promise
 * 先返回，而 Promise::any 允许不同返回值的 Promise，故无法取得返回值。
 *
 * @tparam Args 返回类型
 * @param arg 多个 Promise。跟返回类型有关。
 * @return Promise<void> 回调用 Promise。
 */
template <typename... Args>
Promise<void> any(const Promise<Args>&... arg) {
  using ResultType = std::tuple<Args...>;
  Promise<void> pm;
  std::shared_ptr<size_t> fail_count = std::shared_ptr<size_t>(new size_t(0));
  _promise_any<ResultType, std::tuple<Promise<Args>...>, sizeof...(Args),
               sizeof...(Args) - 1>::apply(std::forward_as_tuple(arg...),
                                           fail_count, pm);
  return pm;
}
template <typename Tuple, size_t TOTAL, size_t N>
struct _promise_race {
  static void apply(const Tuple& t, const Promise<void>& pm) {
    _promise_race<Tuple, TOTAL, N - 1>::apply(t, pm);
    std::get<N>(t).finally([pm]() -> void { pm.resolve(); });
  }
};
template <typename Tuple, size_t TOTAL>
struct _promise_race<Tuple, TOTAL, 0> {
  static void apply(const Tuple& t, const Promise<void>& pm) {
    std::get<0>(t).finally([pm]() -> void { pm.resolve(); });
  }
};
/**
 * @brief 在任意一个 Promise 完成或失败后 resolve。返回一个回调用
 * Promise。注意：因为无法确定哪个 Promise 先返回，而 Promise::race
 * 允许不同返回值的 Promise，故无法取得返回值。并且，Promise::race
 * 无论是否成功都会 resolve。
 *
 * @tparam Args 返回类型
 * @param arg 多个 Promise。跟返回类型有关。
 * @return Promise<void> 回调用 Promise。
 */
template <typename... Args>
Promise<void> race(const Promise<Args>&... arg) {
  Promise<void> pm;
  _promise_race<std::tuple<Promise<Args>...>, sizeof...(Args),
                sizeof...(Args) - 1>::apply(std::forward_as_tuple(arg...), pm);
  return pm;
}
template <typename Tuple, size_t TOTAL, size_t N>
struct _promise_allSettled {
  static void apply(const Tuple& t, const std::shared_ptr<size_t>& done_count,
                    const Promise<void>& pm) {
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
                    const Promise<void>& pm) {
    std::get<0>(t).finally([done_count, pm]() -> void {
      if ((++(*done_count)) == TOTAL) {
        pm.resolve();
      }
    });
  }
};
/**
 * @brief 在所有 Promise 都完成或失败后 resolve。返回一个回调用
 * Promise。注意：因为无法确定哪个 Promise 先返回，而 Promise::allSettled
 * 允许不同返回值的 Promise，故无法取得返回值。
 *
 * @tparam Args 返回类型
 * @param arg 多个 Promise。跟返回类型有关。
 * @return Promise<void> 回调用 Promise。
 */
template <typename... Args>
Promise<void> allSettled(const Promise<Args>&... arg) {
  Promise<void> pm;
  std::shared_ptr<size_t> done_count = std::shared_ptr<size_t>(new size_t(0));
  _promise_allSettled<std::tuple<Promise<Args>...>, sizeof...(Args),
                      sizeof...(Args) - 1>::apply(std::forward_as_tuple(arg...),
                                                  done_count, pm);
  return pm;
}
}  // namespace Promise
#endif
