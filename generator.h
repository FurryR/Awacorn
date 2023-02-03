#ifndef _GENERATOR_H
#define _GENERATOR_H
#ifdef __APPLE__
#define _XOPEN_SOURCE
#endif

#include <ucontext.h>

#include <functional>
#include <memory>
#include <typeinfo>
#include <vector>

#include "./promise.h"

namespace Generator {
/**
 * @brief 类型安全的最小 any 实现，用于 await。
 */
typedef class __Any {
  typedef struct Base {
    virtual ~Base() = default;
    virtual std::unique_ptr<Base> clone() const = 0;
  } Base;
  template <typename T>
  struct Derived : Base {
    T data;
    virtual std::unique_ptr<Base> clone() const override {
      return std::unique_ptr<Base>(new Derived<T>(data));
    }
    Derived(const T& data) : data(data) {}
    ~Derived() = default;
  };
  std::unique_ptr<Base> ptr;

 public:
  __Any() = default;
  template <typename T>
  __Any(const T& val) : ptr(std::unique_ptr<Base>(new Derived<T>(val))) {}
  __Any(const __Any& val) : ptr(val.ptr ? val.ptr->clone() : nullptr) {}
  __Any& operator=(const __Any& rhs) { return *new (this) __Any(rhs); }
  template <typename T>
  friend const T& __Any_cast(const __Any&);
} __Any;
template <typename T>
const T& __Any_cast(const __Any& val) {
  __Any::Derived<T>* ptr = dynamic_cast<__Any::Derived<T>*>(val.ptr.get());
  if (ptr) return ptr->data;
  throw std::bad_cast();
}

/**
 * @brief 表示返回或中断值。
 *
 * @tparam RetType 返回的类型
 * @tparam YieldType 中断的类型
 */
template <typename RetType, typename YieldType>
struct Result {
  /**
   * @brief 结果的类型。
   */
  typedef enum ValueType {
    Null = 0,    // 空
    Return = 1,  // 返回类型
    Yield = 2    // 中断类型
  } ValueType;
  /**
   * @brief 获得返回值。类型不正确则抛出 std::bad_cast 错误。
   *
   * @return const RetType& 返回值的只读引用。
   */
  const RetType& ret() const {
    if (_type != Return) throw std::bad_cast();
    return *(const RetType*)_val;
  }
  /**
   * @brief 获得中断值。类型不正确则抛出 std::bad_cast 错误。
   *
   * @return const YieldType& 中断值的只读引用。
   */
  const YieldType& yield() const {
    if (_type != Yield) throw std::bad_cast();
    return *(const YieldType*)_val;
  }
  /**
   * @brief 获得结果类型。
   *
   * @return ValueType 结果类型。
   */
  ValueType type() const noexcept { return _type; }
  static Result<RetType, YieldType> generate_yield(
      const YieldType& value) {
    Result<RetType, YieldType> tmp;
    tmp._type = Yield;
    *(YieldType*)tmp._val = value;
    return tmp;
  }
  static Result<RetType, YieldType> generate_ret(const RetType& value) {
    Result<RetType, YieldType> tmp;
    tmp._type = Return;
    *(RetType*)tmp._val = value;
    return tmp;
  }
  Result() : _type(Null) {}
  Result(const Result<RetType, YieldType>& val) : _type(val._type) {
    if (val._type == Return) {
      *(RetType*)_val = *(const RetType*)val._val;
    } else if (val._type == Yield) {
      *(YieldType*)_val = *(const YieldType*)val._val;
    }
  }
  Result& operator=(const Result& rhs) {
    return *new (this) Result(rhs);
  }
  ~Result() {
    if (_type == Return) {
      ((RetType*)_val)->~RetType();
    } else if (_type == Yield) {
      ((YieldType*)_val)->~YieldType();
    }
  }

