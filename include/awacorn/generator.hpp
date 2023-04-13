#ifndef _AWACORN_GENERATOR
#define _AWACORN_GENERATOR
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
#include "promise.hpp"

namespace awacorn {
struct cancel_error : public std::exception {
  ~cancel_error() noexcept override {}
  const char* what() const noexcept override { return "generator cancelled"; }
};
/**
 * @brief 表示返回或中断值。
 *
 * @tparam RetType 返回的类型
 * @tparam YieldType 中断的类型
 */
template <typename RetType, typename YieldType>
struct result_t {
  using ret_type = typename std::decay<RetType>::type;
  using yield_type = typename std::decay<YieldType>::type;
  /**
   * @brief 结果的类型。
   */
  enum state_t {
    Null = 0,    // 空
    Return = 1,  // 返回类型
    Yield = 2    // 中断类型
  };
  /**
   * @brief 获得返回值。类型不正确则抛出 std::bad_cast 错误。
   *
   * @return const ret_type& 返回值的只读引用。
   */
  const ret_type& ret() const {
    if (_type != Return) throw std::bad_cast();
    return *(const ret_type*)_val;
  }
  /**
   * @brief 获得中断值。类型不正确则抛出 std::bad_cast 错误。
   *
   * @return const YieldType& 中断值的只读引用。
   */
  const yield_type& yield() const {
    if (_type != Yield) throw std::bad_cast();
    return *(const yield_type*)_val;
  }
  /**
   * @brief 获得结果类型。
   *
   * @return state_t 结果类型。
   */
  state_t type() const noexcept { return _type; }
  static result_t<ret_type, yield_type> generate_yield(
      const yield_type& value) {
    result_t<ret_type, yield_type> tmp;
    tmp._type = Yield;
    new (tmp._val) yield_type(value);
    return tmp;
  }
  static result_t<ret_type, yield_type> generate_yield(yield_type&& value) {
    result_t<ret_type, yield_type> tmp;
    tmp._type = Yield;
    new (tmp._val) yield_type(std::move(value));
    return tmp;
  }
  static result_t<ret_type, yield_type> generate_ret(const ret_type& value) {
    result_t<ret_type, yield_type> tmp;
    tmp._type = Return;
    new (tmp._val) ret_type(value);
    return tmp;
  }
  static result_t<ret_type, yield_type> generate_ret(ret_type&& value) {
    result_t<ret_type, yield_type> tmp;
    tmp._type = Return;
    new (tmp._val) ret_type(std::move(value));
    return tmp;
  }
  result_t() : _type(Null) {}
  result_t(const result_t<ret_type, yield_type>& val) : _type(val._type) {
    if (val._type == Return) {
      new (_val) ret_type(*(const ret_type*)val._val);
    } else if (val._type == Yield) {
      new (_val) yield_type(*(const yield_type*)val._val);
    }
  }
  result_t(result_t<RetType, YieldType>&& val) : _type(val._type) {
    std::swap(_type, val._type);
    std::swap(_val, val._val);
  }
  result_t& operator=(const result_t& val) {
    ~result_t();
    _type = val._type;
    if (val._type == Return) {
      new (_val) ret_type(*(const ret_type*)val._val);
    } else if (val._type == Yield) {
      new (_val) yield_type(*(const yield_type*)val._val);
    }
    return *this;
  }
  result_t& operator=(result_t&& rhs) {
    std::swap(_type, rhs._type);
    std::swap(_val, rhs._val);
    return *this;
  }
  ~result_t() {
    if (_type == Return) {
      ((ret_type*)_val)->~ret_type();
    } else if (_type == Yield) {
      ((yield_type*)_val)->~yield_type();
    }
  }

