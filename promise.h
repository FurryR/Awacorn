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
 * @brief Promise的状态。
 */
typedef enum Status {
  Pending = 0,   // 等待中。
  Resolved = 1,  // 已完成。
  Rejected = 2   // 已失败。
} Status;
/**
 * @brief Promise 对象。
 *
 * @tparam T Promise返回的值。
 */
template <typename T>
class Promise {
  struct _Promise {
    typedef std::function<void(const T&)> type;
    typedef std::function<void(const std::exception&)> error_type;
    typedef std::function<void()> finally_type;
    void then(const type& fn) {
      _then = fn;
      if (_status == Resolved) {
        _then(*reinterpret_cast<T*>(_val));
      }
    }
    void error(const error_type& fn) {
      _error = fn;
      if (_status == Rejected) {
        _error(*reinterpret_cast<std::exception*>(_val));
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
        (*reinterpret_cast<T*>(_val)) = val;
        if (_then) _then(val);
        if (_finally) _finally();
      }
    }
    void reject(const std::exception& err) {
      if (_status == Pending) {
        _status = Rejected;
        (*reinterpret_cast<std::exception*>(_val)) = err;
        if (_error) _error(err);
        if (_finally) _finally();
      }
    }
    constexpr Status status() const noexcept { return _status; }
    _Promise() : _status(Pending) {}
    _Promise(const _Promise& val) : _status(val._status) {
      switch (val._status) {
        case Resolved:
          (*reinterpret_cast<T*>(_val)) = (*reinterpret_cast<const T*>(val._val));
        case Rejected:
          (*reinterpret_cast<std::exception*>(_val)) =
              (*reinterpret_cast<const std::exception*>(val._val));
      }
    }
    ~_Promise() {
      switch (_status) {
        case Resolved: {
          reinterpret_cast<T*>(_val)->~T();
          break;
        }
        case Rejected: {
          reinterpret_cast<std::exception*>(_val)->~exception();
          break;
        }
      }
    }

   private:
    Status _status;
    type _then;
    error_type _error;
    finally_type _finally;
    alignas(alignof(T) > alignof(std::exception)
                ? sizeof(T)
                : sizeof(std::exception)) unsigned char _val
        [sizeof(T) > sizeof(std::exception) ? sizeof(T)
                                            : sizeof(std::exception)];
  };
  std::shared_ptr<_Promise> pm;

