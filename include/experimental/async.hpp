#ifndef _AWACORN_EXPERIMENTAL_ASYNC
#define _AWACORN_EXPERIMENTAL_ASYNC
#if __cplusplus >= 201101L
/**
 * Project Awacorn 基于 MIT 协议开源。
 * Copyright(c) 凌 2023.
 */

#include <list>
#include <memory>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <utility>

#include "../detail/capture.hpp"
#include "../detail/function.hpp"
#include "../promise.hpp"

namespace awacorn {
template <typename>
struct context;
template <typename>
struct asyncfn;
template <typename U, typename Ret>
promise<Ret> async(U&& fn);
namespace detail {
template <std::size_t... Is>
struct index_sequence {};
template <std::size_t I, std::size_t... Is>
struct make_index_sequence : make_index_sequence<I - 1, I - 1, Is...> {};
template <std::size_t... Is>
struct make_index_sequence<0, Is...> : index_sequence<Is...> {};
template <typename>
struct _result_of_impl {};
template <typename Object, typename Ret, typename... Args>
struct _result_of_impl<Ret (Object::*)(Args...) const> {
  using result_type = Ret;
  using arg_type = std::tuple<Args...>;
};
template <typename Callable>
struct result_of : _result_of_impl<decltype(&Callable::operator())> {};
template <typename Ret, typename... Args>
struct result_of<Ret(Args...)> {
  using result_type = Ret;
  using arg_type = std::tuple<Args...>;
};
template <typename Ret, typename... Args>
struct result_of<Ret (*)(Args...)> {
  using result_type = Ret;
  using arg_type = std::tuple<Args...>;
};
template <typename T>
struct remove_asyncfn {
  using type = T;
};
template <typename T>
struct remove_asyncfn<asyncfn<T>> {
  using type = T;
};
// value
template <typename Ret, typename Obj, typename Obj2, typename... Args,
          typename... Args2, std::size_t... Is>
inline Ret apply(Ret (Obj::*ptr)(Args...), Obj2&& instance,
                 std::tuple<Args2...>&& args, detail::index_sequence<Is...>) {
  return (std::forward<Obj2>(instance).*ptr)(std::get<Is>(std::move(args))...);
}
template <typename Ret, typename Obj, typename Obj2, typename... Args,
          typename... Args2, std::size_t... Is>
inline Ret apply(Ret (Obj::*ptr)(Args...) const, Obj2&& instance,
                 std::tuple<Args2...>&& args, detail::index_sequence<Is...>) {
  return (std::forward<Obj2>(instance).*ptr)(std::get<Is>(std::move(args))...);
}
// lref
template <typename Ret, typename Obj, typename Obj2, typename... Args,
          typename... Args2, std::size_t... Is>
inline Ret apply(Ret (Obj::*ptr)(Args...) &, Obj2&& instance,
                 std::tuple<Args2...>&& args, detail::index_sequence<Is...>) {
  return (std::forward<Obj2>(instance).*ptr)(std::get<Is>(std::move(args))...);
}
template <typename Ret, typename Obj, typename Obj2, typename... Args,
          typename... Args2, std::size_t... Is>
inline Ret apply(Ret (Obj::*ptr)(Args...) const&, Obj&& instance,
                 std::tuple<Args2...>&& args, detail::index_sequence<Is...>) {
  return (std::forward<Obj2>(instance).*ptr)(std::get<Is>(std::move(args))...);
}
// rref
template <typename Ret, typename Obj, typename Obj2, typename... Args,
          typename... Args2, std::size_t... Is>
inline Ret apply(Ret (Obj::*ptr)(Args...) &&, Obj2&& instance,
                 std::tuple<Args2...>&& args, detail::index_sequence<Is...>) {
  return (std::forward<Obj2>(instance).*ptr)(std::get<Is>(std::move(args))...);
}
template <typename Ret, typename Obj, typename Obj2, typename... Args,
          typename... Args2, std::size_t... Is>
inline Ret apply(Ret (Obj::*ptr)(Args...) const&&, Obj&& instance,
                 std::tuple<Args2...>&& args, detail::index_sequence<Is...>) {
  return (std::forward<Obj2>(instance).*ptr)(std::get<Is>(std::move(args))...);
}
template <typename T, typename Ret, typename>
struct expr_constructor {
  template <typename U>
  static function<promise<Ret>(context<T>&)> apply(capture_helper<U>&& _val) {
    return [_val](context<T>& ctx) mutable {
      try {
        return resolve(std::move(_val.borrow())(ctx));
      } catch (...) {
        return reject<Ret>(std::current_exception());
      }
    };
  }
};
template <typename T, typename Ret>
struct expr_constructor<T, Ret, void> {
  template <typename U>
  static function<promise<void>(context<T>&)> apply(capture_helper<U>&& _val) {
    return [_val](context<T>& ctx) mutable {
      try {
        std::move(_val.borrow())(ctx);
        return resolve();
      } catch (...) {
        return reject<Ret>(std::current_exception());
      }
    };
  }
};
/**
 * @brief 可重用的 DSL 表达式对象。
 *
 * @tparam T   上下文的类型。
 * @tparam Ret 表达式的返回类型。
 */
template <typename T, typename Ret>
struct basic_expr {
  basic_expr() = delete;
  template <typename U, typename = typename std::enable_if<
                            (!std::is_same<typename std::decay<U>::type,
                                           basic_expr<T, Ret>>::value)>::type>
  basic_expr(U&& fn) : ptr(std::make_shared<_expr>(std::forward<U>(fn))) {}
  basic_expr(const basic_expr& rhs) : ptr(rhs.ptr) {}
  basic_expr(basic_expr&& rhs) : ptr(std::move(rhs.ptr)) {}
  basic_expr& operator=(const basic_expr& rhs) {
    ptr = rhs.ptr;
    return *this;
  }
  basic_expr& operator=(basic_expr&& rhs) {
    ptr = std::move(rhs.ptr);
    return *this;
  }
  /**
   * @brief 实际运算这个 expr。
   *
   * @param ctx 上下文。
   * @return promise<Ret> expr 的返回值。
   */
  promise<Ret> apply(context<T>& ctx) const { return ptr->_fn(ctx); }