 private:
  alignas(alignof(ret_type) > alignof(yield_type)
              ? alignof(ret_type)
              : alignof(yield_type)) unsigned char _val
      [sizeof(ret_type) > sizeof(yield_type) ? sizeof(ret_type)
                                             : sizeof(yield_type)];
  state_t _type;
};
/**
 * @brief 返回类型为空的返回或中断值。
 *
 * @tparam YieldType 中断类型。
 */
template <typename YieldType>
struct result_t<void, YieldType> {
  using ret_type = void;
  using yield_type = typename std::decay<YieldType>::type;
  /**
   * @brief 结果的类型。
   */
  enum state_t {
    Null = 0,    // 空
    Return = 1,  // 返回类型
    Yield = 2    // 中断类型
  };
  /**
   * @brief 获得中断值。类型不正确则抛出 std::bad_cast 错误。
   *
   * @return const YieldType& 中断值的只读引用。
   */
  const yield_type& yield() const {
    if (_type != Yield) throw std::bad_cast();
    return *(const yield_type*)_val;
  }
  /**
   * @brief 获得结果类型。
   *
   * @return state_t 结果类型。
   */
  state_t type() const noexcept { return _type; }
  static result_t<void, yield_type> generate_yield(const yield_type& value) {
    result_t<void, yield_type> tmp;
    tmp._type = Yield;
    new (tmp._val) yield_type(value);
    return tmp;
  }
  static result_t<void, yield_type> generate_yield(yield_type&& value) {
    result_t<void, yield_type> tmp;
    tmp._type = Yield;
    new (tmp._val) yield_type(std::move(value));
    return tmp;
  }
  static result_t<void, yield_type> generate_ret() {
    result_t<void, yield_type> tmp;
    tmp._type = Return;
    return tmp;
  }
  result_t() : _type(Null) {}
  result_t(const result_t<ret_type, yield_type>& val) : _type(val._type) {
    if (val._type == Yield) {
      *(yield_type*)_val = *(const yield_type*)val._val;
    }
  }
  result_t(result_t<ret_type, yield_type>&& val)
      : _val(val._val), _type(val._type) {
    val._type = Null;
  }
  result_t& operator=(const result_t& rhs) { return *new (this) result_t(rhs); }
  result_t& operator=(result_t&& rhs) {
    return *new (this) result_t(std::move(rhs));
  }
  ~result_t() {
    if (_type == Yield) {
      ((yield_type*)_val)->~yield_type();
    }
  }

 private:
  alignas(alignof(yield_type)) unsigned char _val[sizeof(yield_type)];
  state_t _type;
};
namespace detail {
/**
 * @brief 生成器上下文基类。
 */
template <typename State>
struct basic_context {
 protected:
#if defined(AWACORN_USE_BOOST)
  basic_context(void (*fn)(void*), void* arg, size_t stack_size = 0)
      : _cancelling(false),
        _status(State::Pending),
        _ctx(boost::context::callcc(
            std::allocator_arg, boost::context::fixedsize_stack(stack_size),
            [this, fn, arg](boost::context::continuation&& ctx) {
              _ctx = ctx.resume();
              fn(arg);
              return std::move(_ctx);
            })) {}
#elif defined(AWACORN_USE_UCONTEXT)
  basic_context(void (*fn)(void*), void* arg, size_t stack_size = 0)
      : _cancelling(false),
        _status(State::Pending),
        _stack(nullptr, [](char* ptr) {
          if (ptr) delete[] ptr;
        }) {
    getcontext(&_ctx);
    if (!stack_size) stack_size = 128 * 1024;  // default stack size
    _stack.reset(new char[stack_size]);
    ctx.uc_stack.ss_sp = _stack.get();
    ctx.uc_stack.ss_size = stack_size;
    ctx.uc_stack.ss_flags = 0;
    ctx.uc_link = nullptr;
    makecontext(&_ctx, (void (*)(void))fn, 1, arg);
  }
#else
#error Please define "AWACORN_USE_UCONTEXT" or "AWACORN_USE_BOOST".
#endif
  basic_context(const basic_context&) = delete;
  // ctx->yield(T) implementation
  struct __yield_impl {
    template <typename RetType, typename YieldType>
    static void apply(basic_context<State>* ctx, YieldType&& value,
                      any* result) {
      if (ctx->_status != State::Active) throw std::bad_function_call();
      ctx->_status = State::Yielded;
      *result = std::forward<YieldType>(value);
      ctx->resume();
      if (ctx->_cancelling) {
        throw cancel_error();
      }
    }
  };
  // ctx->await(Promise::Promise<T>) implementation
  struct __await_impl {
    template <typename T>
    static T apply(basic_context<State>* ctx, const promise<T>& value,
                   any* result, bool* failbit) {
      if (ctx->_status != State::Active) throw std::bad_function_call();
      ctx->_status = State::Awaiting;
      *result = value.then([](const T& v) { return any(v); });
      ctx->resume();
      if (ctx->_cancelling) {
        throw cancel_error();
      }
      if (*failbit) {
        *failbit = false;
        throw *result;
      }
      return result->template cast<T>();
    }
    static void apply(basic_context<State>* ctx, const promise<void>& value,
                      any* result, bool* failbit) {
      if (ctx->_status != State::Active) throw std::bad_function_call();
      ctx->_status = State::Awaiting;
      *result = value.then([]() { return any(); });
      ctx->resume();
      if (ctx->_cancelling) {
        throw cancel_error();
      }
      if (*failbit) {
        *failbit = false;
        throw *result;
      }
    }
  };
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
  bool _cancelling;
  State _status;