 public:
  typedef T type;
  /**
   * @brief 设定在 Promise 完成后执行的函数。返回一个对这个函数的
   * Promise，当函数返回时，Promise 被 resolve。如果返回一个
   * Promise，则会等待这个 Promise 完成以后再 resolve。
   *
   * @tparam Ret 函数的返回类型。
   * @param fn 要执行的函数。
   * @return Promise<Ret> 对函数的 Promise
   */
  template <typename Ret,
            typename = typename std::enable_if<!std::is_void<Ret>::value>::type>
  Promise<typename std::decay<Ret>::type> then(
      const std::function<typename std::decay<Ret>::type(const T&)>& fn) const {
    using Decay = typename std::decay<Ret>::type;
    Promise<Decay> t;
    pm->then([t, fn](const T& val) -> void {
      try {
        t.resolve(fn(val));
      } catch (const std::exception& err) {
        t.reject(err);
      }
    });
    return t;
  }
  template <typename Ret>
  Promise<Ret> then(const std::function<Ret(const T&)>& fn) const {
    Promise<Ret> t;
    pm->then([t, fn](const T& val) -> void {
      try {
        fn(val);
      } catch (const std::exception& err) {
        t.reject(err);
        return;
      }
      t.resolve();
    });
    return t;
  }
  template <typename Ret,
            typename = typename std::enable_if<!std::is_void<Ret>::value>::type>
  Promise<typename std::decay<Ret>::type> then(
      const std::function<Promise<typename std::decay<Ret>::type>(const T&)>&
          fn) const {
    using Decay = typename std::decay<Ret>::type;
    Promise<Decay> t;
    pm->then([t, fn](const T& val) -> void {
      Promise<Decay> tmp = fn(val);
      tmp.template then<void>(
          [t](const Decay& val) -> void { t.resolve(val); });
      tmp.template error<void>(
          [t](const std::exception& err) -> void { t.reject(err); });
    });
    return t;
  }
  template <typename Ret>
  Promise<Ret> then(const std::function<Promise<Ret>(const T&)>& fn) const {
    Promise<Ret> t;
    pm->then([t, fn](const T& val) -> void {
      Promise<Ret> tmp = fn(val);
      tmp.template then<void>([t]() -> void { t.resolve(); });
      tmp.template error<void>(
          [t](const std::exception& err) -> void { t.reject(err); });
    });
    return t;
  }
  /**
   * @brief 设定在 Promise 发生错误后执行的函数。返回一个对这个函数的
   * Promise，当函数返回时，Promise 被 resolve。如果返回一个
   * Promise，则会等待这个 Promise 完成以后再 resolve。
   *
   * @tparam Ret 函数的返回类型。
   * @param fn 要执行的函数。
   * @return Promise<Ret> 对函数的 Promise。
   */
  template <typename Ret,
            typename = typename std::enable_if<!std::is_void<Ret>::value>::type>
  Promise<typename std::decay<Ret>::type> error(
      const std::function<
          typename std::decay<Ret>::type(const std::exception&)>& fn) const {
    using Decay = typename std::decay<Ret>::type;
    Promise<Decay> t;
    pm->error([t, fn](const std::exception& err) -> void {
      try {
        t.resolve(fn(err));
      } catch (const std::exception& err2) {
        t.reject(err2);
      }
    });
    return t;
  }
  template <typename Ret>
  Promise<Ret> error(
      const std::function<Ret(const std::exception&)>& fn) const {
    Promise<Ret> t;
    pm->error([t, fn](const std::exception& err) -> void {
      try {
        fn(err);
      } catch (const std::exception& err2) {
        t.reject(err2);
        return;
      }
      t.resolve();
    });
    return t;
  }
  template <typename Ret,
            typename = typename std::enable_if<!std::is_void<Ret>::value>::type>
  Promise<typename std::decay<Ret>::type> error(
      const std::function<Promise<typename std::decay<Ret>::type>(
          const std::exception&)>& fn) const {
    using Decay = typename std::decay<Ret>::type;
    Promise<Decay> t;
    pm->error([t, fn](const std::exception& err) -> void {
      Promise<Decay> tmp = fn(err);
      tmp.template then<void>(
          [t](const Decay& val) -> void { t.resolve(val); });
      tmp.template error<void>(
          [t](const std::exception& err) -> void { t.reject(err); });
    });
    return t;
  }
  template <typename Ret>
  Promise<Ret> error(
      const std::function<Promise<Ret>(const std::exception&)>& fn) const {
    Promise<Ret> t;
    pm->error([t, fn](const std::exception& err) -> void {
      Promise<Ret> tmp = fn(err);
      tmp.template then<void>([t]() -> void { t.resolve(); });
      tmp.template error<void>(
          [t](const std::exception& err) -> void { t.reject(err); });
    });
    return t;
  }
  /**
   * @brief 设定在 Promise
   * 无论发生错误还是正确返回都会执行的函数。不关心 Promise
   * 的返回值。返回一个对这个函数的 Promise，当函数返回时，Promise 被
   * resolve。如果返回一个 Promise，则会等待这个 Promise 完成以后再 resolve。
   *
   * @tparam Ret 函数的返回类型。
   * @param fn 要执行的函数。
   * @return Promise<Ret> 对函数的 Promise。
   */
  template <typename Ret,
            typename = typename std::enable_if<!std::is_void<Ret>::value>::type>
  Promise<typename std::decay<Ret>::type> finally(
      const std::function<typename std::decay<Ret>::type()>& fn) const {
    using Decay = typename std::decay<Ret>::type;
    Promise<Decay> t;
    pm->finally([t, fn]() -> void {
      try {
        t.resolve(fn());
      } catch (const std::exception& err) {
        t.reject(err);
      }
    });
    return t;
  }
  template <typename Ret>
  Promise<Ret> finally(const std::function<Ret()>& fn) const {
    Promise<Ret> t;
    pm->finally([t, fn]() -> void {
      try {
        fn();
      } catch (const std::exception& err) {
        t.reject(err);
        return;
      }
      t.resolve();
    });
    return t;
  }
  template <typename Ret,
            typename = typename std::enable_if<!std::is_void<Ret>::value>::type>
  Promise<typename std::decay<Ret>::type> finally(
      const std::function<Promise<typename std::decay<Ret>::type>()>& fn) {
    using Decay = typename std::decay<Ret>::type;
    Promise<Decay> t;
    pm->finally([t, fn]() -> void {
      Promise<Decay> tmp = fn();
      tmp.template then<void>(
          [t](const Decay& val) -> void { t.resolve(val); });
      tmp.template error<void>(
          [t](const std::exception& err) -> void { t.reject(err); });
    });
    return t;
  }
  template <typename Ret>
  Promise<Ret> finally(const std::function<Promise<Ret>()>& fn) {
    Promise<Ret> t;
    pm->finally([t, fn]() -> void {
      Promise<Ret> tmp = fn();
      tmp.template then<void>([t]() -> void { t.resolve(); });
      tmp.template error<void>(
          [t](const std::exception& err) -> void { t.reject(err); });
    });
    return t;
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
  void reject(const std::exception& value) const { pm->reject(value); }
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
    typedef std::function<void(const std::exception&)> error_type;
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
    void reject(const std::exception& err) {
      if (_status == Pending) {
        _status = Rejected;
        _val = err;
        if (_error) _error(err);
        if (_finally) _finally();
      }
    }
    constexpr Status status() const noexcept { return _status; }
    _Promise() : _status(Pending) {}

   private:
    Status _status;
    type _then;
    error_type _error;
    finally_type _finally;
    std::exception _val;
  };
  std::shared_ptr<_Promise> pm;

