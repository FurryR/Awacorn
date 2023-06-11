#ifndef _AWACORN_CONTEXT_
#define _AWACORN_CONTEXT_
#if __cplusplus >= 201101L
/**
 * Project Awacorn 基于 MIT 协议开源。
 * Copyright(c) 凌 2022.
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
namespace awacorn {
namespace detail {

#if defined(AWACORN_USE_BOOST)
struct basic_context {
  basic_context(void (*fn)(void*), void* arg, std::size_t stack_size = 0)
      : _ctx(boost::context::callcc(
            std::allocator_arg,
            boost::context::fixedsize_stack(
                stack_size ? stack_size
                           : boost::context::stack_traits::default_size()),
            [this, fn, arg](boost::context::continuation&& ctx) {
              _ctx = ctx.resume();
              fn(arg);
              return std::move(_ctx);
            })) {}
  inline void resume() { _ctx = _ctx.resume(); }

 private:
  boost::context::continuation _ctx;
};
#elif defined(AWACORN_USE_UCONTEXT)
struct basic_context {
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
  inline void resume() {
    ucontext_t orig = _ctx;
    swapcontext(&_ctx, &orig);
  }

 private:
  ucontext_t _ctx;
  std::unique_ptr<char, void (*)(char*)> _stack;
};
#else
#error Please define "AWACORN_USE_UCONTEXT" or "AWACORN_USE_BOOST".
#endif

}  // namespace detail
}  // namespace awacorn
#endif
#endif