 private:
  alignas(alignof(RetType) > alignof(YieldType)
              ? sizeof(RetType)
              : sizeof(YieldType)) unsigned char _val[sizeof(RetType) >
                                                              sizeof(YieldType)
                                                          ? sizeof(RetType)
                                                          : sizeof(YieldType)];
  ValueType _type;
};
/**
 * @brief 返回类型为空的返回或中断值。
 *
 * @tparam YieldType 中断类型。
 */
template <typename YieldType>
struct Result<void, YieldType> {
  /**
   * @brief 结果的类型。
   */
  typedef enum ValueType {
    Null = 0,    // 空
    Return = 1,  // 返回类型
    Yield = 2    // 中断类型
  } ValueType;
  /**
   * @brief 获得中断值。类型不正确则抛出 std::bad_cast 错误。
   *
   * @return const YieldType& 中断值的只读引用。
   */
  const YieldType& yield() const {
    if (_type != Yield) throw std::bad_cast();
    return _val;
  }
  /**
   * @brief 获得结果类型。
   *
   * @return ValueType 结果类型。
   */
  ValueType type() const noexcept { return _type; }
  static Result<void, YieldType> generate_yield(const YieldType& value) {
    Result<void, YieldType> tmp;
    tmp._type = Yield;
    tmp._val = value;
    return tmp;
  }
  static Result<void, YieldType> generate_ret() {
    Result<void, YieldType> tmp;
    tmp._type = Return;
    return tmp;
  }
  Result() : _type(Null) {}

 private:
  YieldType _val;
  ValueType _type;
};
constexpr size_t STACK_SIZE = 1024 * 128;  // 默认 128K 栈大小
/**
 * @brief 生成器上下文基类。
 */
typedef struct _BaseContext {
  _BaseContext(void (*fn)(void), void* arg)
      : _stack(std::vector<char>(STACK_SIZE)) {
    getcontext(&_ctx);
    _ctx.uc_stack.ss_sp = _stack.data();
    _ctx.uc_stack.ss_size = _stack.size();
    _ctx.uc_stack.ss_flags = 0;
    _ctx.uc_link = nullptr;
    makecontext(&_ctx, fn, 1, arg);
  }
  _BaseContext(_BaseContext&& v)
      : _ctx(std::move(v._ctx)), _stack(std::move(v._stack)) {}
  _BaseContext(const _BaseContext&) = delete;

 protected:
  void _switch_ctx() {
    ucontext_t orig = _ctx;
    swapcontext(&_ctx, &orig);
  }
  ucontext_t _ctx;
  std::vector<char> _stack;
} _BaseContext;
/**
 * @brief 生成器成员，用于实现智能生命周期。
 *
 * @tparam Fn 函数类型。
 * @tparam Context 上下文类型。
 * @tparam Status 状态类型。
 */
template <typename Fn, typename Context>
struct _BaseGenerator {
  std::function<Fn> fn;
  Context ctx;
  explicit _BaseGenerator(const std::function<Fn>& fn, void (*run_fn)(void),
                          void* arg)
      : fn(fn), ctx(run_fn, arg) {}
  explicit _BaseGenerator(_BaseGenerator&& v)
      : fn(std::move(v.fn)), ctx(std::move(v.ctx)) {}
  explicit _BaseGenerator(const _BaseGenerator& v) = delete;
};
// ctx->yield(T) implementation
template <typename RetType, typename YieldType, typename Gen>
struct _YieldImpl {
  static void apply(void (_BaseContext::*fn)(), _BaseContext* ctx,
                    Result<RetType, YieldType>* result,
                    typename Gen::Status* status, const YieldType& value) {
    if (*status != Gen::Active) throw std::bad_function_call();
    *status = Gen::Yielded;
    *result = Result<RetType, YieldType>::generate_yield(value);
    (ctx->*fn)();
  }
};
// ctx->await(Promise::Promise<T>) implementation
template <typename T, typename Gen>
struct _AwaitImpl {
  static T apply(void (_BaseContext::*fn)(), _BaseContext* ctx,
                 __Any* pm_result, typename Gen::Status* status,
                 const Promise::Promise<T>& value) {
    if (*status != Gen::Active) throw std::bad_function_call();
    *status = Gen::Awaiting;
    *pm_result = value.then([](const T& v) { return __Any(v); });
    (ctx->*fn)();
    return __Any_cast<T>(*pm_result);
  }
};
// ctx->await(Promise::Promise<void>) implementation
template <typename Gen>
struct _AwaitImpl<void, Gen> {
  static void apply(void (_BaseContext::*fn)(), _BaseContext* ctx,
                    __Any* pm_result, typename Gen::Status* status,
                    const Promise::Promise<void>& value) {
    if (*status != Gen::Active) throw std::bad_function_call();
    *status = Gen::Awaiting;
    *pm_result = value.then([]() { return __Any(); });
    (ctx->*fn)();
  }
};
/**
 * @brief 生成器。
 *
 * @tparam RetType 返回类型。
 * @tparam YieldType 中断类型。
 */
template <typename RetType, typename YieldType>
struct Generator {
  /**
   * @brief 生成器的状态。
   */
  typedef enum Status {
    Pending = 0,  // 尚未运行
    Yielded = 1,  // 已中断
    Active = 2,   // 运行中
    Returned = 3  // 已返回
  } Status;
  /**
   * @brief 生成器上下文。
   */
  typedef struct Context : public _BaseContext {
    /**
     * @brief 中断当前的生成器。若不在生成器内调用此函数则抛出
     * std::bad_function_call 错误。
     *
     * @param value 中断值。
     */
    void yield(const YieldType& value) {
      _YieldImpl<RetType, YieldType, Generator>::apply(
          &Context::_switch_ctx, this, &_result, &_status, value);
    }
    Context(void (*fn)(void), void* arg)
        : _BaseContext(fn, arg), _status(Pending) {}
    Context(Context&& v)
        : _BaseContext(std::move(v)), _status(std::move(v._status)) {}
    Context(const Context&) = delete;
    friend struct Generator;

   private:
    Status _status;
    Result<RetType, YieldType> _result;
  } Context;