 private:
#if defined(AWACORN_USE_BOOST)
  boost::context::continuation _ctx;
#elif defined(AWACORN_USE_UCONTEXT)
  ucontext_t _ctx;
  std::unique_ptr<char, void (*)(char*)> _stack;
#else
#error Please define "AWACORN_USE_UCONTEXT" or "AWACORN_USE_BOOST".
#endif
};
/**
 * @brief 生成器成员，用于实现智能生命周期。
 *
 * @tparam Fn 函数类型。
 * @tparam Context 上下文类型。
 */
template <typename Fn, typename Context>
struct basic_generator {
 protected:
  Context ctx;
  detail::function<Fn> fn;
  template <typename U>
  basic_generator(U&& fn, void (*run_fn)(void*), void* args,
                  size_t stack_size = 0)
      : ctx(run_fn, args, stack_size), fn(std::forward<U>(fn)) {}
  basic_generator(const basic_generator& v) = delete;
};
};  // namespace detail
/**
 * @brief 生成器。
 *
 * @tparam RetType 返回类型。
 * @tparam YieldType 中断类型。
 */
template <typename RetType, typename YieldType>
struct generator {
  /**
   * @brief 生成器的状态。
   */
  enum state_t {
    Pending = 0,   // 尚未运行
    Yielded = 1,   // 已中断
    Active = 2,    // 运行中
    Returned = 3,  // 已返回
    Throwed = 4,   // 抛出错误
    Cancelled = 5  // 已取消
  };

 private:
  struct _generator;
  /**
   * @brief 生成器上下文。
   */
  struct context : private detail::basic_context<state_t> {
    /**
     * @brief 中断当前的生成器。若不在生成器内调用此函数则抛出
     * std::bad_function_call 错误。
     *
     * @param value 中断值。
     */
    void yield(const YieldType& value) {
      context::__yield_impl::template apply<RetType, YieldType>(this, value,
                                                                &this->_result);
    }
    void yield(YieldType&& value) {
      context::__yield_impl::template apply<RetType, YieldType>(
          this, std::move(value), &this->_result);
    }
    context(void (*fn)(void*), void* arg, size_t stack_size = 0)
        : detail::basic_context<state_t>(fn, arg, stack_size) {}
    context(const context&) = delete;
    friend struct _generator;

   private:
    any _result;
  };
  struct _generator : public detail::basic_generator<RetType(context*), context> {
    // ctx(run_fn, this, stack_size), fn(std::forward<U>(fn))
    template <typename U>
    explicit _generator(U&& fn, size_t stack_size = 0)
        : detail::basic_generator<RetType(context*), context>(
              std::forward<U>(fn), (void (*)(void*))run_fn, this, stack_size) {}
    explicit _generator(const _generator& v) = delete;
    ~_generator() {
      if (this->ctx._status == Yielded) {
        this->ctx._cancelling = true;
        try {
          next();
        } catch (...) {
        }
      }
    }
    inline state_t status() const noexcept { return this->ctx._status; }
    result_t<RetType, YieldType> next() {
      if (this->ctx._status == Yielded || this->ctx._status == Pending) {
        this->ctx._status = Active;
        this->ctx.resume();
        if (this->ctx._status == Yielded) {
          return result_t<RetType, YieldType>::generate_yield(
              this->ctx._result.template cast<YieldType>());
        } else if (this->ctx._status == Returned) {
          return result_t<RetType, YieldType>::generate_ret(
              this->ctx._result.template cast<RetType>());
        } else if (this->ctx._status == Cancelled) {
          throw cancel_error();
        }
        throw this->ctx._result;
      } else if (this->ctx._status == Returned) {
        return result_t<RetType, YieldType>::generate_ret(
            this->ctx._result.template cast<RetType>());
      } else if (this->ctx._status == Throwed) {
        throw this->ctx._result;
      }
      throw std::bad_function_call();
    }

   private:
    static void run_fn(_generator* self) {
      try {
        self->ctx._result = self->fn(&self->ctx);
        self->ctx._status = Returned;
      } catch (const any& err) {
        self->ctx._result = err;
        self->ctx._status = Throwed;
      } catch (const cancel_error&) {
        self->ctx._status = Cancelled;
      }
      self->ctx.resume();
    }
  };

  std::shared_ptr<_generator> _ctx;