 private:
  struct _expr {
    _expr() = delete;
    template <typename U, typename RetT = decltype(std::declval<U>()(
                              std::declval<context<T>&>()))>
    _expr(U&& fn) {
      auto _val = capture(std::forward<U>(fn));
      _fn = expr_constructor<T, Ret, RetT>::apply(std::move(_val));
    }
    template <typename U, typename Decay = typename std::decay<U>::type,
              typename = typename std::enable_if<
                  ((std::is_same<Decay, Ret>::value) ||
                   (std::is_convertible<Decay, Ret>::value))>::type>
    _expr(U&& val) {
      auto _val = capture(Ret(std::forward<U>(val)));
      _fn = [_val](context<T>&) mutable {
        return resolve(std::move(_val.borrow()));
      };
    }
    _expr(const _expr&) = delete;
    /**
     * @brief 实际运算这个 expr。
     *
     * @param ctx 上下文。
     * @return promise<Ret> expr 的返回值。
     */
    promise<Ret> apply(context<T>& ctx) const { return _fn(ctx); }
    function<promise<Ret>(context<T>&)> _fn;
  };
  std::shared_ptr<_expr> ptr;
};
template <typename T, typename Ret, typename = void>
struct expr : basic_expr<T, Ret> {
  using basic_expr<T, Ret>::basic_expr;
};
template <typename>
struct _is_expr_impl : std::false_type {};
template <typename Ctx, typename T>
struct _is_expr_impl<expr<Ctx, T>> : std::true_type {};
template <typename T>
using is_expr = _is_expr_impl<typename std::decay<T>::type>;
template <typename>
struct _is_asyncfn_impl : std::false_type {};
template <typename T>
struct _is_asyncfn_impl<asyncfn<T>> : std::true_type {};
template <typename T>
using is_asyncfn = _is_asyncfn_impl<typename std::decay<T>::type>;
template <typename T, typename Ret>
struct expr<T, Ret,
            typename std::enable_if<!(std::is_void<Ret>::value ||
                                      std::is_scalar<Ret>::value ||
                                      std::is_pointer<Ret>::value)>::type>
    : basic_expr<T, Ret> {
  using basic_expr<T, Ret>::basic_expr;
  template <typename MemberT>
  expr<T, MemberT> get(MemberT Ret::*ptr) {
    auto self = *this;
    return expr<T, MemberT>([self, ptr](context<T>& ctx) {
      return self.apply(ctx).then(
          [ptr](Ret&& t) { return std::move((&t).*ptr); });
    });
  }
#define AWACORN_ASYNC_CALL(type)                                         \
  template <typename RetT, typename... Args, typename... Args2>          \
  expr<T, RetT> call(type, Args2&&... args) {                            \
    auto self = *this;                                                   \
    auto _args = capture(std::make_tuple(std::forward<Args2>(args)...)); \
    return expr<T, RetT>([self, ptr, _args](context<T>& ctx) mutable {   \
      return self.apply(ctx).then([ptr, _args](Ret&& t) mutable {        \
        return apply(ptr, std::move(t), std::move(_args.borrow()),       \
                     make_index_sequence<sizeof...(Args2)>());           \
      });                                                                \
    });                                                                  \
  }
  AWACORN_ASYNC_CALL(RetT (Ret::*ptr)(Args...))
  AWACORN_ASYNC_CALL(RetT (Ret::*ptr)(Args...) const)
  AWACORN_ASYNC_CALL(RetT (Ret::*ptr)(Args...) &)
  AWACORN_ASYNC_CALL(RetT (Ret::*ptr)(Args...) const&)
  AWACORN_ASYNC_CALL(RetT (Ret::*ptr)(Args...) &&)
  AWACORN_ASYNC_CALL(RetT (Ret::*ptr)(Args...) const&&)
#undef AWACORN_ASYNC_CALL
};
template <typename T, typename Ret>
struct expr<T, Ret*,
            typename std::enable_if<(std::is_void<Ret>::value ||
                                     std::is_scalar<Ret>::value)>::type>
    : basic_expr<T, Ret*> {
  using basic_expr<T, Ret*>::basic_expr;
  expr<T, Ret> deref() {
    auto self = *this;
    return expr<T, Ret*>([self](context<T>& ctx) mutable {
      return self.apply(ctx).then([](Ret* t) mutable { return *t; });
    });
  }
  template <typename T2,
            typename = typename std::enable_if<!is_expr<T2>::value>::type>
  expr<T, Ret*> operator=(T2&& v) {
    auto self = *this;
    auto _v = capture(std::forward<T2>(v));
    return expr<T, Ret*>([self, _v](context<T>& ctx) mutable {
      return self.apply(ctx).then(
          [_v](Ret* t) mutable { return &(*t = std::move(_v.borrow())); });
    });
  }
  template <typename T2>
  expr<T, Ret*> operator=(const expr<T, T2>& v) {
    auto self = *this;
    return expr<T, Ret*>([self, v](context<T>& ctx) {
      return self.apply(ctx).then([&ctx, v](Ret* t) {
        return v.apply(ctx).then(
            [t](T2&& v2) { return &(*t = std::move(v2)); });
      });
    });
  }
};
template <typename T, typename Ret>
struct expr<T, Ret*,
            typename std::enable_if<!(std::is_void<Ret>::value ||
                                      std::is_scalar<Ret>::value)>::type>
    : basic_expr<T, Ret*> {
  using basic_expr<T, Ret*>::basic_expr;
  expr<T, Ret> deref() {
    auto self = *this;
    return expr<T, Ret>([self](context<T>& ctx) mutable {
      return self.apply(ctx).then([](Ret* t) mutable { return *t; });
    });
  }
  template <typename T2,
            typename = typename std::enable_if<!is_expr<T2>::value>::type>
  expr<T, Ret*> operator=(T2&& v) {
    auto self = *this;
    auto _v = capture(Ret(std::forward<T2>(v)));
    return expr<T, Ret*>([self, _v](context<T>& ctx) mutable {
      return self.apply(ctx).then(
          [_v](Ret* t) mutable { return &(*t = std::move(_v.borrow())); });
    });
  }
  template <typename T2>
  expr<T, Ret*> operator=(const expr<T, T2>& v) {
    auto self = *this;
    return expr<T, Ret*>([self, v](context<T>& ctx) {
      return self.apply(ctx).then([&ctx, v](Ret* t) {
        return v.apply(ctx).then(
            [t](T2&& v2) { return &(*t = std::move(v2)); });
      });
    });
  }
  template <typename MemberT>
  expr<T, MemberT*> get(MemberT Ret::*ptr) {
    auto self = *this;
    return expr<T, MemberT*>([self, ptr](context<T>& ctx) {
      return self.apply(ctx).then([ptr](Ret* t) { return &t.*ptr; });
    });
  }
#define AWACORN_ASYNC_CALL(type)                                         \
  template <typename RetT, typename... Args, typename... Args2>          \
  expr<T, RetT> call(type, Args2&&... args) {                            \
    auto self = *this;                                                   \
    auto _args = capture(std::make_tuple(std::forward<Args2>(args)...)); \
    return expr<T, RetT>([self, ptr, _args](context<T>& ctx) mutable {   \
      return self.apply(ctx).then([ptr, _args](Ret* t) mutable {         \
        return apply(ptr, *t, std::move(_args.borrow()),                 \
                     make_index_sequence<sizeof...(Args2)>());           \
      });                                                                \
    });                                                                  \
  }
#define AWACORN_ASYNC_CALL_RVALUE(type)                                  \
  template <typename RetT, typename... Args, typename... Args2>          \
  expr<T, RetT> call(type, Args2&&... args) {                            \
    auto self = *this;                                                   \
    auto _args = capture(std::make_tuple(std::forward<Args2>(args)...)); \
    return expr<T, RetT>([self, ptr, _args](context<T>& ctx) mutable {   \
      return self.apply(ctx).then([ptr, _args](Ret* t) mutable {         \
        return apply(ptr, std::move(*t), std::move(_args.borrow()),      \
                     make_index_sequence<sizeof...(Args2)>());           \
      });                                                                \
    });                                                                  \
  }
  AWACORN_ASYNC_CALL(RetT (Ret::*ptr)(Args...))
  AWACORN_ASYNC_CALL(RetT (Ret::*ptr)(Args...) const)
  AWACORN_ASYNC_CALL(RetT (Ret::*ptr)(Args...) &)
  AWACORN_ASYNC_CALL(RetT (Ret::*ptr)(Args...) const&)
  AWACORN_ASYNC_CALL_RVALUE(RetT (Ret::*ptr)(Args...) &&)
  AWACORN_ASYNC_CALL_RVALUE(RetT (Ret::*ptr)(Args...) const&&)
#undef AWACORN_ASYNC_CALL
#undef AWACORN_ASYNC_CALL_RVALUE
};
// 运算符宏
namespace {
// 二元运算符
#define AWACORN_ASYNC_EXPR_BINARY(op)                                          \
  template <typename Ctx, typename T, typename T2,                             \
            typename V = typename std::decay<T2>::type,                        \
            typename = typename std::enable_if<!(                              \
                is_expr<T2>::value || is_asyncfn<T2>::value)>::type>           \
  auto operator op(const expr<Ctx, T>& v, T2&& v2)                             \
      ->expr<Ctx, decltype(std::declval<T>() op std::declval<V>())> {          \
    using Ret = decltype(std::declval<T>() op std::declval<V>());              \
    auto _v2 = capture(std::forward<V>(v2));                                   \
    return expr<Ctx, Ret>([v, _v2](context<Ctx>& ctx) mutable {                \
      return v.apply(ctx).then([_v2](T&& v) mutable {                          \
        return std::move(v) op std::move(_v2.borrow());                        \
      });                                                                      \
    });                                                                        \
  }                                                                            \
  template <typename Ctx, typename T, typename T2,                             \
            typename V = typename std::decay<T>::type,                         \
            typename = typename std::enable_if<!(is_expr<T>::value ||          \
                                                 is_asyncfn<T>::value)>::type> \
  auto operator op(T&& v, const expr<Ctx, T2>& v2)                             \
      ->expr<Ctx, decltype(std::declval<V>() op std::declval<T2>())> {         \
    using Ret = decltype(std::declval<V>() op std::declval<T2>());             \
    auto _v = capture(std::forward<V>(v));                                     \
    return expr<Ctx, Ret>([_v, v2](context<Ctx>& ctx) mutable {                \
      return v2.apply(ctx).then([_v](T2&& v2) mutable {                        \
        return std::move(_v.borrow()) op std::move(v2);                        \
      });                                                                      \
    });                                                                        \
  }                                                                            \
  template <typename Ctx, typename T, typename T2>                             \
  auto operator op(const expr<Ctx, T>& v, const expr<Ctx, T2>& v2)             \
      ->expr<Ctx, decltype(std::declval<T>() op std::declval<T2>())> {         \
    using Ret = decltype(std::declval<T>() op std::declval<T2>());             \
    return expr<Ctx, Ret>([v, v2](context<Ctx>& ctx) {                         \
      return v.apply(ctx).then([&ctx, v2](T&& v) {                             \
        auto _v = capture(std::move(v));                                       \
        return v2.apply(ctx).then([_v](T2&& v2) mutable {                      \
          return std::move(_v.borrow()) op std::move(v2);                      \
        });                                                                    \
      });                                                                      \
    });                                                                        \
  }                                                                            \
  template <typename Ctx, typename T, typename T2,                             \
            typename V = typename std::decay<T2>::type,                        \
            typename = typename std::enable_if<!(                              \
                is_expr<T2>::value || is_asyncfn<T2>::value)>::type>           \
  auto operator op(const expr<Ctx, T*>& v, T2&& v2)                            \
      ->expr<Ctx, decltype(std::declval<T>() op std::declval<V>())> {          \
    using Ret = decltype(std::declval<T>() op std::declval<V>());              \
    auto _v2 = capture(std::forward<V>(v2));                                   \
    return expr<Ctx, Ret>([v, _v2](context<Ctx>& ctx) mutable {                \
      return v.apply(ctx).then(                                                \
          [_v2](T* v) mutable { return (*v)op std::move(_v2.borrow()); });     \
    });                                                                        \
  }                                                                            \
  template <typename Ctx, typename T, typename T2,                             \
            typename V = typename std::decay<T>::type,                         \
            typename = typename std::enable_if<!(is_expr<T>::value ||          \
                                                 is_asyncfn<T>::value)>::type> \
  auto operator op(T&& v, const expr<Ctx, T2*>& v2)                            \
      ->expr<Ctx, decltype(std::declval<V>() op std::declval<T2>())> {         \
    using Ret = decltype(std::declval<V>() op std::declval<T2>());             \
    auto _v = capture(std::forward<V>(v));                                     \
    return expr<Ctx, Ret>([_v, v2](context<Ctx>& ctx) mutable {                \
      return v2.apply(ctx).then(                                               \
          [_v](T2* v2) mutable { return std::move(_v.borrow()) op(*v2); });    \
    });                                                                        \
  }                                                                            \
  template <typename Ctx, typename T, typename T2>                             \
  auto operator op(const expr<Ctx, T*>& v, const expr<Ctx, T2*>& v2)           \
      ->expr<Ctx, decltype(std::declval<T>() op std::declval<T2>())> {         \
    using Ret = decltype(std::declval<T>() op std::declval<T2>());             \
    return expr<Ctx, Ret>([v, v2](context<Ctx>& ctx) {                         \
      return v.apply(ctx).then([&ctx, v2](T* v) {                              \
        return v2.apply(ctx).then(                                             \
            [v](T2* v2) mutable { return (*v)op(*v2); });                      \
      });                                                                      \
    });                                                                        \
  }
// 二元运算符(引用)
#define AWACORN_ASYNC_EXPR_BINARY_REF(op)                                     \
  template <typename Ctx, typename T, typename T2,                            \
            typename V = typename std::decay<T2>::type,                       \
            typename = typename std::enable_if<!(                             \
                is_expr<T2>::value || is_asyncfn<T2>::value)>::type>          \
  auto operator op(const expr<Ctx, T*>& v, T2&& v2)                           \
      ->expr<Ctx, decltype(std::declval<T>() op std::declval<V>())*> {        \
    using Ret = decltype(std::declval<T>() op std::declval<V>());             \
    auto _v2 = capture(std::forward<V>(v2));                                  \
    return expr<Ctx, Ret*>([v, _v2](context<Ctx>& ctx) mutable {              \
      return v.apply(ctx).then(                                               \
          [_v2](T* v) mutable { return &((*v)op std::move(_v2.borrow())); }); \
    });                                                                       \
  }                                                                           \
  template <typename Ctx, typename T, typename T2>                            \
  auto operator op(const expr<Ctx, T*>& v, const expr<Ctx, T2*>& v2)          \
      ->expr<Ctx, decltype(std::declval<T>() op std::declval<T2>())*> {       \
    using Ret = decltype(std::declval<T>() op std::declval<T2>())*;           \
    return expr<Ctx, Ret*>([v, v2](context<Ctx>& ctx) {                       \
      return v.apply(ctx).then([&ctx, v2](T* v) {                             \
        return v2.apply(ctx).then(                                            \
            [v](T2* v2) mutable { return &((*v)op(*v2)); });                  \
      });                                                                     \
    });                                                                       \
  }
// 一元前缀运算符
#define AWACORN_ASYNC_EXPR_UNARY_PREFIX(op)                                    \
  template <typename Ctx, typename T>                                          \
  auto operator op(const expr<Ctx, T>& v)                                      \
      ->expr<Ctx, decltype(op std::declval<T>())> {                            \
    using Ret = decltype(op std::declval<T>());                                \
    return expr<Ctx, Ret>([v](context<Ctx>& ctx) mutable {                     \
      return v.apply(ctx).then([](T&& v) mutable { return op std::move(v); }); \
    });                                                                        \
  }                                                                            \
  template <typename Ctx, typename T>                                          \
  auto operator op(const expr<Ctx, T*>& v)                                     \
      ->expr<Ctx, decltype(op std::declval<T>())> {                            \
    using Ret = decltype(op std::declval<T>());                                \
    return expr<Ctx, Ret>([v](context<Ctx>& ctx) {                             \
      return v.apply(ctx).then([](T* v) { return op(*v); });                   \
    });                                                                        \
  }
// 一元前缀运算符(引用)
#define AWACORN_ASYNC_EXPR_UNARY_PREFIX_REF(op)                 \
  template <typename Ctx, typename T>                           \
  auto operator op(const expr<Ctx, T*>& v)                      \
      ->expr<Ctx, decltype(op std::declval<T>())*> {            \
    using Ret = decltype(op std::declval<T>());                 \
    return expr<Ctx, Ret*>([v](context<Ctx>& ctx) {             \
      return v.apply(ctx).then([](T* v) { return &(op(*v)); }); \
    });                                                         \
  }
// 一元后缀运算符(引用)
#define AWACORN_ASYNC_EXPR_UNARY_SUFFIX(op)                  \
  template <typename Ctx, typename T>                        \
  auto operator op(const expr<Ctx, T*>& v, int)              \
      ->expr<Ctx, decltype(std::declval<T>() op)> {          \
    using Ret = decltype(std::declval<T>() op);              \
    return expr<Ctx, Ret>([v](context<Ctx>& ctx) {           \
      return v.apply(ctx).then([](T* v) { return (*v)op; }); \
    });                                                      \
  }
};  // namespace
/// 二元 (按值)
// 四则运算
AWACORN_ASYNC_EXPR_BINARY(+)
AWACORN_ASYNC_EXPR_BINARY(-)
AWACORN_ASYNC_EXPR_BINARY(*)
AWACORN_ASYNC_EXPR_BINARY(/)
AWACORN_ASYNC_EXPR_BINARY(%)
// 逻辑运算
AWACORN_ASYNC_EXPR_BINARY(==)
AWACORN_ASYNC_EXPR_BINARY(!=)
AWACORN_ASYNC_EXPR_BINARY(<)
AWACORN_ASYNC_EXPR_BINARY(>)
AWACORN_ASYNC_EXPR_BINARY(<=)
AWACORN_ASYNC_EXPR_BINARY(>=)
AWACORN_ASYNC_EXPR_BINARY(&&)
AWACORN_ASYNC_EXPR_BINARY(||)
// 位运算
AWACORN_ASYNC_EXPR_BINARY(<<)
AWACORN_ASYNC_EXPR_BINARY(>>)
AWACORN_ASYNC_EXPR_BINARY(&)
AWACORN_ASYNC_EXPR_BINARY(|)
AWACORN_ASYNC_EXPR_BINARY(^)
/// 二元 (按引用)
// 四则运算
AWACORN_ASYNC_EXPR_BINARY_REF(+=)
AWACORN_ASYNC_EXPR_BINARY_REF(-=)
AWACORN_ASYNC_EXPR_BINARY_REF(*=)
AWACORN_ASYNC_EXPR_BINARY_REF(/=)
AWACORN_ASYNC_EXPR_BINARY_REF(%=)
// 位运算
AWACORN_ASYNC_EXPR_BINARY_REF(&=)
AWACORN_ASYNC_EXPR_BINARY_REF(|=)
AWACORN_ASYNC_EXPR_BINARY_REF(^=)
AWACORN_ASYNC_EXPR_BINARY_REF(<<=)
AWACORN_ASYNC_EXPR_BINARY_REF(>>=)
/// 一元前缀 (按值)
// 符号
AWACORN_ASYNC_EXPR_UNARY_PREFIX(+)
AWACORN_ASYNC_EXPR_UNARY_PREFIX(-)
AWACORN_ASYNC_EXPR_UNARY_PREFIX(!)
AWACORN_ASYNC_EXPR_UNARY_PREFIX(~)
// 取地址/解引用
AWACORN_ASYNC_EXPR_UNARY_PREFIX(&)
AWACORN_ASYNC_EXPR_UNARY_PREFIX(*)
/// 一元前缀 (按引用)
// 自增/减 操作
AWACORN_ASYNC_EXPR_UNARY_PREFIX_REF(++)
AWACORN_ASYNC_EXPR_UNARY_PREFIX_REF(--)
/// 一元后缀 (按值)
// 自增/减 操作
AWACORN_ASYNC_EXPR_UNARY_SUFFIX(++)
AWACORN_ASYNC_EXPR_UNARY_SUFFIX(--)

#undef AWACORN_ASYNC_EXPR_BINARY
#undef AWACORN_ASYNC_EXPR_BINARY_REF
#undef AWACORN_ASYNC_EXPR_UNARY_PREFIX
#undef AWACORN_ASYNC_EXPR_UNARY_PREFIX_REF
#undef AWACORN_ASYNC_EXPR_UNARY_SUFFIX
template <typename T>
class basic_fn {
  template <typename Ret>
  using expr = detail::expr<T, Ret>;
  template <typename... Args, std::size_t... Is>
  static inline detail::expr<T, void> _stmt_apply(
      std::tuple<Args...>&& args, detail::index_sequence<Is...>) {
    return _stmt(std::get<Is>(std::move(args))...);
  }
  template <typename ExprT, typename... Args>
  inline static expr<void> _stmt(const expr<ExprT>& v, Args&&... args) {
    auto _args = capture(std::make_tuple(std::forward<Args>(args)...));
    return expr<void>([v, _args](context<T>& ctx) mutable {
      return v.apply(ctx).then([&ctx, _args](ExprT&&) mutable {
        return _stmt_apply(std::move(_args.borrow()),
                           make_index_sequence<sizeof...(Args)>())
            .apply(ctx);
      });
    });
  }
  template <typename... Args>
  inline static expr<void> _stmt(const expr<void>& v, Args&&... args) {
    auto _args = capture(std::make_tuple(std::forward<Args>(args)...));
    return expr<void>([v, _args](context<T>& ctx) mutable {
      return v.apply(ctx).then([&ctx, _args]() mutable {
        return _stmt_apply(std::move(_args.borrow()),
                           make_index_sequence<sizeof...(Args)>())
            .apply(ctx);
      });
    });
  }
  template <typename ExprT>
  inline static expr<void> _stmt(const expr<ExprT>& v) {
    return expr<void>(
        [v](context<T>& ctx) { return v.apply(ctx).then([&ctx](ExprT&&) {}); });
  }
  inline static expr<void> _stmt(const expr<void>& v) {
    return expr<void>([v](context<T>& ctx) { return v.apply(ctx); });
  }