 private:
  typedef _BaseGenerator<RetType(Context*), Context> _type;
  static Result<RetType, YieldType> _next(
      const std::shared_ptr<_type>& _ctx) {
    if (_ctx->ctx._status == Yielded || _ctx->ctx._status == Pending) {
      _ctx->ctx._status = Active;
      _ctx->ctx._switch_ctx();
      return _ctx->ctx._result;
    } else if (_ctx->ctx._status == Returned) {
      return _ctx->ctx._result;
    }
    throw std::bad_function_call();
  }
  static void run_fn(_type* self) {
    self->ctx._result =
        Result<RetType, YieldType>::generate_ret(self->fn(&self->ctx));
    self->ctx._status = Returned;
    setcontext(&self->ctx._ctx);
  }

  std::shared_ptr<_type> _ctx;

 public:
  /**
   * @brief 获得当前生成器的状态。
   *
   * @return Status 生成器状态。
   */
  Status status() const noexcept { return _ctx->ctx._status; }
  /**
   * @brief 获得生成器返回的下一个值。若在生成器内调用此函数则抛出
   * std::bad_function_call 错误。
   *
   * @return Result<RetType, YieldType> 结果。
   */
  Result<RetType, YieldType> next() const { return _next(_ctx); }
  /**
   * @brief 根据函数构造生成器。
   *
   * @param fn 生成器内部的函数。
   */
  explicit Generator(const decltype(_type::fn)& fn) {
    _type* ptr = (decltype(ptr)) new char[sizeof(_type)];
    new (ptr) _type(fn, (void (*)(void))run_fn, ptr);
    _ctx = std::shared_ptr<_type>(ptr);
  }
};
/**
 * @brief 不返回值的生成器。
 *
 * @tparam YieldType 中断类型。
 */
template <typename YieldType>
struct Generator<void, YieldType> {
  /**
   * @brief 生成器的状态。
   */
  typedef enum Status {
    Pending = 0,  // 尚未运行
    Yielded = 1,  // 已中断
    Active = 2,   // 运行中
    Returned = 3  // 已返回
  } Status;
  /**
   * @brief 生成器上下文。
   */
  typedef struct Context : public _BaseContext {
    /**
     * @brief 中断当前的生成器。若不在生成器内调用此函数则抛出
     * std::bad_function_call 错误。
     *
     * @param value 中断值。
     */
    void yield(const YieldType& value) {
      _YieldImpl<void, YieldType, Generator>::apply(&Context::_switch_ctx, this,
                                                    &_result, &_status, value);
    }
    Context(void (*fn)(void), void* arg)
        : _BaseContext(fn, arg), _status(Pending) {}
    Context(Context&& v)
        : _BaseContext(std::move(v)), _status(std::move(v._status)) {}
    Context(const Context&) = delete;
    friend struct Generator;

   private:
    Status _status;
    Result<void, YieldType> _result;
  } Context;

