#ifndef _AWACORN_ASYNC
#define _AWACORN_ASYNC
#if __cplusplus >= 201101L
/**
 * Project Awacorn 基于 MIT 协议开源。
 * Copyright(c) 凌 2023.
 */
#if !defined(AWACORN_USE_BOOST) && !defined(AWACORN_USE_UCONTEXT)
#if __has_include(<boost/context/continuation.hpp>)
#define AWACORN_USE_BOOST
#elif __has_include(<ucontext.h>)
#define AWACORN_USE_UCONTEXT
#else
#error Neither <boost/context/continuation.hpp> nor <ucontext.h> is found.
#endif
#endif
#if defined(AWACORN_USE_BOOST)
#include <boost/context/continuation.hpp>
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
#include "detail/unsafe_any.hpp"
#include "promise.hpp"

namespace awacorn {
namespace detail {
/**
 * @brief 生成器的状态。
 */
enum class async_state_t {
  Pending = 0,   // 尚未运行
  Active = 1,    // 运行中
  Returned = 2,  // 已返回
  Awaiting = 3,  // await 中
  Throwed = 4    // 抛出错误
};
template <typename Fn>
struct basic_async_fn;
template <typename RetType>
struct async_fn;
};  // namespace detail
/**
 * @brief 生成器上下文基类。
 */
struct context {
  context() = delete;
  context(const context&) = delete;
  /**
   * @brief 等待 promise 完成并返回 promise 的结果值。
   *
   * @tparam T promise 的结果类型。
   * @param value promise 本身。
   * @return T promise 的结果。
   */
  template <typename T>
  T operator>>(const promise<T>& value) {
    if (_status != detail::async_state_t::Active)
      throw std::bad_function_call();
    _status = detail::async_state_t::Awaiting;
    _result = value.then([](const T& v) { return detail::unsafe_any(v); });
    resume();
    if (_failbit) {
      _failbit = false;
      std::rethrow_exception(detail::unsafe_cast<std::exception_ptr>(_result));
    }
    return detail::unsafe_cast<T>(_result);
  }
  void operator>>(const promise<void>& value) {
    if (_status != detail::async_state_t::Active)
      throw std::bad_function_call();
    _status = detail::async_state_t::Awaiting;
    _result = value.then([]() { return detail::unsafe_any(); });
    resume();
    if (_failbit) {
      _failbit = false;
      std::rethrow_exception(detail::unsafe_cast<std::exception_ptr>(_result));
    }
  }

