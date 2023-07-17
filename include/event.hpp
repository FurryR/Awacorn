#ifndef _AWACORN_EVENT_
#define _AWACORN_EVENT_
#if __cplusplus >= 201101L
/**
 * Project Awacorn 基于 MIT 协议开源。
 * Copyright(c) 凌 2023.
 */
#include <chrono>
#include <list>
#include <thread>

#include "detail/capture.hpp"
#include "detail/function.hpp"

namespace awacorn {
class event_loop;
class task_t {
  /**
   * @brief 事件的标识。
   */
  class event {
    /**
     * @brief 事件类型。
     */
    using fn_t = detail::function<void()>;
    /**
     * @brief 事件回调函数。
     */
    fn_t fn;
    /**
     * @brief 对于 Interval 是循环间隔，对于 Event 无效。
     */
    std::chrono::steady_clock::duration timeout;
    /**
     * @brief 用于指定一个事件是一次性事件还是循环事件。
     */
    bool interval;

   public:
    template <typename U>
    explicit event(U&& fn, const std::chrono::steady_clock::duration& timeout,
                   bool interval)
        : fn(std::forward<U>(fn)), timeout(timeout), interval(interval) {}
    event(const event&) = delete;
    event(event&& v)
        : fn(std::move(v.fn)), timeout(v.timeout), interval(v.interval) {}
    event& operator=(const event&) = delete;
    event& operator=(event&& rhs) {
      fn = std::move(rhs.fn);
      timeout = rhs.timeout;
      interval = rhs.interval;
      return *this;
    }
    friend class awacorn::event_loop;
  };
  std::list<event>::const_iterator it;
  task_t(std::list<event>::const_iterator it) : it(it) {}

 public:
  friend class event_loop;
};
/**
 * @brief 默认 yield 实现。
 *
 * @param tm 预期可用时间。
 */
inline void yield_for(const std::chrono::steady_clock::duration& tm) noexcept {
  if (tm != std::chrono::steady_clock::duration(0)) {
    std::this_thread::sleep_for(tm);
  }
}
/**
 * @brief 事件循环。
 */
class event_loop {
  std::list<task_t::event> _event;
  std::list<task_t::event>::iterator _current;
  detail::function<void(const std::chrono::steady_clock::duration&)> _yield;
  bool _execute() {
    if (!_event.empty()) {
      auto min = _event.cend();
      for (auto it = _event.cbegin();
           it != _event.cend(); it++) {
        if (min == _event.cend() || it->timeout < min->timeout) min = it;
      }
      auto tm =
          std::chrono::steady_clock::now();
      if (min->timeout > std::chrono::steady_clock::duration(0))
        _yield(min->timeout);
      auto duration =
          std::chrono::steady_clock::now() - tm;
      for (auto it = _event.begin();
           it != _event.end(); it++) {
        it->timeout = it->timeout > duration
                          ? (it->timeout - duration)
                          : std::chrono::steady_clock::duration(0);
      }
      tm = std::chrono::steady_clock::now();
      for (auto it = _event.begin();
           it != _event.end();) {
        if (it->timeout == std::chrono::steady_clock::duration(0)) {
          _current = it;
          it->fn();
          if (!it->interval) {
            it = _event.erase(it);
            continue;
          }
        }
        it++;
      }
      duration = std::chrono::steady_clock::now() - tm;
      for (auto it = _event.begin();
           it != _event.end(); it++) {
        it->timeout = it->timeout > duration
                          ? (it->timeout - duration)
                          : std::chrono::steady_clock::duration(0);
      }
      return true;
    }
    _yield(std::chrono::steady_clock::duration(0));
    return !_event.empty();
  }
  template <typename... Args>
  static inline task_t _create(std::list<task_t::event>* list, Args&&... args) {
    return list->emplace_back(std::forward<Args>(args)...),
           task_t(--list->cend());
  }

 public:
  /**
   * @brief 设置 yield 实现。
   *
   * @tparam U 新 yield 实现的类型。
   * @param impl yield 实现的对象，可以为仿函数对象或者函数指针。
   */
  template <typename U>
  inline void set_yield(U&& impl) noexcept {
    _yield = detail::function<void(const std::chrono::steady_clock::duration&)>(
        std::forward<U>(impl));
  }
  /**
   * @brief 获取当前的事件。
   *
   * @return task_t
   * 当前正在执行的事件。若没有，返回 nullptr。
   */
  inline task_t current() const noexcept { return task_t(_current); }
  /**
   * @brief 创建定时事件。
   *
   * @param fn 事件函数。
   * @param tm 指定事件触发的时间。
   * @return task_t 事件的标识，可用于clear。
   */
  template <typename Rep, typename Period, typename U>
  inline task_t event(U&& fn, const std::chrono::duration<Rep, Period>& tm) {
    return _create(
        &_event, std::forward<U>(fn),
        std::chrono::duration_cast<std::chrono::steady_clock::duration>(tm),
        false);
  }
  /**
   * @brief 创建循环事件。
   *
   * @param fn 事件函数。
   * @param tm 指定事件触发的时间。
   * @return task_t 事件的标识，可用于clear。
   */
  template <typename Rep, typename Period, typename U>
  inline task_t interval(U&& fn, const std::chrono::duration<Rep, Period>& tm) {
    auto arg_fn = detail::capture(std::forward<U>(fn));
    return _create(
        &_event,
        [this, arg_fn, tm]() mutable {
          arg_fn.borrow()();
          _current->timeout = tm;
        },
        std::chrono::duration_cast<std::chrono::steady_clock::duration>(tm),
        true);
  }
  /**
   * @brief 删除即将发生的事件。
   *
   * @param task 事件标识。
   */
  void clear(task_t task) {
    if (task.it == _event.cend()) return;
    if (task.it == _current)
      _current->interval = false;
    else
      _event.erase(task.it);
  }
  /**
   * @brief 运行事件循环。此函数将在所有事件都运行完成之后返回。
   */
  inline void start() {
    while (_execute())
      ;
  }
  event_loop() : _yield(yield_for) {}
  template <typename U>
  event_loop(U&& yield_impl) : _yield(std::forward<U>(yield_impl)) {}
  event_loop(const event_loop&) = delete;
  event_loop& operator=(const event_loop&) = delete;
};
};  // namespace awacorn
#endif
#endif