 private:
  typedef _BaseGenerator<void(Context*), Context> _type;
  static Result<void, YieldType> _next(
      const std::shared_ptr<_type>& _ctx) {
    if (_ctx->ctx._status == Yielded || _ctx->ctx._status == Pending) {
      _ctx->ctx._status = Active;
      _ctx->ctx._switch_ctx();
      return _ctx->ctx._result;
    } else if (_ctx->ctx._status == Returned) {
      return _ctx->ctx._result;
    }
    throw std::bad_function_call();
  }
  static void run_fn(_type* self) {
    self->fn(&self->ctx);
    self->ctx._status = Returned;
    setcontext(&self->ctx._ctx);
  }

  std::shared_ptr<_type> _ctx;

 public:
  /**
   * @brief 获得当前生成器的状态。
   *
   * @return Status 生成器状态。
   */
  Status status() const noexcept { return _ctx->ctx._status; }
  /**
   * @brief 获得生成器返回的下一个值。若在生成器内调用此函数则抛出
   * std::bad_function_call 错误。
   *
   * @return Result<void, YieldType> 结果。
   */
  Result<void, YieldType> next() const { return _next(_ctx); }
  /**
   * @brief 根据函数构造生成器。
   *
   * @param fn 生成器内部的函数。
   */
  explicit Generator(const decltype(_type::fn)& fn) {
    _type* ptr = (decltype(ptr)) new char[sizeof(_type)];
    new (ptr) _type(fn, (void (*)(void))run_fn, ptr);
    _ctx = std::shared_ptr<_type>(ptr);
  }
};
/**
 * @brief 异步生成器。
 *
 * @tparam RetType 返回类型。
 * @tparam YieldType 中断类型。
 */
template <typename RetType, typename YieldType = void>
struct AsyncGenerator {
  /**
   * @brief 生成器的状态。
   */
  typedef enum Status {
    Pending = 0,   // 尚未运行
    Yielded = 1,   // 已中断
    Active = 2,    // 运行中
    Returned = 3,  // 已返回
    Awaiting = 4,  // await 中
  } Status;
  /**
   * @brief 生成器上下文。
   */
  typedef struct Context : public _BaseContext {
    /**
     * @brief 中断当前的生成器。若不在生成器内调用此函数则抛出
     * std::bad_function_call 错误。
     *
     * @param value 中断值。
     */
    void yield(const YieldType& value) {
      _YieldImpl<RetType, YieldType, AsyncGenerator>::apply(
          &Context::_switch_ctx, this, &_result, &_status, value);
    }
    /**
     * @brief 等待一个 Promise 完成，并取得 Promise
     * 的值。若不在生成器内调用此函数则抛出 std::bad_function_call 错误。
     *
     * @tparam T Promise 内部元素的类型。
     * @param value Promise 本身。
     * @return T Promise 的值。
     */
    template <typename T>
    T await(const Promise::Promise<T>& value) {
      return _AwaitImpl<T, AsyncGenerator>::apply(&Context::_switch_ctx, this,
                                                  &_pm_result, &_status, value);
    }
    Context(void (*fn)(void), void* arg)
        : _BaseContext(fn, arg), _status(Pending) {}
    Context(Context&& v)
        : _BaseContext(std::move(v)),
          _status(std::move(v._status)),
          _pm_result(std::move(v._pm_result)) {}
    Context(const Context&) = delete;
    friend struct AsyncGenerator;

   private:
    Status _status;
    __Any _pm_result;
    Result<RetType, YieldType> _result;
  } Context;