 public:
  template <typename U>
  inline static expr<U> await(const expr<promise<U>>& v) {
    return expr<U>([v](context<T>& ctx) {
      return v.apply(ctx).then([](promise<U>&& v) {
        return v;
      });  // 这里返回了一个 promise<U> 所以最终的类型是 expr<U>
    });
  }
  inline static expr<void> await(const expr<promise<void>>& v) {
    return expr<void>([v](context<T>& ctx) {
      return v.apply(ctx).then([](promise<void>&& v) { return v; });
    });
  }
  inline static expr<void> error(const expr<std::exception_ptr>& v) {
    return expr<void>([v](context<T>& ctx) {
      return v.apply(ctx).then([&ctx](std::exception_ptr&& v) {
        return reject<void>(std::move(v));
      });
    });
  }
  // TODO: capture & handler fn & scope
  template <typename... Args>
  inline static expr<std::unique_ptr<std::exception_ptr>> capture(
      Args&&... args) {
    auto _args = detail::capture(std::make_tuple(std::forward<Args>(args)...));
    return expr<void>([_args](context<T>& ctx) mutable {
      auto sub_ctx =
          detail::capture(std::unique_ptr<context<T>>(new context<T>(&ctx)));
      auto pm = promise<std::unique_ptr<std::exception_ptr>>();
      auto temp = _stmt_apply(std::move(_args.borrow()),
                              make_index_sequence<sizeof...(Args)>())
                      .apply(*sub_ctx.borrow());
      temp.then([sub_ctx, pm]() mutable {
        // 保证 ctx 不被提前析构
        pm.resolve(nullptr);
        sub_ctx.borrow().reset();
      });
      temp.error([pm](std::exception_ptr&& e) {
        pm.resolve(std::unique_ptr<std::exception_ptr>(
            new std::exception_ptr(std::move(e))));
      });
      return pm;
    });
  }
  template <typename... Args>
  inline static expr<void> stmt(Args&&... args) {
    auto _args = capture(std::make_tuple(std::forward<Args>(args)...));
    return expr<void>([_args](context<T>& ctx) mutable {
      auto sub_ctx = capture(std::unique_ptr<context<T>>(new context<T>(&ctx)));
      return _stmt_apply(std::move(_args.borrow()),
                         make_index_sequence<sizeof...(Args)>())
          .apply(*sub_ctx.borrow())
          .then([sub_ctx]() mutable {
            // 保证 ctx 不被提前析构
            sub_ctx.borrow().reset();
          });
    });
  }
  inline static expr<bool> cond(const expr<bool>& v,
                                const expr<void>& if_true) {
    return expr<bool>([v, if_true](context<T>& ctx) {
      return v.apply(ctx).then([&ctx, if_true](bool v) {
        if (v) return if_true.apply(ctx).then([]() { return true; });
        return resolve(false);
      });
    });
  }
  inline static expr<bool> cond(const expr<bool>& v, const expr<void>& if_true,
                                const expr<void>& if_false) {
    return expr<bool>([v, if_true, if_false](context<T>& ctx) {
      return v.apply(ctx).then([&ctx, if_true, if_false](bool v) {
        if (v) return if_true.apply(ctx).then([]() { return true; });
        return if_false.apply(ctx).then([]() { return false; });
      });
    });
  }
  // TODO: lazy create
  template <typename U>
  inline static expr<U*> var(const std::string& name) {
    auto cache = detail::capture(std::unique_ptr<U*>(new U*(nullptr)));
    return expr<U*>([name, cache](context<T>& ctx) mutable {
      if (!(*cache.borrow())) {
        *cache.borrow() = (U*)ctx.find_var(name);
        if (!(*cache.borrow())) {
          *cache.borrow() =
              (U*)ctx.local
                  .insert(
                      {name, std::unique_ptr<void, void (*)(void*)>(
                                 new U(), [](void* ptr) { ((U*)ptr)->~U(); })})
                  .first->second.get();
        }
      }
      return *cache.borrow();
    });
  }