 public:
  using context = context;
  /**
   * @brief 获得当前生成器的状态。
   *
   * @return state_t 生成器状态。
   */
  inline state_t status() const noexcept { return _ctx->status(); }
  /**
   * @brief 获得生成器返回的下一个值。若在生成器内调用此函数则抛出
   * std::bad_function_call 错误。
   *
   * @return result_t<RetType, YieldType> 结果。
   */
  result_t<RetType, YieldType> next() const { return _ctx->next(); }
  /**
   * @brief 根据函数构造生成器。
   *
   * @param fn 生成器内部的函数。
   * @param stack_size 栈大小。
   */
  template <typename U>
  explicit generator(U&& fn, size_t stack_size = 0) {
    _ctx = std::make_shared<_generator>(std::forward<U>(fn), stack_size);
  }
};
/**
 * @brief 不返回值的生成器。
 *
 * @tparam YieldType 中断类型。
 */
template <typename YieldType>
struct generator<void, YieldType> {
  /**
   * @brief 生成器的状态。
   */
  enum state_t {
    Pending = 0,   // 尚未运行
    Yielded = 1,   // 已中断
    Active = 2,    // 运行中
    Returned = 3,  // 已返回
    Throwed = 4,   // 抛出错误
    Cancelled = 5  // 已取消
  };

 private:
  struct _generator;
  /**
   * @brief 生成器上下文。
   */
  struct context : private detail::basic_context<state_t> {
    /**
     * @brief 中断当前的生成器。若不在生成器内调用此函数则抛出
     * std::bad_function_call 错误。
     *
     * @param value 中断值。
     */
    void yield(const YieldType& value) {
      context::__yield_impl::template apply<void, YieldType>(this, value,
                                                             &this->_result);
    }
    void yield(YieldType&& value) {
      context::__yield_impl::template apply<void, YieldType>(
          this, std::move(value), &this->_result);
    }
    context(void (*fn)(void*), void* arg, size_t stack_size = 0)
        : detail::basic_context<state_t>(fn, arg, stack_size) {}
    context(const context&) = delete;
    friend struct _generator;

   private:
    any _result;
  };
  struct _generator : public detail::basic_generator<void(context*), context> {
    template <typename U>
    explicit _generator(U&& fn, size_t stack_size = 0)
        : detail::basic_generator<void(context*), context>(
              std::forward<U>(fn), (void (*)(void*))run_fn, this, stack_size) {}
    explicit _generator(const _generator& v) = delete;
    ~_generator() {
      if (this->ctx._status == Yielded) {
        this->ctx._cancelling = true;
        try {
          next();
        } catch (...) {
        }
      }
    }
    inline state_t status() const noexcept { return this->ctx._status; }
    result_t<void, YieldType> next() {
      if (this->ctx._status == Yielded || this->ctx._status == Pending) {
        this->ctx._status = Active;
        this->ctx.resume();
        if (this->ctx._status == Yielded) {
          return result_t<void, YieldType>::generate_yield(
              this->ctx._result.template cast<YieldType>());
        } else if (this->ctx._status == Returned) {
          return result_t<void, YieldType>::generate_ret();
        } else if (this->ctx._status == Cancelled) {
          throw cancel_error();
        }
        throw this->ctx._result;
      } else if (this->ctx._status == Returned) {
        return result_t<void, YieldType>::generate_ret();
      } else if (this->ctx._status == Throwed) {
        throw this->ctx._result;
      }
      throw std::bad_function_call();
    }

   private:
    static void run_fn(_generator* self) {
      try {
        self->fn(&self->ctx);
        self->ctx._status = Returned;
      } catch (const any& err) {
        self->ctx._result = err;
        self->ctx._status = Throwed;
      } catch (const cancel_error&) {
        self->ctx._status = Cancelled;
      }
      self->ctx.resume();
    }
  };

  std::shared_ptr<_generator> _ctx;

 public:
  using context = context;
  /**
   * @brief 获得当前生成器的状态。
   *
   * @return state_t 生成器状态。
   */
  inline state_t status() const noexcept { return _ctx->status(); }
  /**
   * @brief 获得生成器返回的下一个值。若在生成器内调用此函数则抛出
   * std::bad_function_call 错误。
   *
   * @return result_t<void, YieldType> 结果。
   */
  result_t<void, YieldType> next() const { return _ctx->next(); }
  /**
   * @brief 根据函数构造生成器。
   *
   * @param fn 生成器内部的函数。
   * @param stack_size 栈大小。
   */
  template <typename U>
  explicit generator(U&& fn, size_t stack_size = 0) {
    _ctx = std::make_shared<_generator>(std::forward<U>(fn), stack_size);
  }
};
/**
 * @brief 异步生成器。
 *
 * @tparam RetType 返回类型。
 * @tparam YieldType 中断类型。
 */
template <typename RetType, typename YieldType = void>
struct async_generator {
  /**
   * @brief 生成器的状态。
   */
  enum state_t {
    Pending = 0,   // 尚未运行
    Yielded = 1,   // 已中断
    Active = 2,    // 运行中
    Returned = 3,  // 已返回
    Awaiting = 4,  // await 中
    Throwed = 5,   // 抛出错误
    Cancelled = 6  // 已取消
  };

