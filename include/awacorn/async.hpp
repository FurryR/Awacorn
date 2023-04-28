#ifndef _AWACORN_ASYNC
#define _AWACORN_ASYNC
#if __cplusplus >= 201101L
/**
 * Project Awacorn 基于 MIT 协议开源。
 * Copyright(c) 凌 2023.
 */
#if !defined(AWACORN_USE_BOOST) && !defined(AWACORN_USE_UCONTEXT)
#if __has_include(<boost/coroutine2/coroutine.hpp>)
#define AWACORN_USE_BOOST
#elif __has_include(<ucontext.h>)
#define AWACORN_USE_UCONTEXT
#else
#error Neither <boost/coroutine2/coroutine.hpp> nor <ucontext.h> is found.
#endif
#endif
#if defined(AWACORN_USE_BOOST)
#include <boost/coroutine2/coroutine.hpp>
#elif defined(AWACORN_USE_UCONTEXT)
#ifdef __APPLE__
#define _XOPEN_SOURCE
#endif
#include <ucontext.h>
#endif

#include <memory>
#include <stdexcept>
#include <typeinfo>
#include <vector>

#include "detail/function.hpp"
#include "promise.hpp"

namespace awacorn {
namespace detail {
/**
 * @brief 生成器的状态。
 */
enum class _async_state_t {
  Pending = 0,   // 尚未运行
  Active = 1,    // 运行中
  Returned = 2,  // 已返回
  Awaiting = 3,  // await 中
  Throwed = 4    // 抛出错误
};
template <typename Fn>
struct _basic_async_fn;
template <typename RetType>
struct _async_fn;
};  // namespace detail
/**
 * @brief 生成器上下文基类。
 */
class context {
#if defined(AWACORN_USE_BOOST)
  context(void (*fn)(void*), void* arg, size_t = 0)
      : _status(detail::_async_state_t::Pending),
        _pullfn([this, fn,
                 arg](boost::coroutines2::coroutine<void>::push_type& pushfn) {
          _pushfn = &pushfn;
          (*_pushfn)();
          fn(arg);
        }),
        _push_or_pull(false) {}
#elif defined(AWACORN_USE_UCONTEXT)
  context(void (*fn)(void*), void* arg, size_t stack_size = 0)
      : _status(detail::_async_state_t::Pending),
        _stack(nullptr, [](char* ptr) {
          if (ptr) delete[] ptr;
        }) {
    getcontext(&_ctx);
    if (!stack_size) stack_size = 128 * 1024;  // default stack size
    _stack.reset(new char[stack_size]);
    _ctx.uc_stack.ss_sp = _stack.get();
    _ctx.uc_stack.ss_size = stack_size;
    _ctx.uc_stack.ss_flags = 0;
    _ctx.uc_link = nullptr;
    makecontext(&_ctx, (void (*)(void))fn, 1, arg);
  }
#else
#error Please define "AWACORN_USE_UCONTEXT" or "AWACORN_USE_BOOST".
#endif
 public:
  context(const context&) = delete;
  // await implementation
  template <typename T>
  T operator>>(const promise<T>& value) {
    if (_status != detail::_async_state_t::Active)
      throw std::bad_function_call();
    _status = detail::_async_state_t::Awaiting;
    _result = value.then([](const T& v) { return any(v); });
    resume();
    if (_failbit) {
      _failbit = false;
      std::rethrow_exception(any_cast<std::exception_ptr>(_result));
    }
    return any_cast<T>(_result);
  }
  void operator>>(const promise<void>& value) {
    if (_status != detail::_async_state_t::Active)
      throw std::bad_function_call();
    _status = detail::_async_state_t::Awaiting;
    _result = value.then([]() { return any(); });
    resume();
    if (_failbit) {
      _failbit = false;
      std::rethrow_exception(any_cast<std::exception_ptr>(_result));
    }
  }