 public:
  typedef void type;
  /**
   * @brief 设定在 Promise 完成后执行的函数。返回一个对这个函数的
   * Promise，当函数返回时，Promise 被 resolve。如果返回一个
   * Promise，则会等待这个 Promise 完成以后再 resolve。
   *
   * @tparam Ret 函数的返回类型。
   * @param fn 要执行的函数。
   * @return Promise<Ret> 对函数的 Promise
   */
  template <typename Ret,
            typename = typename std::enable_if<!std::is_void<Ret>::value>::type>
  Promise<typename std::decay<Ret>::type> then(
      const std::function<typename std::decay<Ret>::type()>& fn) const {
    using Decay = typename std::decay<Ret>::type;
    Promise<Decay> t;
    pm->then([t, fn]() -> void {
      try {
        t.resolve(fn());
      } catch (const std::exception& err) {
        t.reject(err);
      }
    });
    return t;
  }
  template <typename Ret>
  Promise<Ret> then(const std::function<Ret()>& fn) const {
    Promise<Ret> t;
    pm->then([t, fn]() -> void {
      try {
        fn();
      } catch (const std::exception& err) {
        t.reject(err);
        return;
      }
      t.resolve();
    });
    return t;
  }
  template <typename Ret,
            typename = typename std::enable_if<!std::is_void<Ret>::value>::type>
  Promise<typename std::decay<Ret>::type> then(
      const std::function<Promise<typename std::decay<Ret>::type>()>& fn)
      const {
    using Decay = typename std::decay<Ret>::type;
    Promise<Decay> t;
    pm->then([t, fn]() -> void {
      Promise<Decay> tmp = fn();
      tmp.template then<void>(
          [t](const Decay& val) -> void { t.resolve(val); });
      tmp.template error<void>(
          [t](const std::exception& err) -> void { t.reject(err); });
    });
    return t;
  }
  template <typename Ret>
  Promise<Ret> then(const std::function<Promise<Ret>()>& fn) const {
    Promise<Ret> t;
    pm->then([t, fn]() -> void {
      Promise<Ret> tmp = fn();
      tmp.template then<void>([t]() -> void { t.resolve(); });
      tmp.template error<void>(
          [t](const std::exception& err) -> void { t.reject(err); });
    });
    return t;
  }
  /**
   * @brief 设定在 Promise 发生错误后执行的函数。返回一个对这个函数的
   * Promise，当函数返回时，Promise 被 resolve。如果返回一个
   * Promise，则会等待这个 Promise 完成以后再 resolve。
   *
   * @tparam Ret 函数的返回类型。
   * @param fn 要执行的函数。
   * @return Promise<Ret> 对函数的 Promise。
   */
  template <typename Ret,
            typename = typename std::enable_if<!std::is_void<Ret>::value>::type>
  Promise<typename std::decay<Ret>::type> error(
      const std::function<
          typename std::decay<Ret>::type(const std::exception&)>& fn) const {
    using Decay = typename std::decay<Ret>::type;
    Promise<Decay> t;
    pm->error([t, fn](const std::exception& err) -> void {
      try {
        t.resolve(fn(err));
      } catch (const std::exception& err2) {
        t.reject(err2);
      }
    });
    return t;
  }
  template <typename Ret>
  Promise<Ret> error(
      const std::function<Ret(const std::exception&)>& fn) const {
    Promise<Ret> t;
    pm->error([t, fn](const std::exception& err) -> void {
      try {
        fn(err);
      } catch (const std::exception& err2) {
        t.reject(err2);
        return;
      }
      t.resolve();
    });
    return t;
  }
  template <typename Ret,
            typename = typename std::enable_if<!std::is_void<Ret>::value>::type>
  Promise<typename std::decay<Ret>::type> error(
      const std::function<Promise<typename std::decay<Ret>::type>(
          const std::exception&)>& fn) const {
    using Decay = typename std::decay<Ret>::type;
    Promise<Decay> t;
    pm->error([t, fn](const std::exception& err) -> void {
      Promise<Decay> tmp = fn(err);
      tmp.template then<void>(
          [t](const Decay& val) -> void { t.resolve(val); });
      tmp.template error<void>(
          [t](const std::exception& err) -> void { t.reject(err); });
    });
    return t;
  }
  template <typename Ret>
  Promise<Ret> error(
      const std::function<Promise<Ret>(const std::exception&)>& fn) const {
    Promise<Ret> t;
    pm->error([t, fn](const std::exception& err) -> void {
      Promise<Ret> tmp = fn(err);
      tmp.template then<void>([t]() -> void { t.resolve(); });
      tmp.template error<void>(
          [t](const std::exception& err) -> void { t.reject(err); });
    });
    return t;
  }
  /**
   * @brief 设定在 Promise
   * 无论发生错误还是正确返回都会执行的函数。不关心 Promise
   * 的返回值。返回一个对这个函数的 Promise，当函数返回时，Promise 被
   * resolve。如果返回一个 Promise，则会等待这个 Promise 完成以后再 resolve。
   *
   * @tparam Ret 函数的返回类型。
   * @param fn 要执行的函数。
   * @return Promise<Ret> 对函数的 Promise。
   */
  template <typename Ret,
            typename = typename std::enable_if<!std::is_void<Ret>::value>::type>
  Promise<typename std::decay<Ret>::type> finally(
      const std::function<typename std::decay<Ret>::type()>& fn) const {
    using Decay = typename std::decay<Ret>::type;
    Promise<Decay> t;
    pm->finally([t, fn]() -> void {
      try {
        t.resolve(fn());
      } catch (const std::exception& err) {
        t.reject(err);
      }
    });
    return t;
  }
  template <typename Ret>
  Promise<Ret> finally(const std::function<Ret()>& fn) const {
    Promise<Ret> t;
    pm->finally([t, fn]() -> void {
      try {
        fn();
      } catch (const std::exception& err) {
        t.reject(err);
        return;
      }
      t.resolve();
    });
    return t;
  }
  template <typename Ret,
            typename = typename std::enable_if<!std::is_void<Ret>::value>::type>
  Promise<typename std::decay<Ret>::type> finally(
      const std::function<Promise<typename std::decay<Ret>::type>()>& fn) {
    using Decay = typename std::decay<Ret>::type;
    Promise<Decay> t;
    pm->finally([t, fn]() -> void {
      Promise<Decay> tmp = fn();
      tmp.template then<void>(
          [t](const Decay& val) -> void { t.resolve(val); });
      tmp.template then<void>(
          [t](const std::exception& err) -> void { t.reject(err); });
    });
    return t;
  }
  template <typename Ret>
  Promise<Ret> finally(const std::function<Promise<Ret>()>& fn) {
    Promise<Ret> t;
    pm->finally([t, fn]() -> void {
      Promise<Ret> tmp = fn();
      tmp.template then<void>([t]() -> void { t.resolve(); });
      tmp.template error<void>(
          [t](const std::exception& err) -> void { t.reject(err); });
    });
    return t;
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
  void reject(const std::exception& value) const { pm->reject(value); }
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
template <typename Value,
          typename = typename std::enable_if<std::is_void<Value>::value>::type>
Promise<Value> resolve() {
  Promise<Value> tmp;
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
Promise<Value> reject(const std::exception& err) {
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
    std::get<N>(t).template then<void>(
        [result, done_count,
         pm](const typename std::tuple_element<N, ResultType>::type& value)
            -> void {
          std::get<N>(*result) = value;
          if ((++(*done_count)) == TOTAL) {
            pm.resolve(*result);
          }
        });
    std::get<N>(t).template error<void>(
        [pm](const std::exception&) -> void { pm.reject(std::exception()); });
  }
};
template <typename ResultType, typename Tuple, size_t TOTAL>
struct _promise_all<ResultType, Tuple, TOTAL, 0> {
  static void apply(const Tuple& t, const std::shared_ptr<ResultType>& result,
                    const std::shared_ptr<size_t>& done_count,
                    const Promise<ResultType>& pm) {
    std::get<0>(t).template then<void>(
        [result, done_count,
         pm](const typename std::tuple_element<0, ResultType>::type& value)
            -> void {
          std::get<0>(*result) = value;
          if ((++(*done_count)) == TOTAL) {
            pm.resolve(*result);
          }
        });
    std::get<0>(t).template error<void>(
        [pm](const std::exception&) -> void { pm.reject(std::exception()); });
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
    std::get<N>(t).template then<void>(
        [pm](const typename std::tuple_element<N, ResultType>::type&) -> void {
          pm.resolve();
        });
    std::get<N>(t).template error<void>(
        [pm, fail_count](const std::exception&) -> void {
          if ((++(*fail_count)) == TOTAL) {
            pm.reject(std::exception());
          }
        });
  }
};
template <typename ResultType, typename Tuple, size_t TOTAL>
struct _promise_any<ResultType, Tuple, TOTAL, 0> {
  static void apply(const Tuple& t, const std::shared_ptr<size_t>& fail_count,
                    const Promise<void>& pm) {
    std::get<0>(t).template then<void>(
        [pm](const typename std::tuple_element<0, ResultType>::type&) -> void {
          pm.resolve();
        });
    std::get<0>(t).template error<void>(
        [pm, fail_count](const std::exception&) -> void {
          if ((++(*fail_count)) == TOTAL) {
            pm.reject(std::exception());
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
    std::get<N>(t).template finally<void>([pm]() -> void { pm.resolve(); });
  }
};
template <typename Tuple, size_t TOTAL>
struct _promise_race<Tuple, TOTAL, 0> {
  static void apply(const Tuple& t, const Promise<void>& pm) {
    std::get<0>(t).template finally<void>([pm]() -> void { pm.resolve(); });
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
    std::get<N>(t).template finally<void>([done_count, pm]() -> void {
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
    std::get<0>(t).template finally<void>([done_count, pm]() -> void {
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
