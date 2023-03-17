#ifndef _AWACORN_H
#define _AWACORN_H
/**
 * Project Awacorn 基于 MIT 协议开源。
 * Copyright(c) 凌 2022.
 */
#include <chrono>
#include <functional>
#include <list>
#include <thread>

namespace Awacorn {
template <typename T>
struct _Event {
  /**
   * @brief 事件类型。
   */
  typedef std::function<void(class EventLoop*, const T*)> Fn;
  /**
   * @brief 事件回调函数。
   */
  Fn fn;
  /**
   * @brief 对于 Interval 是循环间隔，对于 Event 无效。
   */
  std::chrono::high_resolution_clock::duration timeout;
  explicit _Event(const Fn& fn,
                  const std::chrono::high_resolution_clock::duration& timeout)
      : fn(fn), timeout(timeout) {}
};
/**
 * @brief 事件的标识。
 */
class Event : public _Event<Event> {
  using _Event<Event>::_Event;
};
/**
 * @brief 循环事件的标识。
 */
struct Interval : public _Event<Interval> {
  /**
   * @brief 上一次的事件标识。若 Interval 还未执行，则 ev 为 nullptr。
   */
  const Event* ev;
  /**
   * @brief 若 pending 为 false，则代表此 Interval 已经加入事件循环，否则此
   * Interval 仍未被加入事件循环。
   */
  bool pending;
  explicit Interval(const Fn& fn,
                    const std::chrono::high_resolution_clock::duration& timeout)
      : _Event<Interval>(fn, timeout), ev(nullptr), pending(true) {}
};
/**
 * @brief 事件循环。
 */
typedef class EventLoop {
  std::list<Event> _event;
  std::list<Interval> _intv;
  void _inst_intv() {
    if (_intv.empty()) return;
    for (std::list<Interval>::iterator it = _intv.begin(); it != _intv.end();
         it++) {
      if (it->pending) {
        Interval::Fn fn = it->fn;
        Interval* ptr = &(*it);
        it->pending = false;
        it->ev = create(
            [ptr](EventLoop* ev, const Event*) -> void {
              for (std::list<Interval>::iterator it = ev->_intv.begin();
                   it != ev->_intv.end(); it++) {
                if (&(*it) == ptr) {
                  it->pending = true;
                  it->fn(ev, ptr);
                  break;
                }
              }
            },
            it->timeout);
      }
    }
  }
  void _execute() {
    std::list<Event>::iterator min = _event.end();
    for (std::list<Event>::iterator it = _event.begin(); it != _event.end();
         it++) {
      if (min == _event.end() || it->timeout < min->timeout) min = it;
    }
    if (min != _event.cend()) {
      std::chrono::high_resolution_clock::duration duration = min->timeout;
      if (duration != std::chrono::high_resolution_clock::duration(0)) {
        std::this_thread::sleep_for(duration);
      }
      for (std::list<Event>::iterator it = _event.begin(); it != _event.end();
           it++)
        it->timeout = it->timeout > duration
                          ? (it->timeout - duration)
                          : std::chrono::high_resolution_clock::duration(0);
      std::chrono::high_resolution_clock::time_point st =
          std::chrono::high_resolution_clock::now();
      min->fn(this, &(*min)), _event.erase(min);
      duration = std::chrono::high_resolution_clock::now() - st;
      for (std::list<Event>::iterator it = _event.begin(); it != _event.end();
           it++)
        it->timeout = it->timeout > duration
                          ? (it->timeout - duration)
                          : std::chrono::high_resolution_clock::duration(0);
    }
  }
  template <typename T>
  static void _clear(const T* task, std::list<T>* list) noexcept {
    for (typename std::list<T>::const_iterator it = list->cbegin();
         it != list->cend(); it++) {
      if (&(*it) == task) {
        list->erase(it);
        return;
      }
    }
  }
  template <typename T>
  static const T* _create(const T& obj, std::list<T>* list) {
    return list->push_back(obj), (const T*)&list->back();
  }

 public:
  /**
   * @brief 创建定时事件。
   *
   * @param fn 事件函数。
   * @param tm 指定事件触发的时间。
   * @return const Event* 事件的标识，可用于clear。
   */
  template <typename Rep, typename Period>
  const Event* create(const Event::Fn& fn,
                      const std::chrono::duration<Rep, Period>& tm) {
    return _create<Event>(
        Event(fn, std::chrono::duration_cast<
                      std::chrono::high_resolution_clock::duration>(tm)),
        &_event);
  }
  /**
   * @brief 创建循环事件。
   *
   * @param fn 事件函数。
   * @param tm 指定事件触发的时间。
   * @return const Interval* 事件的标识，可用于clear。
   */
  template <typename Rep, typename Period>
  const Interval* create(const Interval::Fn& fn,
                         const std::chrono::duration<Rep, Period>& tm) {
    return _create<Interval>(
        Interval(fn, std::chrono::duration_cast<
                         std::chrono::high_resolution_clock::duration>(tm)),
        &_intv);
  }
  /**
   * @brief 删除即将发生的事件。
   *
   * @param task 事件标识。
   */
  void clear(const Event* task) noexcept { _clear<Event>(task, &_event); }
  /**
   * @brief 删除循环事件。已经被放入队列中的事件无法取消。
   *
   * @param task 循环事件标识。
   */
  void clear(const Interval* task) noexcept {
    if (task->ev && (!task->pending)) clear(task->ev);
    _clear<Interval>(task, &_intv);
  }
  /**
   * @brief 异步函数语法糖。运行指定的 AsyncFn 异步函数。
   *
   * @tparam Ret 函数的返回值类型。
   * @param fn 异步函数。
   * @return Ret 函数的返回值。
   */
  template <typename Ret>
  inline Ret run(const std::function<Ret(EventLoop* ev)>& fn) {
    return fn(this);
  }
  /**
   * @brief 运行事件循环。此函数将在所有事件都运行完成之后返回。
   */
  void start() {
    while (1) {
      _inst_intv(), _execute();
      if (_event.empty() && _intv.empty()) break;
    }
  }
  EventLoop() = default;
} EventLoop;
template <typename Ret>
using AsyncFn = std::function<Ret(EventLoop*)>;
};  // namespace Awacorn
#endif