 private:
  typedef _BaseGenerator<RetType(Context*), Context> _type;
  static Promise::Promise<Result<RetType, YieldType>> _next(
      const std::shared_ptr<_type>& _ctx) {
    if (_ctx->ctx._status == Yielded || _ctx->ctx._status == Pending ||
        _ctx->ctx._status == Awaiting) {
      _ctx->ctx._status = Active;
      _ctx->ctx._switch_ctx();
      if (_ctx->ctx._status == Awaiting) {
        std::shared_ptr<_type> tmp = _ctx;
        return __Any_cast<Promise::Promise<__Any>>(_ctx->ctx._pm_result)
            .then([tmp](const __Any& v) {
              tmp->ctx._pm_result = v;
              return _next(tmp);
            });
      }
      return Promise::resolve(_ctx->ctx._result);
    } else if (_ctx->ctx._status == Returned) {
      return Promise::resolve(_ctx->ctx._result);
    }
    throw std::bad_function_call();
  }
  static void run_fn(_type* self) {
    self->ctx._result =
        Result<RetType, YieldType>::generate_ret(self->fn(&self->ctx));
    self->ctx._status = Returned;
    setcontext(&self->ctx._ctx);
  }

  std::shared_ptr<_type> _ctx;

 public:
  /**
   * @brief 获得当前生成器的状态。
   *
   * @return Status 生成器状态。
   */
  Status status() const noexcept { return _ctx->ctx._status; }
  /**
   * @brief 获得生成器返回的下一个值。若在生成器内调用此函数则抛出
   * std::bad_function_call 错误。
   *
   * @return Promise::Promise<Result<RetType, YieldType>> 结果。
   */
  Promise::Promise<Result<RetType, YieldType>> next() const {
    return _next(_ctx);
  }
  /**
   * @brief 根据函数构造生成器。
   *
   * @param fn 生成器内部的函数。
   */
  explicit AsyncGenerator(const decltype(_type::fn)& fn) {
    _type* ptr = (decltype(ptr)) new char[sizeof(_type)];
    new (ptr) _type(fn, (void (*)(void))run_fn, ptr);
    _ctx = std::shared_ptr<_type>(ptr);
  }
};
/**
 * @brief 不返回值的异步生成器。
 *
 * @tparam YieldType 中断类型。
 */
template <typename YieldType>
struct AsyncGenerator<void, YieldType> {
  /**
   * @brief 生成器的状态。
   */
  typedef enum Status {
    Pending = 0,   // 尚未运行
    Yielded = 1,   // 已中断
    Active = 2,    // 运行中
    Returned = 3,  // 已返回
    Awaiting = 4,  // await 中
  } Status;
  /**
   * @brief 生成器上下文。
   */
  typedef struct Context : public _BaseContext {
    /**
     * @brief 中断当前的生成器。若不在生成器内调用此函数则抛出
     * std::bad_function_call 错误。
     *
     * @param value 中断值。
     */
    void yield(const YieldType& value) {
      _YieldImpl<void, YieldType, AsyncGenerator>::apply(
          &Context::_switch_ctx, this, &_result, &_status, value);
    }
    /**
     * @brief 等待一个 Promise 完成，并取得 Promise
     * 的值。若不在生成器内调用此函数则抛出 std::bad_function_call 错误。
     *
     * @tparam T Promise 内部元素的类型。
     * @param value Promise 本身。
     * @return T Promise 的值。
     */
    template <typename T>
    T await(const Promise::Promise<T>& value) {
      return _AwaitImpl<T, AsyncGenerator>::apply(&Context::_switch_ctx, this,
                                                  &_pm_result, &_status, value);
    }
    Context(void (*fn)(void), void* arg)
        : _BaseContext(fn, arg), _status(Pending) {}
    Context(Context&& v)
        : _BaseContext(std::move(v)),
          _status(std::move(v._status)),
          _pm_result(std::move(v._pm_result)) {}
    Context(const Context&) = delete;
    friend struct AsyncGenerator;

   private:
    Status _status;
    __Any _pm_result;
    Result<void, YieldType> _result;
  } Context;

