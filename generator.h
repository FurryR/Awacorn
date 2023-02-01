#ifndef _GENERATOR_H
#define _GENERATOR_H
#ifdef __APPLE__
#define _XOPEN_SOURCE
#endif

#include <ucontext.h>

#include <memory>
#include <vector>

#include "./promise.h"

/**
 * @brief 表示返回或中断值。
 *
 * @tparam RetType 返回的类型
 * @tparam YieldType 中断的类型
 */
template <typename RetType, typename YieldType>
struct UnaryResult {
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
    return *reinterpret_cast<const RetType*>(_val);
  }
  /**
   * @brief 获得中断值。类型不正确则抛出 std::bad_cast 错误。
   *
   * @return const YieldType& 中断值的只读引用。
   */
  const YieldType& yield() const {
    if (_type != Yield) throw std::bad_cast();
    return *reinterpret_cast<const YieldType*>(_val);
  }
  /**
   * @brief 获得结果类型。
   *
   * @return ValueType 结果类型。
   */
  ValueType type() const noexcept { return _type; }
  static UnaryResult<RetType, YieldType> generate_yield(
      const YieldType& value) {
    UnaryResult<RetType, YieldType> tmp;
    tmp._type = Yield;
    *reinterpret_cast<YieldType*>(tmp._val) = value;
    return tmp;
  }
  static UnaryResult<RetType, YieldType> generate_ret(const RetType& value) {
    UnaryResult<RetType, YieldType> tmp;
    tmp._type = Return;
    *reinterpret_cast<RetType*>(tmp._val) = value;
    return tmp;
  }
  UnaryResult() : _type(Null) {}
  UnaryResult(const UnaryResult<RetType, YieldType>& val) : _type(val._type) {
    switch (val._type) {
      case Return:
        (*reinterpret_cast<RetType*>(_val)) =
            (*reinterpret_cast<const RetType*>(val._val));
      case Yield:
        (*reinterpret_cast<YieldType*>(_val)) =
            (*reinterpret_cast<const YieldType*>(val._val));
    }
  }
  ~UnaryResult() {
    switch (_type) {
      case Return: {
        reinterpret_cast<RetType*>(_val)->~RetType();
        break;
      }
      case Yield: {
        reinterpret_cast<YieldType*>(_val)->~YieldType();
        break;
      }
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

template <typename RetType, typename YieldType>
struct Generator {
  /**
   * @brief 生成器的状态。
   */
  typedef enum Status {
    Pending = 0,   // 尚未运行
    Yielded = 1,   // 已中断
    Active = 2,    // 运行中
    Returned = 3,  // 已返回
  } Status;
  /**
   * @brief 生成器上下文。
   */
  typedef struct Context {
    void yield(const YieldType& value) {
      _status = Yielded;
      _result = UnaryResult<RetType, YieldType>::generate_yield(value);
      ucontext_t orig = _ctx;
      swapcontext(&_ctx, &orig);
    }
    friend struct Generator;

   private:
    Status _status;
    ucontext_t _ctx;
    UnaryResult<RetType, YieldType> _result;
    std::vector<char> _stack;
    Context() : _stack(std::vector<char>(1024 * 128)) {}
  } Context;
  /**
   * @brief 获得当前生成器的状态。
   *
   * @return constexpr Status 生成器状态。
   */
  constexpr Status status() const noexcept { return _ctx._status; }
  /**
   * @brief 获得生成器返回的下一个值。若在生成器内/函数返回后调用此函数则抛出
   * std::bad_function_call 错误。
   *
   * @return const UnaryResult& 结果。
   */
  UnaryResult<RetType, YieldType> next() {
    if (_ctx._status == Pending || _ctx._status == Yielded) {
      // _ctx.__status = GeneratorStatus::Active;
      ucontext_t orig = _ctx._ctx;
      _ctx._status = Active;
      swapcontext(&_ctx._ctx, &orig);
      return _ctx._result;
    } else if (_ctx._status == Returned) {
      return _ctx._result;
    }
    throw std::bad_function_call();
  }
  /**
   * @brief 根据函数构造生成器。
   *
   * @param fn 生成器内部的函数。
   */
  explicit Generator(const std::function<RetType(Context*)>& fn) : _fn(fn) {
    _ctx._status = Pending;
    getcontext(&_ctx._ctx);
    _ctx._ctx.uc_stack.ss_sp = _ctx._stack.data();
    _ctx._ctx.uc_stack.ss_size = _ctx._stack.size();
    _ctx._ctx.uc_stack.ss_flags = 0;
    _ctx._ctx.uc_link = nullptr;
    makecontext(&_ctx._ctx, (void (*)(void))run_fn, 1, this);
  }

 private:
  static void run_fn(Generator<RetType, YieldType>* self) {
    self->_ctx._result =
        UnaryResult<RetType, YieldType>::generate_ret(self->_fn(&self->_ctx));
    self->_ctx._status = Returned;
    setcontext(&self->_ctx._ctx);
  }
  std::function<RetType(Context*)> _fn;
  Context _ctx;
};
#endif