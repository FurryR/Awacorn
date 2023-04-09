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
    std::chrono::high_resolution_clock::duration timeout;
    /**
     * @brief 如果 keep 设置为 true，那么将会保留这个事件。
     */
    bool keep;

   public:
    template <typename U>
    explicit event(U&& fn,
                   const std::chrono::high_resolution_clock::duration& timeout,
                   bool keep)
        : fn(std::forward<U>(fn)), timeout(timeout), keep(keep) {}
    event(const event&) = delete;
    event(event&& v) : fn(std::move(v.fn)), timeout(v.timeout), keep(v.keep) {}
    event& operator=(event&& rhs) {
      fn = std::move(rhs.fn);
      timeout = rhs.timeout;
      keep = rhs.keep;
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
 * @brief 事件循环。
 */
class event_loop {
  std::list<task_t::event> _event;
  std::list<task_t::event>::iterator _current;
  void _execute() {
    std::list<task_t::event>::iterator min = _event.end();
    for (std::list<task_t::event>::iterator it = _event.begin();
         it != _event.end(); it++) {
      if (min == _event.end() || it->timeout < min->timeout) min = it;
    }
    if (min != _event.cend()) {
      std::chrono::high_resolution_clock::duration duration = min->timeout;
      if (duration != std::chrono::high_resolution_clock::duration(0)) {
        std::this_thread::sleep_for(duration);
      }
      for (std::list<task_t::event>::iterator it = _event.begin();
           it != _event.end(); it++)
        it->timeout = it->timeout > duration
                          ? (it->timeout - duration)
                          : std::chrono::high_resolution_clock::duration(0);
      std::chrono::high_resolution_clock::time_point st =
          std::chrono::high_resolution_clock::now();
      _current = min;
      min->fn();
      if (!min->keep) _event.erase(min);
      _current = _event.end();
      duration = std::chrono::high_resolution_clock::now() - st;
      for (std::list<task_t::event>::iterator it = _event.begin();
           it != _event.end(); it++)
        it->timeout = it->timeout > duration
                          ? (it->timeout - duration)
                          : std::chrono::high_resolution_clock::duration(0);
    }
  }
  template <typename... Args>
  static task_t _create(std::list<task_t::event>* list, Args&&... args) {
    return list->emplace_back(std::forward<Args>(args)...),
           task_t(--list->cend());
  }

 public:
  /**
   * @brief 获取当前的事件。
   *
   * @return task_t
   * 当前正在执行的事件。若没有，返回 nullptr。
   */
  task_t current() const noexcept { return task_t(_current); }
  /**
   * @brief 创建定时事件。
   *
   * @param fn 事件函数。
   * @param tm 指定事件触发的时间。
   * @return task_t 事件的标识，可用于clear。
   */
  template <typename Rep, typename Period, typename U>
  task_t event(U&& fn, const std::chrono::duration<Rep, Period>& tm) {
    return _create(&_event, std::forward<U>(fn),
                   std::chrono::duration_cast<
                       std::chrono::high_resolution_clock::duration>(tm),
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
  task_t interval(U&& fn, const std::chrono::duration<Rep, Period>& tm) {
    return _create(
        &_event,
        [this, fn, tm]() {
          fn();
          _current->timeout = tm;
        },
        std::chrono::duration_cast<
            std::chrono::high_resolution_clock::duration>(tm),
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
      _current->keep = false;
    else
      _event.erase(task.it);
  }
  /**
   * @brief 运行事件循环。此函数将在所有事件都运行完成之后返回。
   */
  void start() {
    do {
      _execute();
    } while (!_event.empty());
  }
  event_loop() = default;
  event_loop(const event_loop&) = delete;
};
};  // namespace awacorn
#endif
#endif
