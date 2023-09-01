#ifndef _AWACORN_ASYNC
#define _AWACORN_ASYNC
#if __cplusplus >= 201101L
/**
 * Project Awacorn 基于 MIT 协议开源。
 * Copyright(c) 凌 2023.
 */

#include <memory>
#include <stdexcept>
#include <typeinfo>
#include <vector>

#include "detail/context.hpp"
#include "detail/function.hpp"
#include "detail/unsafe_any.hpp"
#include "promise.hpp"

namespace awacorn {
namespace detail {
/**
 * @brief 生成器的状态。
 */
enum class async_state_t {
  pending = 0,   // 尚未运行
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
  context& operator=(const context&) = delete;
  /**
   * @brief 等待 promise 完成并返回 promise 的结果值。
   *
   * @tparam T promise 的结果类型。
   * @param value promise 本身。
   * @return T promise 的结果。
   */
  template <typename T>
  T&& operator>>(const promise<T>& value) {
    if (_status != detail::async_state_t::Active)
      throw std::bad_function_call();
    _status = detail::async_state_t::Awaiting;
    _result = value.then([](const T& v) { return detail::unsafe_any(v); });
    resume();
    if (_failbit) {
      _failbit = false;
      std::rethrow_exception(detail::unsafe_cast<std::exception_ptr>(_result));
    }
    return detail::unsafe_cast<T>(std::move(_result));
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
  context(void (*fn)(void*), void* arg, std::size_t stack_size = 0)
      : _status(detail::async_state_t::pending),
        _ctx(fn, arg, stack_size),
        _failbit(false) {}
  inline void resume() { _ctx.resume(); }
  detail::unsafe_any _result;
  detail::async_state_t _status;
  detail::basic_context _ctx;
  bool _failbit;
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
  basic_async_fn(const basic_async_fn&) = delete;
  basic_async_fn& operator=(const basic_async_fn&) = delete;
};
template <typename RetType>
struct async_fn : basic_async_fn<RetType(context&)>,
                  std::enable_shared_from_this<async_fn<RetType>> {
  explicit async_fn(const async_fn&) = delete;
  promise<RetType> next() {
    if (this->ctx._status == async_state_t::pending) {
      this->ctx._status = async_state_t::Active;
      this->ctx.resume();
      if (this->ctx._status == async_state_t::Awaiting) {
        auto ref = this->shared_from_this();
        promise<RetType> pm;
        auto tmp = std::move(detail::unsafe_cast<promise<detail::unsafe_any>>(
            std::move(this->ctx._result)));
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
        return resolve(std::move(
            detail::unsafe_cast<RetType>(std::move(this->ctx._result))));
      }
      return reject<RetType>(std::move(detail::unsafe_cast<std::exception_ptr>(
          std::move(this->ctx._result))));
    } else if (this->ctx._status == async_state_t::Returned) {
      return resolve(std::move(
          detail::unsafe_cast<RetType>(std::move(this->ctx._result))));
    }
    return reject<RetType>(std::move(
        detail::unsafe_cast<std::exception_ptr>(std::move(this->ctx._result))));
  }
  template <typename... Args>
  static inline std::shared_ptr<async_fn> create(Args&&... args) {
    return std::shared_ptr<async_fn>(new async_fn(std::forward<Args>(args)...));
  }

 private:
  inline promise<RetType> _await_next() {
    this->ctx._status = async_state_t::pending;
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
  explicit async_fn(const async_fn&) = delete;
  async_fn& operator=(const async_fn&) = delete;
  promise<void> next() {
    if (this->ctx._status == async_state_t::pending) {
      this->ctx._status = async_state_t::Active;
      this->ctx.resume();
      if (this->ctx._status == async_state_t::Awaiting) {
        auto ref = this->shared_from_this();
        promise<void> pm;
        auto tmp = std::move(detail::unsafe_cast<promise<detail::unsafe_any>>(
            std::move(this->ctx._result)));
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
      return reject<void>(std::move(detail::unsafe_cast<std::exception_ptr>(
          std::move(this->ctx._result))));
    } else if (this->ctx._status == async_state_t::Returned) {
      return resolve();
    }
    return reject<void>(std::move(
        detail::unsafe_cast<std::exception_ptr>(std::move(this->ctx._result))));
  }
  template <typename... Args>
  static inline std::shared_ptr<async_fn> create(Args&&... args) {
    return std::shared_ptr<async_fn>(new async_fn(std::forward<Args>(args)...));
  }

 private:
  inline promise<void> _await_next() {
    this->ctx._status = async_state_t::pending;
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