 private:
  void resume() {
#if defined(AWACORN_USE_BOOST)
    if (_push_or_pull) {
      _push_or_pull = false;
      (*_pushfn)();
    } else {
      _push_or_pull = true;
      _pullfn();
    }
#elif defined(AWACORN_USE_UCONTEXT)
    ucontext_t orig = _ctx;
    swapcontext(&_ctx, &orig);
#else
#error Please define "AWACORN_USE_UCONTEXT" or "AWACORN_USE_BOOST".
#endif
  }
  detail::_async_state_t _status;
  bool _failbit;
  any _result;
#if defined(AWACORN_USE_BOOST)
  boost::coroutines2::coroutine<void>::pull_type _pullfn;
  boost::coroutines2::coroutine<void>::push_type* _pushfn;
  bool _push_or_pull;
#elif defined(AWACORN_USE_UCONTEXT)
  ucontext_t _ctx;
  std::unique_ptr<char, void (*)(char*)> _stack;
#else
#error Please define "AWACORN_USE_UCONTEXT" or "AWACORN_USE_BOOST".
#endif
  template <typename T>
  friend struct detail::_async_fn;
  template <typename T>
  friend struct detail::_basic_async_fn;
};
namespace detail {
/**
 * @brief 生成器成员，用于实现智能生命周期。
 *
 * @tparam Fn 函数类型。
 * @tparam Context 上下文类型。
 */
template <typename Fn>
struct _basic_async_fn {
 protected:
  context ctx;
  function<Fn> fn;
  template <typename U>
  _basic_async_fn(U&& fn, void (*run_fn)(void*), void* args,
                  size_t stack_size = 0)
      : ctx(run_fn, args, stack_size), fn(std::forward<U>(fn)) {}
  _basic_async_fn(const _basic_async_fn& v) = delete;
};
/**
 * @brief 不中断值的异步生成器。
 *
 * @tparam RetType 返回类型。
 */
template <typename RetType>
struct _async_fn : public _basic_async_fn<RetType(context&)>,
                   public std::enable_shared_from_this<_async_fn<RetType>> {
  explicit _async_fn(const _async_fn& v) = delete;
  promise<RetType> next() {
    if (this->ctx._status == _async_state_t::Pending) {
      this->ctx._status = _async_state_t::Active;
      this->ctx.resume();
      if (this->ctx._status == _async_state_t::Awaiting) {
        std::shared_ptr<_async_fn> ref = this->shared_from_this();
        promise<RetType> pm;
        promise<any> tmp = any_cast<promise<any>>(this->ctx._result);
        tmp.then([ref, pm](std::exception_ptr res) {
             ref->ctx._result = res;
             ref->_await_next()
                 .then([pm](const RetType& res) { pm.resolve(res); })
                 .error(
                     [pm](const std::exception_ptr& err) { pm.reject(err); });
           })
            .error([ref, pm](const std::exception_ptr& err) {
              ref->ctx._result = err;
              ref->ctx._failbit = true;
              ref->_await_next()
                  .then([pm](const RetType& res) { pm.resolve(res); })
                  .error(
                      [pm](const std::exception_ptr& err) { pm.reject(err); });
            });
        return pm;
      } else if (this->ctx._status == _async_state_t::Returned) {
        return resolve(any_cast<RetType>(this->ctx._result));
      }
      return reject<RetType>(any_cast<std::exception_ptr>(this->ctx._result));
    } else if (this->ctx._status == _async_state_t::Returned) {
      return resolve(any_cast<RetType>(this->ctx._result));
    }
    return reject<RetType>(any_cast<std::exception_ptr>(this->ctx._result));
  }
  template <typename... Args>
  static inline std::shared_ptr<_async_fn> create(Args&&... args) {
    return std::shared_ptr<_async_fn>(
        new _async_fn(std::forward<Args>(args)...));
  }

 private:
  inline promise<RetType> _await_next() {
    this->ctx._status = _async_state_t::Pending;
    return this->next();
  }
  template <typename U>
  explicit _async_fn(U&& fn, size_t stack_size = 0)
      : _basic_async_fn<RetType(context&)>(
            std::forward<U>(fn), (void (*)(void*))run_fn, this, stack_size) {}
  static void run_fn(_async_fn* self) {
    try {
      self->ctx._result = self->fn(self->ctx);
      self->ctx._status = _async_state_t::Returned;
    } catch (...) {
      self->ctx._result = std::current_exception();
      self->ctx._status = _async_state_t::Throwed;
    }
    self->ctx.resume();
  }
};
/**
 * @brief 不中断值也不返回值的异步生成器。
 */
template <>
struct _async_fn<void> : public _basic_async_fn<void(context&)>,
                         public std::enable_shared_from_this<_async_fn<void>> {
  explicit _async_fn(const _async_fn& v) = delete;
  promise<void> next() {
    if (this->ctx._status == _async_state_t::Pending) {
      this->ctx._status = _async_state_t::Active;
      this->ctx.resume();
      if (this->ctx._status == _async_state_t::Awaiting) {
        std::shared_ptr<_async_fn> ref = this->shared_from_this();
        promise<void> pm;
        promise<any> tmp = any_cast<promise<any>>(this->ctx._result);
        tmp.then([ref, pm](const any& res) {
             ref->ctx._result = res;
             ref->_await_next()
                 .then([pm]() { pm.resolve(); })
                 .error(
                     [pm](const std::exception_ptr& err) { pm.reject(err); });
           })
            .error([ref, pm](const std::exception_ptr& err) {
              ref->ctx._result = err;
              ref->ctx._failbit = true;
              ref->_await_next()
                  .then([pm]() { pm.resolve(); })
                  .error(
                      [pm](const std::exception_ptr& err) { pm.reject(err); });
            });
        return pm;
      } else if (this->ctx._status == _async_state_t::Returned) {
        return resolve();
      }
      return reject<void>(any_cast<std::exception_ptr>(this->ctx._result));
    } else if (this->ctx._status == _async_state_t::Returned) {
      return resolve();
    }
    return reject<void>(any_cast<std::exception_ptr>(this->ctx._result));
  }
  template <typename... Args>
  static inline std::shared_ptr<_async_fn> create(Args&&... args) {
    return std::shared_ptr<_async_fn>(
        new _async_fn(std::forward<Args>(args)...));
  }

 private:
  inline promise<void> _await_next() {
    this->ctx._status = _async_state_t::Pending;
    return this->next();
  }
  template <typename U>
  explicit _async_fn(U&& fn, size_t stack_size = 0)
      : _basic_async_fn<void(context&)>(
            std::forward<U>(fn), (void (*)(void*))run_fn, this, stack_size) {}
  static void run_fn(_async_fn* self) {
    try {
      self->fn(self->ctx);
      self->ctx._status = _async_state_t::Returned;
    } catch (...) {
      self->ctx._result = std::current_exception();
      self->ctx._status = _async_state_t::Throwed;
    }
    self->ctx.resume();
  }
};
};  // namespace detail
template <typename U>
auto async(U&& fn, size_t stack_size = 0)
    -> promise<decltype(fn(std::declval<context&>()))> {
  return detail::_async_fn<decltype(fn(std::declval<context&>()))>::create(
             std::forward<U>(fn), stack_size)
      ->next();
}
};  // namespace awacorn
#endif
#endif