 private:
  struct _generator;
  /**
   * @brief 生成器上下文。
   */
  struct context : private detail::basic_context<state_t> {
    /**
     * @brief 中断当前的生成器。若不在生成器内调用此函数则抛出
     * std::bad_function_call 错误。
     *
     * @param value 中断值。
     */
    void yield(const YieldType& value) {
      context::__yield_impl::template apply<RetType, YieldType>(this, value,
                                                                &this->_result);
    }
    void yield(YieldType&& value) {
      context::__yield_impl::template apply<RetType, YieldType>(
          this, std::move(value), &this->_result);
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
    T await(const promise<T>& value) {
      return context::__await_impl::template apply<T>(
          this, value, &this->_result, &this->_failbit);
    }
    context(void (*fn)(void*), void* arg, size_t stack_size = 0)
        : detail::basic_context<state_t>(fn, arg, stack_size), _failbit(false) {}
    context(const context&) = delete;
    friend struct _generator;

   private:
    any _result;
    bool _failbit;
  };
  struct _generator : public detail::basic_generator<RetType(context*), context> {
    template <typename U>
    explicit _generator(U&& fn, size_t stack_size = 0)
        : detail::basic_generator<RetType(context*), context>(
              std::forward<U>(fn), (void (*)(void*))run_fn, this, stack_size) {}
    explicit _generator(const _generator& v) = delete;
    ~_generator() {
      if (this->ctx._status == Yielded) {
        this->ctx._cancelling = true;
        next();
      } else if (this->ctx._status == Awaiting) {
        this->ctx._cancelling = true;
        this->ctx._result.template cast<promise<any>>().reject(any());
      }
    }
    inline state_t status() const noexcept { return this->ctx._status; }
    promise<result_t<RetType, YieldType>> next(
        const std::shared_ptr<_generator>& ref) {
      if (this->ctx._status == Awaiting || this->ctx._status == Yielded ||
          this->ctx._status == Pending) {
        this->ctx._status = Active;
        this->ctx.resume();
        if (this->ctx._status == Awaiting) {
          return this->ctx._result.template cast<promise<any>>()
              .then([ref](const any& v) {
                ref->ctx._result = v;
                return ref->next(ref);
              })
              .error([ref](const any& err) {
                ref->ctx._result = err;
                ref->ctx._failbit = true;
                return ref->next(ref);
              });
        } else if (this->ctx._status == Yielded) {
          return resolve(result_t<RetType, YieldType>::generate_yield(
              this->ctx._result.template cast<YieldType>()));
        } else if (this->ctx._status == Returned) {
          return resolve(result_t<RetType, YieldType>::generate_ret(
              this->ctx._result.template cast<RetType>()));
        } else if (this->ctx._status == Cancelled) {
          return reject<result_t<RetType, YieldType>>(any(cancel_error()));
        }
        return reject(this->ctx._result);
      } else if (this->ctx._status == Returned) {
        return resolve(result_t<RetType, YieldType>::generate_ret(
            this->ctx._result.template cast<RetType>()));
      } else if (this->ctx._status == Throwed) {
        return reject(this->ctx._result);
      }
      throw std::bad_function_call();
    }

   private:
    static void run_fn(_generator* self) {
      try {
        self->ctx._result = self->fn(&self->ctx);
        self->ctx._status = Returned;
      } catch (const any& err) {
        self->ctx._result = err;
        self->ctx._status = Throwed;
      } catch (const cancel_error&) {
        self->ctx._status = Cancelled;
      }
      self->ctx.resume();
    }
  };

  std::shared_ptr<_generator> _ctx;