 protected:
  basic_fn() = default;
};
};  // namespace detail
#define AWACORN_ASYNC_CONTEXT_HANDLE_RET_VOID \
  void handle_ret() const {                   \
    if (parent) return parent->handle_ret();  \
    result->resolve();                        \
  }
#define AWACORN_ASYNC_CONTEXT_HANDLE_RET_NONVOID(T)      \
  void handle_ret(const T& v) const {                    \
    if (parent) return parent->handle_ret(v);            \
    result->resolve(v);                                  \
  }                                                      \
  void handle_ret(T&& v) const {                         \
    if (parent) return parent->handle_ret(std::move(v)); \
    result->resolve(std::move(v));                       \
  }

#define AWACORN_ASYNC_CONTEXT_RAW(T, specialize_code, handle_ret_code)      \
  struct context specialize_code {                                          \
    const promise<T>& get_result() const noexcept {                         \
      if (parent) return parent->get_result();                              \
      return *result;                                                       \
    }                                                                       \
    handle_ret_code void unhandled_err(const std::exception_ptr& v) const { \
      if (parent) {                                                         \
        parent->unhandled_err(v);                                           \
      } else {                                                              \
        result->reject(v);                                                  \
      }                                                                     \
    }                                                                       \
    void unhandled_err(std::exception_ptr&& v) const {                      \
      if (parent) {                                                         \
        parent->unhandled_err(std::move(v));                                \
      } else {                                                              \
        result->reject(std::move(v));                                       \
      }                                                                     \
    }                                                                       \
                                                                            \
   private:                                                                 \
    context() = delete;                                                     \
    context(const context<T>&) = delete;                                    \
    context(context<T>* parent) : parent(parent) {                          \
      if (!parent) {                                                        \
        result = std::unique_ptr<promise<T>>(new promise<T>());             \
      }                                                                     \
    }                                                                       \
    void* find_var(const std::string& name) {                               \
      auto it = local.find(name);                                           \
      if (it == local.cend()) {                                             \
        return parent ? parent->find_var(name) : nullptr;                   \
      } else {                                                              \
        return it->second.get();                                            \
      }                                                                     \
    }                                                                       \
    friend class detail::basic_fn<T>;                                       \
    friend struct asyncfn<T>;                                               \
    std::unique_ptr<promise<T>> result;                                     \
    std::unordered_map<std::string, std::unique_ptr<void, void (*)(void*)>> \
        local;                                                              \
    context<T>* parent;                                                     \
  };
#define AWACORN_ASYNC_CONTEXT(T, specialize_code, handle_ret_code) \
  AWACORN_ASYNC_CONTEXT_RAW(T, specialize_code, handle_ret_code)
/**
 * @brief 异步上下文对象。这个上下文只能通过 awacorn::async 获取。
 *
 * @tparam T context 的返回值。
 */
template <typename T>
AWACORN_ASYNC_CONTEXT(T, , AWACORN_ASYNC_CONTEXT_HANDLE_RET_NONVOID(T))
template <>
AWACORN_ASYNC_CONTEXT(void, <void>, AWACORN_ASYNC_CONTEXT_HANDLE_RET_VOID)
#undef AWACORN_ASYNC_CONTEXT
#undef AWACORN_ASYNC_CONTEXT_HANDLE_RET_VOID
#undef AWACORN_ASYNC_CONTEXT_HANDLE_RET_NONVOID
#undef AWACORN_ASYNC_CONTEXT_RAW
#define AWACORN_ASYNC_ASYNCFN_RET_VOID                               \
  inline static expr<void> ret() {                                   \
    return expr<void>([](context<void>& ctx) { ctx.handle_ret(); }); \
  }
#define AWACORN_ASYNC_ASYNCFN_RET_NONVOID(T)                \
  inline static expr<void> ret(const expr<T>& v) {          \
    return expr<void>([v](context<T>& ctx) {                \
      return v.apply(ctx).then(                             \
          [&ctx](T&& v) { ctx.handle_ret(std::move(v)); }); \
    });                                                     \
  }
#define AWACORN_ASYNC_ASYNCFN_NORETURN_NONVOID \
  ctx->unhandled_err(std::make_exception_ptr(  \
      std::logic_error("control reaches end of non-void function")));
#define AWACORN_ASYNC_ASYNCFN_NORETURN_VOID ctx->handle_ret();
#define AWACORN_ASYNC_ASYNCFN_RAW(T, specialize_code, ret_code,        \
                                  noreturn_behavior)                   \
  struct asyncfn specialize_code : public detail::basic_fn<T> {        \
    template <typename Ret>                                            \
    using expr = detail::expr<T, Ret>;                                 \
    template <typename ExprT>                                          \
    asyncfn& operator<<(const expr<ExprT>& v) {                        \
      chain.emplace_back(expr<void>([v](context<T>& ctx) {             \
        return v.apply(ctx).then([](ExprT&&) {});                      \
      }));                                                             \
      return *this;                                                    \
    }                                                                  \
    asyncfn& operator<<(const expr<void>& v) {                         \
      chain.push_back(v);                                              \
      return *this;                                                    \
    }                                                                  \
    ret_code                                                           \
                                                                       \
        private : asyncfn() = default;                                 \
    friend class detail::basic_fn<T>;                                  \
    template <typename U, typename Ret>                                \
    friend promise<Ret> async(U&&);                                    \
    promise<T> apply() const {                                         \
      auto ctx = std::shared_ptr<context<T>>(new context<T>(nullptr)); \
      auto ptr = ctx.get();                                            \
      auto pm = resolve();                                             \
      for (auto&& it : chain) {                                        \
        pm = pm.then([ptr, it]() {                                     \
          if (ptr->get_result().status() == status_t::pending)         \
            return it.apply(*ptr);                                     \
          return resolve();                                            \
        });                                                            \
        pm.error([ptr](std::exception_ptr&& e) {                       \
          ptr->unhandled_err(std::move(e));                            \
        });                                                            \
      }                                                                \
      pm.then([ctx]() mutable {                                        \
        if (ctx->get_result().status() == status_t::pending) {         \
          noreturn_behavior                                            \
        }                                                              \
      });                                                              \
      return ptr->get_result();                                        \
    }                                                                  \
    std::list<expr<void>> chain;                                       \
  };