 private:
#if defined(AWACORN_USE_BOOST)
  context(void (*fn)(void*), void* arg, std::size_t stack_size = 0)
      : _status(detail::async_state_t::Pending),
        _failbit(false),
        _ctx(boost::context::callcc(
            std::allocator_arg,
            boost::context::fixedsize_stack(
                stack_size ? stack_size
                           : boost::context::stack_traits::default_size()),
            [this, fn, arg](boost::context::continuation&& ctx) {
              _ctx = ctx.resume();
              fn(arg);
              return std::move(_ctx);
            })) {}
#elif defined(AWACORN_USE_UCONTEXT)
  context(void (*fn)(void*), void* arg, std::size_t stack_size = 0)
      : _status(detail::async_state_t::Pending), _stack(nullptr, [](char* ptr) {
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
  void resume() {
#if defined(AWACORN_USE_BOOST)
    _ctx = _ctx.resume();
#elif defined(AWACORN_USE_UCONTEXT)
    ucontext_t orig = _ctx;
    swapcontext(&_ctx, &orig);
#else
#error Please define "AWACORN_USE_UCONTEXT" or "AWACORN_USE_BOOST".
#endif
  }
  detail::unsafe_any _result;
  detail::async_state_t _status;
  bool _failbit;
#if defined(AWACORN_USE_BOOST)
  boost::context::continuation _ctx;
#elif defined(AWACORN_USE_UCONTEXT)
  ucontext_t _ctx;
  std::unique_ptr<char, void (*)(char*)> _stack;
#else
#error Please define "AWACORN_USE_UCONTEXT" or "AWACORN_USE_BOOST".
#endif
  template <typename T>
  friend struct detail::async_fn;
  template <typename T>
  friend struct detail::basic_async_fn;
};
namespace detail {
template <typename Fn>
struct basic_async_fn {
 protected:
  context ctx;
  function<Fn> fn;
  template <typename U>
  basic_async_fn(U&& fn, void (*run_fn)(void*), void* args,
                 std::size_t stack_size = 0)
      : ctx(run_fn, args, stack_size), fn(std::forward<U>(fn)) {}
  basic_async_fn(const basic_async_fn& v) = delete;
};
template <typename RetType>
struct async_fn : basic_async_fn<RetType(context&)>,
                  std::enable_shared_from_this<async_fn<RetType>> {
  explicit async_fn(const async_fn& v) = delete;
  promise<RetType> next() {
    if (this->ctx._status == async_state_t::Pending) {
      this->ctx._status = async_state_t::Active;
      this->ctx.resume();
      if (this->ctx._status == async_state_t::Awaiting) {
        std::shared_ptr<async_fn> ref = this->shared_from_this();
        promise<RetType> pm;
        promise<detail::unsafe_any> tmp =
            detail::unsafe_cast<promise<detail::unsafe_any>>(this->ctx._result);
        tmp.then([ref, pm](const detail::unsafe_any& res) {
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
      } else if (this->ctx._status == async_state_t::Returned) {
        return resolve(detail::unsafe_cast<RetType>(this->ctx._result));
      }
      return reject<RetType>(
          detail::unsafe_cast<std::exception_ptr>(this->ctx._result));
    } else if (this->ctx._status == async_state_t::Returned) {
      return resolve(detail::unsafe_cast<RetType>(this->ctx._result));
    }
    return reject<RetType>(
        detail::unsafe_cast<std::exception_ptr>(this->ctx._result));
  }
  template <typename... Args>
  static inline std::shared_ptr<async_fn> create(Args&&... args) {
    return std::shared_ptr<async_fn>(new async_fn(std::forward<Args>(args)...));
  }

 private:
  inline promise<RetType> _await_next() {
    this->ctx._status = async_state_t::Pending;
    return this->next();
  }
  template <typename U>
  explicit async_fn(U&& fn, std::size_t stack_size = 0)
      : basic_async_fn<RetType(context&)>(
            std::forward<U>(fn), (void (*)(void*))run_fn, this, stack_size) {}
  static void run_fn(async_fn* self) {
    try {
      self->ctx._result = self->fn(self->ctx);
      self->ctx._status = async_state_t::Returned;
    } catch (...) {
      self->ctx._result = std::current_exception();
      self->ctx._status = async_state_t::Throwed;
    }
    self->ctx.resume();
  }
};
template <>
struct async_fn<void> : basic_async_fn<void(context&)>,
                        std::enable_shared_from_this<async_fn<void>> {
  explicit async_fn(const async_fn& v) = delete;
  promise<void> next() {
    if (this->ctx._status == async_state_t::Pending) {
      this->ctx._status = async_state_t::Active;
      this->ctx.resume();
      if (this->ctx._status == async_state_t::Awaiting) {
        std::shared_ptr<async_fn> ref = this->shared_from_this();
        promise<void> pm;
        promise<detail::unsafe_any> tmp =
            detail::unsafe_cast<promise<detail::unsafe_any>>(this->ctx._result);
        tmp.then([ref, pm](const detail::unsafe_any& res) {
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
      } else if (this->ctx._status == async_state_t::Returned) {
        return resolve();
      }
      return reject<void>(
          detail::unsafe_cast<std::exception_ptr>(this->ctx._result));
    } else if (this->ctx._status == async_state_t::Returned) {
      return resolve();
    }
    return reject<void>(
        detail::unsafe_cast<std::exception_ptr>(this->ctx._result));
  }
  template <typename... Args>
  static inline std::shared_ptr<async_fn> create(Args&&... args) {
    return std::shared_ptr<async_fn>(new async_fn(std::forward<Args>(args)...));
  }

 private:
  inline promise<void> _await_next() {
    this->ctx._status = async_state_t::Pending;
    return this->next();
  }
  template <typename U>
  explicit async_fn(U&& fn, std::size_t stack_size = 0)
      : basic_async_fn<void(context&)>(
            std::forward<U>(fn), (void (*)(void*))run_fn, this, stack_size) {}
  static void run_fn(async_fn* self) {
    try {
      self->fn(self->ctx);
      self->ctx._status = async_state_t::Returned;
    } catch (...) {
      self->ctx._result = std::current_exception();
      self->ctx._status = async_state_t::Throwed;
    }
    self->ctx.resume();
  }
};
};  // namespace detail
/**
 * @brief 进入异步函数上下文。
 *
 * @tparam U 函数类型。
 * @param fn 函数。
 * @param stack_size 可选，栈的大小(字节, 如果可用)。在部分架构上可能要求对齐。
 * @return promise<decltype(fn(std::declval<context&>()))> 用于取得函数返回值的
 * promise 对象。
 */
template <typename U>
auto async(U&& fn, std::size_t stack_size = 0)
    -> promise<decltype(fn(std::declval<context&>()))> {
  return detail::async_fn<decltype(fn(std::declval<context&>()))>::create(
             std::forward<U>(fn), stack_size)
      ->next();
}
};  // namespace awacorn
#endif
#endif