 public:
  using context = context;
  /**
   * @brief 获得当前生成器的状态。
   *
   * @return state_t 生成器状态。
   */
  inline state_t status() const noexcept { return _ctx->status(); }
  /**
   * @brief 获得生成器返回的下一个值。若在生成器内调用此函数则抛出
   * std::bad_function_call 错误。
   *
   * @return Promise::Promise<result_t<RetType, YieldType>> 结果。
   */
  promise<result_t<RetType, YieldType>> next() const {
    return _ctx->next(_ctx);
  }
  /**
   * @brief 根据函数构造生成器。
   *
   * @param fn 生成器内部的函数。
   * @param stack_size 栈大小。
   */
  template <typename U>
  explicit async_generator(U&& fn, size_t stack_size = 0) {
    _ctx = std::make_shared<_generator>(std::forward<U>(fn), stack_size);
  }
};
/**
 * @brief 不返回值的异步生成器。
 *
 * @tparam YieldType 中断类型。
 */
template <typename YieldType>
struct async_generator<void, YieldType> {
  /**
   * @brief 生成器的状态。
   */
  enum state_t {
    Pending = 0,   // 尚未运行
    Yielded = 1,   // 已中断
    Active = 2,    // 运行中
    Returned = 3,  // 已返回
    Awaiting = 4,  // await 中
    Throwed = 5,   // 抛出错误
    Cancelled = 6  // 已取消
  };

 private:
  struct _generator;
  /**
   * @brief 生成器上下文。
   */
  struct context : private detail::basic_context<state_t> {
    /**
     * @brief 中断当前的生成器。若不在生成器内调用此函数则抛出
     * std::bad_function_call 错误。
     *
     * @param value 中断值。
     */
    void yield(const YieldType& value) {
      context::__yield_impl::template apply<void, YieldType>(this, value,
                                                             &this->_result);
    }
    void yield(YieldType&& value) {
      context::__yield_impl::template apply<void, YieldType>(
          this, std::move(value), &this->_result);
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
    T await(const promise<T>& value) {
      return context::__await_impl::template apply<T>(
          this, value, &this->_result, &this->_failbit);
    }
    context(void (*fn)(void*), void* arg, size_t stack_size = 0)
        : detail::basic_context<state_t>(fn, arg, stack_size), _failbit(false) {}
    context(const context&) = delete;
    friend struct _generator;

   private:
    any _result;
    bool _failbit;
  };
  struct _generator : public detail::basic_generator<void(context*), context> {
    template <typename U>
    explicit _generator(U&& fn, size_t stack_size = 0)
        : detail::basic_generator<void(context*), context>(
              std::forward<U>(fn), (void (*)(void*))run_fn, this, stack_size) {}
    explicit _generator(const _generator& v) = delete;
    ~_generator() {
      if (this->ctx._status == Yielded) {
        this->ctx._cancelling = true;
        next();
      } else if (this->ctx._status == Awaiting) {
        this->ctx._cancelling = true;
        this->ctx._result.template cast<promise<any>>().reject(any());
      }
    }
    inline state_t status() const noexcept { return this->ctx._status; }
    promise<result_t<void, YieldType>> next(
        const std::shared_ptr<_generator>& ref) {
      if (this->ctx._status == Awaiting || this->ctx._status == Yielded ||
          this->ctx._status == Pending) {
        this->ctx._status = Active;
        this->ctx.resume();
        if (this->ctx._status == Awaiting) {
          return this->ctx._result.template cast<promise<any>>()
              .then([ref](const any& v) {
                ref->ctx._result = v;
                return ref->next(ref);
              })
              .error([ref](const any& err) {
                ref->ctx._result = err;
                ref->ctx._failbit = true;
                return ref->next(ref);
              });
        } else if (this->ctx._status == Yielded) {
          return resolve(result_t<void, YieldType>::generate_yield(
              this->ctx._result.template cast<YieldType>()));
        } else if (this->ctx._status == Returned) {
          return resolve(result_t<void, YieldType>::generate_ret());
        } else if (this->ctx._status == Cancelled) {
          return reject<result_t<void, YieldType>>(cancel_error());
        }
        return reject<result_t<void, YieldType>>(this->ctx._result);
      } else if (this->ctx._status == Returned) {
        return resolve(result_t<void, YieldType>::generate_ret());
      } else if (this->ctx._status == Throwed) {
        return reject<result_t<void, YieldType>>(this->ctx._result);
      }
      throw std::bad_function_call();
    }

   private:
    static void run_fn(_generator* self) {
      try {
        self->fn(&self->ctx);
        self->ctx._status = Returned;
      } catch (const any& err) {
        self->ctx._result = err;
        self->ctx._status = Throwed;
      } catch (const cancel_error&) {
        self->ctx._status = Cancelled;
      }
      self->ctx.resume();
    }
  };
  std::shared_ptr<_generator> _ctx;