#define AWACORN_ASYNC_ASYNCFN(T, specialize_code, ret_code, noreturn_behavior) \
  AWACORN_ASYNC_ASYNCFN_RAW(T, specialize_code, ret_code, noreturn_behavior)
/**
 * @brief 异步函数。这个对象只能通过 awacorn::async 获取。
 *
 * @tparam T context 的返回值。
 */
template <typename T>
AWACORN_ASYNC_ASYNCFN(T, , AWACORN_ASYNC_ASYNCFN_RET_NONVOID(T),
                      AWACORN_ASYNC_ASYNCFN_NORETURN_NONVOID)
template <>
AWACORN_ASYNC_ASYNCFN(void, <void>, AWACORN_ASYNC_ASYNCFN_RET_VOID,
                      AWACORN_ASYNC_ASYNCFN_NORETURN_VOID)
#undef AWACORN_ASYNC_ASYNCFN_RET_NONVOID
#undef AWACORN_ASYNC_ASYNCFN_RET_VOID
#undef AWACORN_ASYNC_ASYNCFN_NORETURN_NONVOID
#undef AWACORN_ASYNC_ASYNCFN_NORETURN_VOID
#undef AWACORN_ASYNC_ASYNCFN_RAW
#undef AWACORN_ASYNC_ASYNCFN
// template <typename T>
/**
 * @brief 开始异步函数。
 *
 * @tparam U 实际逻辑函数的类型。
 * @param fn 实际逻辑函数。
 * @return promise<T> 返回的 promise。
 */
template <
    typename U,
    typename Ret = typename detail::remove_asyncfn<
        typename std::decay<typename std::tuple_element<
            0, typename detail::result_of<U>::arg_type>::type>::type>::type>
promise<Ret> async(U&& fn) {
  auto ctx = asyncfn<Ret>();
  fn(ctx);
  return ctx.apply();
}
};  // namespace awacorn
#endif
#endif