 private:
  typedef _BaseGenerator<void(Context*), Context> _type;
  static Promise::Promise<Result<void, YieldType>> _next(
      const std::shared_ptr<_type>& _ctx) {
    if (_ctx->ctx._status == Yielded || _ctx->ctx._status == Pending ||
        _ctx->ctx._status == Awaiting) {
      _ctx->ctx._status = Active;
      _ctx->ctx._switch_ctx();
      if (_ctx->ctx._status == Awaiting) {
        std::shared_ptr<_type> tmp = _ctx;
        return __Any_cast<Promise::Promise<__Any>>(_ctx->ctx._pm_result)
            .then([tmp](const __Any& v) {
              tmp->ctx._pm_result = v;
              return _next(tmp);
            });
      }
      return Promise::resolve(_ctx->ctx._result);
    } else if (_ctx->ctx._status == Returned) {
      return Promise::resolve(_ctx->ctx._result);
    }
    throw std::bad_function_call();
  }
  static void run_fn(_type* self) {
    self->fn(&self->ctx);
    self->ctx._status = Returned;
    setcontext(&self->ctx._ctx);
  }

  std::shared_ptr<_type> _ctx;

 public:
  /**
   * @brief 获得当前生成器的状态。
   *
   * @return Status 生成器状态。
   */
  Status status() const noexcept { return _ctx->ctx._status; }
  /**
   * @brief 获得生成器返回的下一个值。若在生成器内调用此函数则抛出
   * std::bad_function_call 错误。
   *
   * @return Promise::Promise<Result<void, YieldType>> 结果。
   */
  Promise::Promise<Result<void, YieldType>> next() const {
    return _next(_ctx);
  }
  /**
   * @brief 根据函数构造生成器。
   *
   * @param fn 生成器内部的函数。
   */
  explicit AsyncGenerator(const decltype(_type::fn)& fn) {
    _type* ptr = (decltype(ptr)) new char[sizeof(_type)];
    new (ptr) _type(fn, (void (*)(void))run_fn, ptr);
    _ctx = std::shared_ptr<_type>(ptr);
  }
};
/**
 * @brief 不中断值的异步生成器。
 *
 * @tparam RetType 返回类型。
 */
template <typename RetType>
struct AsyncGenerator<RetType, void> {
  /**
   * @brief 生成器的状态。
   */
  typedef enum Status {
    Pending = 0,   // 尚未运行
    Active = 1,    // 运行中
    Returned = 2,  // 已返回
    Awaiting = 3,  // await 中
  } Status;
  /**
   * @brief 生成器上下文。
   */
  typedef struct Context : public _BaseContext {
    /**
     * @brief 等待一个 Promise 完成，并取得 Promise
     * 的值。若不在生成器内调用此函数则抛出 std::bad_function_call 错误。
     *
     * @tparam T Promise 内部元素的类型。
     * @param value Promise 本身。
     * @return T Promise 的值。
     */
    template <typename T>
    T await(const Promise::Promise<T>& value) {
      return _AwaitImpl<T, AsyncGenerator>::apply(&Context::_switch_ctx, this,
                                                  &_pm_result, &_status, value);
    }
    Context(void (*fn)(void), void* arg)
        : _BaseContext(fn, arg), _status(Pending) {}
    Context(Context&& v)
        : _BaseContext(std::move(v)),
          _status(std::move(v._status)),
          _pm_result(std::move(v._pm_result)) {}
    Context(const Context&) = delete;
    friend struct AsyncGenerator;

   private:
    Status _status;
    __Any _pm_result;
    RetType _result;
  } Context;

 private:
  typedef _BaseGenerator<RetType(Context*), Context> _type;
  static Promise::Promise<RetType> _next(const std::shared_ptr<_type>& _ctx) {
    if (_ctx->ctx._status == Pending || _ctx->ctx._status == Awaiting) {
      _ctx->ctx._status = Active;
      _ctx->ctx._switch_ctx();
      if (_ctx->ctx._status == Awaiting) {
        std::shared_ptr<_type> tmp = _ctx;
        return __Any_cast<Promise::Promise<__Any>>(_ctx->ctx._pm_result)
            .then([tmp](const __Any& v) {
              tmp->ctx._pm_result = v;
              return _next(tmp);
            });
      }
      return Promise::resolve(_ctx->ctx._result);
    } else if (_ctx->ctx._status == Returned) {
      return Promise::resolve(_ctx->ctx._result);
    }
    throw std::bad_function_call();
  }
  static void run_fn(_type* self) {
    self->ctx._result = self->fn(&self->ctx);
    self->ctx._status = Returned;
    setcontext(&self->ctx._ctx);
  }

  std::shared_ptr<_type> _ctx;