 public:
  using context = context;
  /**
   * @brief 获得当前生成器的状态。
   *
   * @return state_t 生成器状态。
   */
  state_t status() const noexcept { return _ctx->ctx._status; }
  /**
   * @brief 获得生成器返回的下一个值。若在生成器内调用此函数则抛出
   * std::bad_function_call 错误。
   *
   * @return Promise::Promise<result_t<void, YieldType>> 结果。
   */
  promise<result_t<void, YieldType>> next() const { return _ctx->next(_ctx); }
  /**
   * @brief 根据函数构造生成器。
   *
   * @param fn 生成器内部的函数。
   * @param stack_size 栈大小。
   */
  template <typename U>
  explicit async_generator(U&& fn, size_t stack_size = 0) {
    _ctx = std::make_shared<_generator>(std::forward<U>(fn), stack_size);
  }
};
/**
 * @brief 不中断值的异步生成器。
 *
 * @tparam RetType 返回类型。
 */
template <typename RetType>
struct async_generator<RetType, void> {
  /**
   * @brief 生成器的状态。
   */
  enum state_t {
    Pending = 0,   // 尚未运行
    Active = 1,    // 运行中
    Returned = 2,  // 已返回
    Awaiting = 3,  // await 中
    Throwed = 4,   // 抛出错误
    Cancelled = 5  // 已取消
  };

 private:
  struct _generator;
  /**
   * @brief 生成器上下文。
   */
  struct context : private detail::basic_context<state_t> {
    /**
     * @brief 等待一个 Promise 完成，并取得 Promise
     * 的值。若不在生成器内调用此函数则抛出 std::bad_function_call 错误。
     *
     * @tparam T Promise 内部元素的类型。
     * @param value Promise 本身。
     * @return T Promise 的值。
     */
    template <typename T>
    T await(const promise<T>& value) {
      return context::__await_impl::template apply<T>(
          this, value, &this->_result, &this->_failbit);
    }
    context(void (*fn)(void*), void* arg, size_t stack_size = 0)
        : detail::basic_context<state_t>(fn, arg, stack_size), _failbit(false) {}
    context(const context&) = delete;
    friend struct _generator;

   private:
    any _result;
    bool _failbit;
  };
  struct _generator : public detail::basic_generator<RetType(context*), context> {
    template <typename U>
    explicit _generator(U&& fn, size_t stack_size = 0)
        : detail::basic_generator<RetType(context*), context>(
              std::forward<U>(fn), (void (*)(void*))run_fn, this, stack_size) {}
    explicit _generator(const _generator& v) = delete;
    ~_generator() {
      if (this->ctx._status == Awaiting) {
        this->ctx._cancelling = true;
        this->ctx._result.template cast<promise<any>>().reject(any());
      }
    }
    inline state_t status() const noexcept { return this->ctx._status; }
    promise<RetType> next(const std::shared_ptr<_generator>& ref) {
      if (this->ctx._status == Awaiting || this->ctx._status == Pending) {
        this->ctx._status = Active;
        this->ctx.resume();
        if (this->ctx._status == Awaiting) {
          return this->ctx._result.template cast<promise<any>>()
              .then([ref](const any& v) {
                ref->ctx._result = v;
                return ref->next(ref);
              })
              .error([ref](const any& err) {
                ref->ctx._result = err;
                ref->ctx._failbit = true;
                return ref->next(ref);
              });
        } else if (this->ctx._status == Returned) {
          return resolve(this->ctx._result.template cast<RetType>());
        } else if (this->ctx._status == Cancelled) {
          return reject<RetType>(cancel_error());
        }
        return reject<RetType>(this->ctx._result);
      } else if (this->ctx._status == Returned) {
        return resolve(this->ctx._result.template cast<RetType>());
      } else if (this->ctx._status == Throwed) {
        return reject<RetType>(this->ctx._result);
      }
      throw std::bad_function_call();
    }

   private:
    static void run_fn(_generator* self) {
      try {
        self->ctx._result = self->fn(&self->ctx);
        self->ctx._status = Returned;
      } catch (const any& err) {
        self->ctx._result = err;
        self->ctx._status = Throwed;
      } catch (const cancel_error&) {
        self->ctx._status = Cancelled;
      }
      self->ctx.resume();
    }
  };
  std::shared_ptr<_generator> _ctx;

