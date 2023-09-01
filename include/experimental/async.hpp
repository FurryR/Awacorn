#ifndef _AWACORN_EXPERIMENTAL_ASYNC
#define _AWACORN_EXPERIMENTAL_ASYNC
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <utility>
#if __cplusplus >= 201101L
/**
 * Project Awacorn 基于 MIT 协议开源。
 * Copyright(c) 凌 2023.
 */

#include <memory>

#include "../detail/capture.hpp"
#include "../detail/function.hpp"
#include "../promise.hpp"

namespace awacorn {
template <typename>
struct context;
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
struct remove_context {
  using type = T;
};
template <typename T>
struct remove_context<context<T>> {
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
  promise<Ret> apply(context<T>& ctx) const { return ptr->fn(ctx); }

 private:
  struct _expr {
    _expr() = delete;
    template <typename U, typename = typename std::enable_if<
                              (std::is_same<decltype(std::declval<U>()(
                                                std::declval<context<T>&>())),
                                            promise<Ret>>::value)>::type>
    _expr(U&& fn) : fn(std::forward<U>(fn)) {}
    template <typename U, typename Decay = typename std::decay<U>::type,
              typename = typename std::enable_if<
                  ((std::is_same<Decay, Ret>::value) ||
                   (std::is_convertible<Decay, Ret>::value))>::type>
    _expr(U&& val) {
      auto _val = capture(Ret(std::forward<U>(val)));
      fn = [_val](context<T>&) mutable {
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
    promise<Ret> apply(context<T>& ctx) const { return fn(ctx); }
    function<promise<Ret>(context<T>&)> fn;
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
  // value
  template <typename RetT, typename... Args, typename... Args2>
  expr<T, RetT> call(RetT (Ret::*ptr)(Args...), Args2&&... args) {
    auto self = *this;
    auto _args = capture(std::make_tuple(std::forward<Args2>(args)...));
    return expr<T, RetT>([self, ptr, _args](context<T>& ctx) mutable {
      return self.apply(ctx).then([ptr, _args](Ret&& t) mutable {
        return apply(ptr, std::move(t), std::move(_args.borrow()),
                     make_index_sequence<sizeof...(Args2)>());
      });
    });
  }
  template <typename RetT, typename... Args, typename... Args2>
  expr<T, RetT> call(RetT (Ret::*ptr)(Args...) const, Args2&&... args) {
    auto self = *this;
    auto _args = capture(std::make_tuple(std::forward<Args2>(args)...));
    return expr<T, RetT>([self, ptr, _args](context<T>& ctx) mutable {
      return self.apply(ctx).then([ptr, _args](Ret&& t) mutable {
        return apply(ptr, std::move(t), std::move(_args.borrow()),
                     make_index_sequence<sizeof...(Args2)>());
      });
    });
  }
  // lref
  template <typename RetT, typename... Args, typename... Args2>
  expr<T, RetT> call(RetT (Ret::*ptr)(Args...) &, Args2&&... args) {
    auto self = *this;
    auto _args = capture(std::make_tuple(std::forward<Args2>(args)...));
    return expr<T, RetT>([self, ptr, _args](context<T>& ctx) mutable {
      return self.apply(ctx).then([ptr, _args](Ret&& t) mutable {
        return apply(ptr, std::move(t), std::move(_args.borrow()),
                     make_index_sequence<sizeof...(Args2)>());
      });
    });
  }
  template <typename RetT, typename... Args, typename... Args2>
  expr<T, RetT> call(RetT (Ret::*ptr)(Args...) const&, Args2&&... args) {
    auto self = *this;
    auto _args = capture(std::make_tuple(std::forward<Args2>(args)...));
    return expr<T, RetT>([self, ptr, _args](context<T>& ctx) mutable {
      return self.apply(ctx).then([ptr, _args](Ret&& t) mutable {
        return apply(ptr, std::move(t), std::move(_args.borrow()),
                     make_index_sequence<sizeof...(Args2)>());
      });
    });
  }
  // rref
  template <typename RetT, typename... Args, typename... Args2>
  expr<T, RetT> call(RetT (Ret::*ptr)(Args...) &&, Args2&&... args) {
    auto self = *this;
    auto _args = capture(std::make_tuple(std::forward<Args2>(args)...));
    return expr<T, RetT>([self, ptr, _args](context<T>& ctx) mutable {
      return self.apply(ctx).then([ptr, _args](Ret&& t) mutable {
        return apply(ptr, std::move(t), std::move(_args.borrow()),
                     make_index_sequence<sizeof...(Args2)>());
      });
    });
  }
  template <typename RetT, typename... Args, typename... Args2>
  expr<T, RetT> call(RetT (Ret::*ptr)(Args...) const&&, Args2&&... args) {
    auto self = *this;
    auto _args = capture(std::make_tuple(std::forward<Args2>(args)...));
    return expr<T, RetT>([self, ptr, _args](context<T>& ctx) mutable {
      return self.apply(ctx).then([ptr, _args](Ret&& t) mutable {
        return apply(ptr, std::move(t), std::move(_args.borrow()),
                     make_index_sequence<sizeof...(Args2)>());
      });
    });
  }
};
template <typename T, typename Ret>
struct expr<T, Ret*,
            typename std::enable_if<(std::is_void<Ret>::value ||
                                     std::is_scalar<Ret>::value)>::type>
    : basic_expr<T, Ret*> {
  using basic_expr<T, Ret*>::basic_expr;
  expr<T, Ret> as_value() {
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
  expr<T, Ret> as_value() {
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
  // value
  template <typename RetT, typename... Args, typename... Args2>
  expr<T, RetT> call(RetT (Ret::*ptr)(Args...), Args2&&... args) {
    auto self = *this;
    auto _args = capture(std::make_tuple(std::forward<Args2>(args)...));
    return expr<T, RetT>([self, ptr, _args](context<T>& ctx) mutable {
      return self.apply(ctx).then([ptr, _args](Ret* t) mutable {
        return apply(ptr, *t, std::move(_args.borrow()),
                     make_index_sequence<sizeof...(Args2)>());
      });
    });
  }
  template <typename RetT, typename... Args, typename... Args2>
  expr<T, RetT> call(RetT (Ret::*ptr)(Args...) const, Args2&&... args) {
    auto self = *this;
    auto _args = capture(std::make_tuple(std::forward<Args2>(args)...));
    return expr<T, RetT>([self, ptr, _args](context<T>& ctx) mutable {
      return self.apply(ctx).then([ptr, _args](Ret* t) mutable {
        return apply(ptr, *t, std::move(_args.borrow()),
                     make_index_sequence<sizeof...(Args2)>());
      });
    });
  }
  // lref
  template <typename RetT, typename... Args, typename... Args2>
  expr<T, RetT> call(RetT (Ret::*ptr)(Args...) &, Args2&&... args) {
    auto self = *this;
    auto _args = capture(std::make_tuple(std::forward<Args2>(args)...));
    return expr<T, RetT>([self, ptr, _args](context<T>& ctx) mutable {
      return self.apply(ctx).then([ptr, _args](Ret* t) mutable {
        return apply(ptr, *t, std::move(_args.borrow()),
                     make_index_sequence<sizeof...(Args2)>());
      });
    });
  }
  template <typename RetT, typename... Args, typename... Args2>
  expr<T, RetT> call(RetT (Ret::*ptr)(Args...) const&, Args2&&... args) {
    auto self = *this;
    auto _args = capture(std::make_tuple(std::forward<Args2>(args)...));
    return expr<T, RetT>([self, ptr, _args](context<T>& ctx) mutable {
      return self.apply(ctx).then([ptr, _args](Ret* t) mutable {
        return apply(ptr, *t, std::move(_args.borrow()),
                     make_index_sequence<sizeof...(Args2)>());
      });
    });
  }
  // rref
  template <typename RetT, typename... Args, typename... Args2>
  expr<T, RetT> call(RetT (Ret::*ptr)(Args...) &&, Args2&&... args) {
    auto self = *this;
    auto _args = capture(std::make_tuple(std::forward<Args2>(args)...));
    return expr<T, RetT>([self, ptr, _args](context<T>& ctx) mutable {
      return self.apply(ctx).then([ptr, _args](Ret* t) mutable {
        return apply(ptr, std::move(*t), std::move(_args.borrow()),
                     make_index_sequence<sizeof...(Args2)>());
      });
    });
  }
  template <typename RetT, typename... Args, typename... Args2>
  expr<T, RetT> call(RetT (Ret::*ptr)(Args...) const&&, Args2&&... args) {
    auto self = *this;
    auto _args = capture(std::make_tuple(std::forward<Args2>(args)...));
    return expr<T, RetT>([self, ptr, _args](context<T>& ctx) mutable {
      return self.apply(ctx).then([ptr, _args](Ret* t) mutable {
        return apply(ptr, std::move(*t), std::move(_args.borrow()),
                     make_index_sequence<sizeof...(Args2)>());
      });
    });
  }
};
// expr operator+
template <typename Ctx, typename T, typename T2,
          typename V = typename std::decay<T2>::type,
          typename = typename std::enable_if<!is_expr<T2>::value>::type>
auto operator+(const expr<Ctx, T>& v, T2&& v2)
    -> expr<Ctx, decltype(std::declval<T>() + std::declval<V>())> {
  using Ret = decltype(std::declval<T>() + std::declval<V>());
  auto _v2 = capture(std::forward<V>(v2));
  return expr<Ctx, Ret>([v, _v2](context<Ctx>& ctx) mutable {
    return v.apply(ctx).then([_v2](T&& v) mutable {
      return std::move(v) + std::move(_v2.borrow());
    });
  });
}
template <typename Ctx, typename T, typename T2,
          typename V = typename std::decay<T>::type,
          typename = typename std::enable_if<!is_expr<T>::value>::type>
auto operator+(T&& v, const expr<Ctx, T2>& v2)
    -> expr<Ctx, decltype(std::declval<V>() + std::declval<T2>())> {
  using Ret = decltype(std::declval<V>() + std::declval<T2>());
  auto _v = capture(std::forward<V>(v));
  return expr<Ctx, Ret>([_v, v2](context<Ctx>& ctx) mutable {
    return v2.apply(ctx).then([_v](T2&& v2) mutable {
      return std::move(_v.borrow()) + std::move(v2);
    });
  });
}
template <typename Ctx, typename T, typename T2>
auto operator+(const expr<Ctx, T>& v, const expr<Ctx, T2>& v2)
    -> expr<Ctx, decltype(std::declval<T>() + std::declval<T2>())> {
  using Ret = decltype(std::declval<T>() + std::declval<T2>());
  return expr<Ctx, Ret>([v, v2](context<Ctx>& ctx) {
    return v.apply(ctx).then([&ctx, v2](T&& v) {
      auto _v = capture(std::move(v));
      return v2.apply(ctx).then([_v](T2&& v2) mutable {
        return std::move(_v.borrow()) + std::move(v2);
      });
    });
  });
}
// expr * operator+
template <typename Ctx, typename T, typename T2,
          typename V = typename std::decay<T2>::type,
          typename = typename std::enable_if<!is_expr<T2>::value>::type>
auto operator+(const expr<Ctx, T*>& v, T2&& v2)
    -> expr<Ctx, decltype(std::declval<T>() + std::declval<V>())> {
  using Ret = decltype(std::declval<T>() + std::declval<V>());
  auto _v2 = capture(std::forward<V>(v2));
  return expr<Ctx, Ret>([v, _v2](context<Ctx>& ctx) mutable {
    return v.apply(ctx).then(
        [_v2](T* v) mutable { return *v + std::move(_v2.borrow()); });
  });
}
template <typename Ctx, typename T, typename T2,
          typename V = typename std::decay<T>::type,
          typename = typename std::enable_if<!is_expr<T>::value>::type>
auto operator+(T&& v, const expr<Ctx, T2*>& v2)
    -> expr<Ctx, decltype(std::declval<V>() + std::declval<T2>())> {
  using Ret = decltype(std::declval<V>() + std::declval<T2>());
  auto _v = capture(std::forward<V>(v));
  return expr<Ctx, Ret>([_v, v2](context<Ctx>& ctx) mutable {
    return v2.apply(ctx).then(
        [_v](T2* v2) mutable { return std::move(_v.borrow()) + *v2; });
  });
}
template <typename Ctx, typename T, typename T2>
auto operator+(const expr<Ctx, T*>& v, const expr<Ctx, T2*>& v2)
    -> expr<Ctx, decltype(std::declval<T>() + std::declval<T2>())> {
  using Ret = decltype(std::declval<T>() + std::declval<T2>());
  return expr<Ctx, Ret>([v, v2](context<Ctx>& ctx) {
    return v.apply(ctx).then([&ctx, v2](T* v) {
      return v2.apply(ctx).then([v](T2* v2) mutable { return *v + *v2; });
    });
  });
}
// expr operator==
template <typename Ctx, typename T, typename T2,
          typename V = typename std::decay<T2>::type,
          typename = typename std::enable_if<!is_expr<T2>::value>::type>
auto operator==(const expr<Ctx, T>& v, T2&& v2)
    -> expr<Ctx, decltype(std::declval<T>() == std::declval<V>())> {
  using Ret = decltype(std::declval<T>() == std::declval<V>());
  auto _v2 = capture(std::forward<V>(v2));
  return expr<Ctx, Ret>([v, _v2](context<Ctx>& ctx) mutable {
    return v.apply(ctx).then([_v2](T&& v) mutable {
      return std::move(v) == std::move(_v2.borrow());
    });
  });
}
template <typename Ctx, typename T, typename T2,
          typename V = typename std::decay<T>::type,
          typename = typename std::enable_if<!is_expr<T>::value>::type>
auto operator==(T&& v, const expr<Ctx, T2>& v2)
    -> expr<Ctx, decltype(std::declval<V>() == std::declval<T2>())> {
  using Ret = decltype(std::declval<V>() == std::declval<T2>());
  auto _v = capture(std::forward<V>(v));
  return expr<Ctx, Ret>([_v, v2](context<Ctx>& ctx) mutable {
    return v2.apply(ctx).then([_v](T2&& v2) mutable {
      return std::move(_v.borrow()) == std::move(v2);
    });
  });
}
template <typename Ctx, typename T, typename T2>
auto operator==(const expr<Ctx, T>& v, const expr<Ctx, T2>& v2)
    -> expr<Ctx, decltype(std::declval<T>() == std::declval<T2>())> {
  using Ret = decltype(std::declval<T>() == std::declval<T2>());
  return expr<Ctx, Ret>([v, v2](context<Ctx>& ctx) {
    return v.apply(ctx).then([&ctx, v2](T&& v) {
      auto _v = capture(std::move(v));
      return v2.apply(ctx).then([_v](T2&& v2) mutable {
        return std::move(_v.borrow()) == std::move(v2);
      });
    });
  });
}
// expr * operator==
template <typename Ctx, typename T, typename T2,
          typename V = typename std::decay<T2>::type,
          typename = typename std::enable_if<!is_expr<T2>::value>::type>
auto operator==(const expr<Ctx, T*>& v, T2&& v2)
    -> expr<Ctx, decltype(std::declval<T>() == std::declval<V>())> {
  using Ret = decltype(std::declval<T>() == std::declval<V>());
  auto _v2 = capture(std::forward<V>(v2));
  return expr<Ctx, Ret>([v, _v2](context<Ctx>& ctx) mutable {
    return v.apply(ctx).then(
        [_v2](T* v) mutable { return *v == std::move(_v2.borrow()); });
  });
}
template <typename Ctx, typename T, typename T2,
          typename V = typename std::decay<T>::type,
          typename = typename std::enable_if<!is_expr<T>::value>::type>
auto operator==(T&& v, const expr<Ctx, T2*>& v2)
    -> expr<Ctx, decltype(std::declval<V>() == std::declval<T2>())> {
  using Ret = decltype(std::declval<V>() == std::declval<T2>());
  auto _v = capture(std::forward<V>(v));
  return expr<Ctx, Ret>([_v, v2](context<Ctx>& ctx) mutable {
    return v2.apply(ctx).then(
        [_v](T2* v2) mutable { return std::move(_v.borrow()) == *v2; });
  });
}
template <typename Ctx, typename T, typename T2>
auto operator==(const expr<Ctx, T*>& v, const expr<Ctx, T2*>& v2)
    -> expr<Ctx, decltype(std::declval<T>() == std::declval<T2>())> {
  using Ret = decltype(std::declval<T>() == std::declval<T2>());
  return expr<Ctx, Ret>([v, v2](context<Ctx>& ctx) {
    return v.apply(ctx).then([&ctx, v2](T* v) {
      return v2.apply(ctx).then([v](T2* v2) mutable { return *v == *v2; });
    });
  });
}
template <typename T>
class basic_context : public std::enable_shared_from_this<context<T>> {
  template <typename... Args, std::size_t... Is>
  static inline detail::expr<T, void> _stmt_apply(
      std::tuple<Args...>&& args, detail::index_sequence<Is...>) {
    return stmt(std::get<Is>(std::move(args))...);
  }

 public:
  template <typename Ret>
  using expr = detail::expr<T, Ret>;
  template <typename U>
  inline static expr<U> await(const expr<promise<U>>& v) {
    return expr<U>([v](context<T>& ctx) {
      return v.apply(ctx).then([](promise<U>&& v) {
        return std::move(v);
      });  // 这里返回了一个 promise<U> 所以最终的类型是 expr<U>
    });
  }
  inline static expr<void> await(const expr<promise<void>>& v) {
    return expr<void>([v](context<T>& ctx) {
      return v.apply(ctx).then([](promise<void>&& v) { return std::move(v); });
    });
  }
  inline static expr<void> error(const expr<std::exception_ptr>& v) {
    return expr<void>([v](context<T>& ctx) {
      return v.apply(ctx).then([&ctx](std::exception_ptr&& v) {
        ctx.reject(std::move(v));
        return awacorn::resolve();
      });  // promise<void>
    });
  }
  template <typename ExprT, typename... Args>
  inline static expr<void> stmt(const expr<ExprT>& v, Args&&... args) {
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
  inline static expr<void> stmt(const expr<void>& v, Args&&... args) {
    auto _args = capture(std::make_tuple(std::forward<Args>(args)...));
    return expr<void>([v, _args](context<T>& ctx) {
      return v.apply(ctx).then([&ctx, _args]() {
        return _stmt_apply(std::move(_args.borrow()),
                           make_index_sequence<sizeof...(Args)>())
            .apply(ctx);
      });
    });
  }
  template <typename ExprT>
  inline static expr<void> stmt(const expr<ExprT>& v) {
    return expr<void>(
        [v](context<T>& ctx) { return v.apply(ctx).then([&ctx](ExprT&&) {}); });
  }
  inline static expr<void> stmt(const expr<void>& v) {
    return expr<void>([v](context<T>& ctx) { return v.apply(ctx); });
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
  template <typename U>
  inline static expr<U*> var(const std::string& name) {
    using Decay = typename std::decay<U>::type;
    return expr<U*>([name](context<T>& ctx) {
      return resolve((Decay*)ctx.local.at(name)
                         .get());  // 如果不存在就抛出一些错误，谁叫你不初始化的
    });
  }
  template <typename U>
  inline expr<U*> create(const std::string& name, U&& v) {
    using Decay = typename std::decay<U>::type;
    local.insert({name, std::unique_ptr<void, void (*)(void*)>(
                            new Decay(std::forward<U>(v)),
                            [](void* ptr) { ((Decay*)ptr)->~Decay(); })});
    return var<U>(name);
  }

 protected:
  basic_context() = default;
  std::unordered_map<std::string, std::unique_ptr<void, void (*)(void*)>> local;
};
};  // namespace detail
/**
 * @brief 异步上下文对象。这个上下文只能通过 awacorn::async 获取。
 *
 * @tparam T context 的返回值。
 */
template <typename T>
struct context : public detail::basic_context<T> {
  template <typename Ret>
  using expr = detail::expr<T, Ret>;
  template <typename ExprT>
  context& operator<<(const expr<ExprT>& v) {
    auto ptr = this->shared_from_this();
    chain = chain.then([ptr, v]() {
      if (ptr->get_result().status() == pending)
        return v.apply(*ptr).then([](ExprT&&) {});
      return awacorn::resolve();
    });
    return *this;
  }
  context& operator<<(const expr<void>& v) {
    auto ptr = this->shared_from_this();
    chain = chain.then([ptr, v]() {
      if (ptr->get_result().status() == pending) return v.borrow().apply(*ptr);
      return awacorn::resolve();
    });
    return *this;
  }
  inline static expr<void> ret(const expr<T>& v) {
    return expr<void>([v](context<T>& ctx) {
      return v.apply(ctx).then([&ctx](T&& v) {
        ctx.resolve(std::move(v));
        return awacorn::resolve();
      });
    });
  }
  const promise<T>& get_result() const noexcept { return result; }
  void resolve(const T& v) const { result.resolve(v); }
  void resolve(T&& v) const { result.resolve(std::move(v)); }
  void reject(const std::exception_ptr& v) const { result.reject(v); }
  void reject(std::exception_ptr&& v) const { result.reject(std::move(v)); }

 private:
  context() : chain(awacorn::resolve()){};
  friend struct detail::basic_context<T>;
  template <typename U, typename Ret>
  friend promise<Ret> async(U&&);
  static std::shared_ptr<context<T>> _create() {
    return std::shared_ptr<context<T>>(new context<T>());
  }
  promise<T> result;
  promise<void> chain;
};
template <>
struct context<void> : public detail::basic_context<void> {
  template <typename Ret>
  using expr = detail::expr<void, Ret>;
  template <typename ExprT>
  context& operator<<(const expr<ExprT>& v) {
    auto ptr = this->shared_from_this();
    chain = chain.then([ptr, v]() {
      if (ptr->get_result().status() == pending)
        return v.apply(*ptr).then([](ExprT&&) {});
      return awacorn::resolve();
    });
    return *this;
  }
  context& operator<<(const expr<void>& v) {
    auto ptr = this->shared_from_this();
    chain = chain.then([ptr, v]() {
      if (ptr->get_result().status() == pending) return v.apply(*ptr);
      return awacorn::resolve();
    });
    return *this;
  }
  inline static expr<void> ret() {
    return expr<void>([](context<void>& ctx) {
      ctx.resolve();
      return awacorn::resolve();
    });
  }
  const promise<void>& get_result() const noexcept { return result; }
  void resolve() const { result.resolve(); }
  void reject(const std::exception_ptr& v) const { result.reject(v); }
  void reject(std::exception_ptr&& v) const { result.reject(std::move(v)); }

 private:
  context() : chain(awacorn::resolve()){};
  friend class detail::basic_context<void>;
  template <typename U, typename Ret>
  friend promise<Ret> async(U&&);
  static std::shared_ptr<context<void>> _create() {
    return std::shared_ptr<context<void>>(new context<void>());
  }
  promise<void> result;
  promise<void> chain;
};
/**
 * @brief 开始异步函数。
 *
 * @tparam U 实际逻辑函数的类型。
 * @param fn 实际逻辑函数。
 * @return promise<T> 返回的 promise。
 */
template <
    typename U,
    typename Ret = typename detail::remove_context<
        typename std::decay<typename std::tuple_element<
            0, typename detail::result_of<U>::arg_type>::type>::type>::type>
promise<Ret> async(U&& fn) {
  auto ctx = context<Ret>::_create();
  fn(*ctx);
  return ctx->get_result();
}
};  // namespace awacorn
#endif
#endif