 public:
  /**
   * @brief 获得当前生成器的状态。
   *
   * @return Status 生成器状态。
   */
  Status status() const noexcept { return _ctx->ctx._status; }
  /**
   * @brief 获得生成器返回的下一个值。若在生成器内调用此函数则抛出
   * std::bad_function_call 错误。
   *
   * @return Promise::Promise<RetType> 结果。
   */
  Promise::Promise<RetType> next() const { return _next(_ctx); }
  /**
   * @brief 根据函数构造生成器。
   *
   * @param fn 生成器内部的函数。
   */
  explicit AsyncGenerator(const decltype(_type::fn)& fn) {
    _type* ptr = (decltype(ptr)) new char[sizeof(_type)];
    new (ptr) _type(fn, (void (*)(void))run_fn, ptr);
    _ctx = std::shared_ptr<_type>(ptr);
  }
};
/**
 * @brief 不中断值也不返回值的异步生成器。
 */
template <>
struct AsyncGenerator<void, void> {
  /**
   * @brief 生成器的状态。
   */
  typedef enum Status {
    Pending = 0,   // 尚未运行
    Active = 1,    // 运行中
    Returned = 2,  // 已返回
    Awaiting = 3,  // await 中
  } Status;
  /**
   * @brief 生成器上下文。
   */
  typedef struct Context : public _BaseContext {
    /**
     * @brief 等待一个 Promise 完成，并取得 Promise
     * 的值。若不在生成器内调用此函数则抛出 std::bad_function_call 错误。
     *
     * @tparam T Promise 内部元素的类型。
     * @param value Promise 本身。
     * @return T Promise 的值。
     */
    template <typename T>
    T await(const Promise::Promise<T>& value) {
      return _AwaitImpl<T, AsyncGenerator>::apply(&Context::_switch_ctx, this,
                                                  &_pm_result, &_status, value);
    }
    Context(void (*fn)(void), void* arg)
        : _BaseContext(fn, arg), _status(Pending) {}
    Context(Context&& v)
        : _BaseContext(std::move(v)),
          _status(std::move(v._status)),
          _pm_result(std::move(v._pm_result)) {}
    Context(const Context&) = delete;
    friend struct AsyncGenerator;

   private:
    Status _status;
    __Any _pm_result;
  } Context;

 private:
  typedef _BaseGenerator<void(Context*), Context> _type;
  static Promise::Promise<void> _next(const std::shared_ptr<_type>& _ctx) {
    if (_ctx->ctx._status == Pending || _ctx->ctx._status == Awaiting) {
      _ctx->ctx._status = Active;
      _ctx->ctx._switch_ctx();
      if (_ctx->ctx._status == Awaiting) {
        std::shared_ptr<_type> tmp = _ctx;
        return __Any_cast<Promise::Promise<__Any>>(_ctx->ctx._pm_result)
            .then([tmp](const __Any& v) {
              tmp->ctx._pm_result = v;
              return _next(tmp);
            });
      }
      return Promise::resolve();
    } else if (_ctx->ctx._status == Returned) {
      return Promise::resolve();
    }
    throw std::bad_function_call();
  }
  static void run_fn(_type* self) {
    self->fn(&self->ctx);
    self->ctx._status = Returned;
    setcontext(&self->ctx._ctx);
  }
  std::shared_ptr<_type> _ctx;

 public:
  /**
   * @brief 获得当前生成器的状态。
   *
   * @return Status 生成器状态。
   */
  Status status() const noexcept { return _ctx->ctx._status; }
  /**
   * @brief 获得生成器返回的下一个值。若在生成器内调用此函数则抛出
   * std::bad_function_call 错误。
   *
   * @return Promise::Promise<void> 结果。
   */
  Promise::Promise<void> next() const { return _next(_ctx); }
  /**
   * @brief 根据函数构造生成器。
   *
   * @param fn 生成器内部的函数。
   */
  explicit AsyncGenerator(const decltype(_type::fn)& fn) {
    char* ptr = new char[sizeof(_type)];
    new (ptr) _type(fn, (void (*)(void))run_fn, ptr);
    _ctx = std::shared_ptr<_type>((_type*)ptr);
  }
};
}  // namespace Generator
#endif