 public:
  using context = context;
  /**
   * @brief 获得当前生成器的状态。
   *
   * @return state_t 生成器状态。
   */
  inline state_t status() const noexcept { return _ctx->status(); }
  /**
   * @brief 获得生成器返回的下一个值。若在生成器内调用此函数则抛出
   * std::bad_function_call 错误。
   *
   * @return Promise::Promise<RetType> 结果。
   */
  promise<RetType> next() const { return _ctx->next(_ctx); }
  /**
   * @brief 根据函数构造生成器。
   *
   * @param fn 生成器内部的函数。
   * @param stack_size 栈大小。
   */
  template <typename U>
  explicit async_generator(U&& fn, size_t stack_size = 0) {
    _ctx = std::make_shared<_generator>(std::forward<U>(fn), stack_size);
  }
};
/**
 * @brief 不中断值也不返回值的异步生成器。
 */
template <>
struct async_generator<void, void> {
  /**
   * @brief 生成器的状态。
   */
  enum state_t {
    Pending = 0,   // 尚未运行
    Active = 1,    // 运行中
    Returned = 2,  // 已返回
    Awaiting = 3,  // await 中
    Throwed = 4,   // 抛出错误
    Cancelled = 5  // 已取消
  };

 private:
  struct _generator;
  /**
   * @brief 生成器上下文。
   */
  struct context : private detail::basic_context<state_t> {
    /**
     * @brief 等待一个 Promise 完成，并取得 Promise
     * 的值。若不在生成器内调用此函数则抛出 std::bad_function_call 错误。
     *
     * @tparam T Promise 内部元素的类型。
     * @param value Promise 本身。
     * @return T Promise 的值。
     */
    template <typename T>
    T await(const promise<T>& value) {
      return context::__await_impl::template apply<T>(
          this, value, &this->_result, &this->_failbit);
    }
    context(void (*fn)(void*), void* arg, size_t stack_size = 0)
        : detail::basic_context<state_t>(fn, arg, stack_size), _failbit(false) {}
    context(const context&) = delete;
    friend struct _generator;

   private:
    any _result;
    bool _failbit;
  };
  struct _generator : public detail::basic_generator<void(context*), context> {
    template <typename U>
    explicit _generator(U&& fn, size_t stack_size = 0)
        : detail::basic_generator<void(context*), context>(
              std::forward<U>(fn), (void (*)(void*))run_fn, this, stack_size) {}
    explicit _generator(const _generator& v) = delete;
    ~_generator() {
      if (this->ctx._status == Awaiting) {
        this->ctx._cancelling = true;
        this->ctx._result.template cast<promise<any>>().reject(any());
      }
    }
    inline state_t status() const noexcept { return this->ctx._status; }
    promise<void> next(const std::shared_ptr<_generator>& ref) {
      if (this->ctx._status == Awaiting || this->ctx._status == Pending) {
        this->ctx._status = Active;
        this->ctx.resume();
        if (this->ctx._status == Awaiting) {
          return this->ctx._result.template cast<promise<any>>()
              .then([ref](const any& v) {
                ref->ctx._result = v;
                return ref->next(ref);
              })
              .error([ref](const any& err) {
                ref->ctx._result = err;
                ref->ctx._failbit = true;
                return ref->next(ref);
              });
        } else if (this->ctx._status == Returned) {
          return resolve();
        } else if (this->ctx._status == Cancelled) {
          return reject<void>(cancel_error());
        }
        return reject<void>(ctx._result);
      } else if (this->ctx._status == Returned) {
        return resolve();
      } else if (this->ctx._status == Throwed) {
        return reject<void>(ctx._result);
      }
      throw std::bad_function_call();
    }

   private:
    static void run_fn(_generator* self) {
      try {
        self->fn(&self->ctx);
        self->ctx._status = Returned;
      } catch (const any& err) {
        self->ctx._result = err;
        self->ctx._status = Throwed;
      }
      self->ctx.resume();
    }
  };
  std::shared_ptr<_generator> _ctx;

 public:
  using context = context;
  /**
   * @brief 获得当前生成器的状态。
   *
   * @return state_t 生成器状态。
   */
  inline state_t status() const noexcept { return _ctx->status(); }
  /**
   * @brief 获得生成器返回的下一个值。若在生成器内调用此函数则抛出
   * std::bad_function_call 错误。
   *
   * @return Promise::Promise<void> 结果。
   */
  promise<void> next() const { return _ctx->next(_ctx); }
  /**
   * @brief 根据函数构造生成器。
   *
   * @param fn 生成器内部的函数。
   * @param stack_size 栈大小。
   */
  template <typename U>
  explicit async_generator(U&& fn, size_t stack_size = 0) {
    _ctx = std::make_shared<_generator>(std::forward<U>(fn), stack_size);
  }
};
}  // namespace awacorn
#endif
#endif
