#ifndef _AWACORN_VARIANT_
#define _AWACORN_VARIANT_
#if __cplusplus >= 201101L
/**
 * Project Awacorn 基于 MIT 协议开源。
 * Copyright(c) 凌 2022.
 */
#include <type_traits>
#include <variant>

namespace awacorn {
namespace detail {
template <typename T, typename... Args>
struct contains : std::integral_constant<bool, false> {};
template <typename T, typename T2, typename... Args>
struct contains<T, T2, Args...> : contains<T, Args...> {};
template <typename T, typename... Args>
struct contains<T, T, Args...> : std::integral_constant<bool, true> {};
template <typename W, typename... Ts>
struct _make_unique_impl {
  using type = W;
};
template <template <typename...> class W, typename... Current, typename T,
          typename... Ts>
struct _make_unique_impl<W<Current...>, T, Ts...>
    : std::conditional<
          contains<T, Ts...>::value,
          typename _make_unique_impl<W<Current...>, Ts...>::type,
          typename _make_unique_impl<W<Current..., T>, Ts...>::type> {};
template <typename W>
struct make_unique;
template <template <typename...> class W, typename... T>
struct make_unique<W<T...>> : _make_unique_impl<W<>, T...> {};
}  // namespace detail
}  // namespace awacorn
#if __cplusplus >= 201703L
#include <cstddef>
#include <variant>
#else
#include <cstddef>
#include <exception>
#include <memory>
#endif
namespace awacorn {
#if __cplusplus >= 201703L
using variant = std::variant;
/**
 * @brief 去重 variant。
 *
 * @tparam Args 类型参数。
 */
template <typename... Args>
using unique_variant =
    typename detail::make_unique<std::variant<Args...>>::type;
using bad_variant_caccess = std::bad_variant_access;
using monostate = std::monostate;
template <size_t I, typename... Args>
constexpr auto get_if(std::variant<Args...>& v) noexcept
    -> decltype(std::get_if<I>(v)) {
  return std::get_if<I>(v);
}
template <size_t I, typename... Args>
constexpr auto get_if(const std::variant<Args...>& v) noexcept
    -> decltype(std::get_if<I>(v)) {
  return std::get_if<I>(v);
}
template <typename T, typename... Args>
constexpr auto get_if(std::variant<Args...>& v) noexcept
    -> decltype(std::get_if<T>(v)) {
  return std::get_if<T>(v);
}
template <typename T, typename... Args>
constexpr auto get_if(const std::variant<Args...>& v) noexcept
    -> decltype(std::get_if<T>(v)) {
  return std::get_if<T>(v);
}
template <size_t I, typename... Args>
constexpr auto get(std::variant<Args...>& v) -> decltype(std::get_if<I>(v)) {
  return std::get<I>(v);
}
template <size_t I, typename... Args>
constexpr auto get(const std::variant<Args...>& v)
    -> decltype(std::get_if<I>(v)) {
  return std::get<I>(v);
}
template <typename T, typename... Args>
constexpr auto get(std::variant<Args...>& v) -> decltype(std::get_if<T>(v)) {
  return std::get<T>(v);
}
template <typename T, typename... Args>
constexpr auto get(const std::variant<Args...>& v)
    -> decltype(std::get_if<T>(v)) {
  return std::get<T>(v);
}
template <typename T, typename... Args>
constexpr bool holds_alternative(const std::variant<Args...>& v) {
  return std::holds_alternative<T>(v);
}
#else
namespace detail {
template <size_t cur, typename TargetT, typename... Args>
struct _idx_helper : std::integral_constant<size_t, (size_t)-1> {};
template <size_t cur, typename TargetT, typename ResultT, typename... Args>
struct _idx_helper<cur, TargetT, ResultT, Args...>
    : _idx_helper<cur + 1, TargetT, Args...> {};
template <size_t cur, typename TargetT, typename... Args>
struct _idx_helper<cur, TargetT, TargetT, Args...>
    : std::integral_constant<size_t, cur> {};
template <typename TargetT, typename... Args>
struct type_index : _idx_helper<0, TargetT, Args...> {};
template <size_t idx, typename... Args>
struct index_type;
template <size_t idx, typename T, typename... Args>
struct index_type<idx, T, Args...> : index_type<idx - 1, Args...> {};
template <typename T, typename... Args>
struct index_type<0, T, Args...> {
  using type = T;
};

};  // namespace detail
struct bad_variant_access : std::exception {
  virtual const char* what() const noexcept { return "bad variant access"; }
};
struct monostate {};
template <typename... Args>
struct variant {
  static constexpr size_t npos = (size_t)-1;
  constexpr size_t index() const noexcept { return _idx; }
  constexpr bool valueless_by_exception() const noexcept {
    return _idx == npos;
  }
  template <typename T, typename... Arg>
  T& emplace(Arg&&... args) {
    static_assert(detail::type_index<T, Args...>::value != (size_t)-1,
                  "ill-formed construct");
    _idx = detail::type_index<T, Args...>::value;
    _ptr = std::unique_ptr<void, void (*)(void*)>(
        new T(std::forward<Arg>(args)...), [](void* v) { delete (T*)v; });
    return *((T*)_ptr.get());
  }
  inline void swap(variant& v) noexcept {
    std::swap(v._idx, _idx);
    std::swap(v._ptr, _ptr);
  }
  variant()
      : _idx(npos),
        _ptr(nullptr, [](void*) {}),
        _clone([](void*) -> void* { return nullptr; }){};
  template <typename T,
            typename = typename std::enable_if<!std::is_same<
                typename std::decay<T>::type, variant<Args...>>::value>::type>
  explicit variant(T&& v)
      : _idx(detail::type_index<typename std::decay<T>::type, Args...>::value),
        _ptr(new typename std::decay<T>::type(std::forward<T>(v)),
             [](void* v) { delete (typename std::decay<T>::type*)v; }),
        _clone([](void* ptr) -> void* {
          return (void*)new typename std::decay<T>::type(
              *((typename std::decay<T>::type*)ptr));
        }) {
    static_assert(
        detail::type_index<typename std::decay<T>::type, Args...>::value !=
            (size_t)-1,
        "ill-formed construct");
  }
  variant(const variant& v)
      : _idx(v._idx),
        _ptr(v._clone(v._ptr.get()), v._ptr.get_deleter()),
        _clone(v._clone) {}
  variant(variant&& v)
      : _idx(v._idx), _ptr(std::move(v._ptr)), _clone(v._clone) {
    v._idx = npos;
    v._clone = [](void*) -> void* { return nullptr; };
  }
  variant& operator=(const variant& v) {
    _idx = v._idx;
    _ptr = std::unique_ptr<void, void (*)(void*)>(v._clone(v._ptr.get()),
                                                  v._ptr.get_deleter());
    _clone = v._clone;
    return *this;
  }
  variant& operator=(variant&& v) {
    _idx = v._idx;
    v._idx = npos;
    _ptr = std::move(v._ptr);
    _clone = v._clone;
    v._clone = [](void*) -> void* { return nullptr; };
    return *this;
  }
  template <size_t idx, typename... Arg>
  friend typename detail::index_type<idx, Arg...>::type* get_if(
      variant<Arg...>&) noexcept;
  template <size_t idx, typename... Arg>
  friend const typename detail::index_type<idx, Arg...>::type* get_if(
      const variant<Arg...>&) noexcept;
  template <typename T, typename... Arg>
  friend T* get_if(variant<Arg...>&) noexcept;
  template <typename T, typename... Arg>
  friend const T* get_if(const variant<Arg...>&) noexcept;
  template <typename T, typename... Arg>
  friend bool holds_alternative(const variant<Arg...>&) noexcept;

 private:
  size_t _idx;
  std::unique_ptr<void, void (*)(void*)> _ptr;
  void* (*_clone)(void*);
};
/**
 * @brief 去重 variant。
 *
 * @tparam Args 类型参数。
 */
template <typename... Args>
using unique_variant = typename detail::make_unique<variant<Args...>>::type;
template <size_t idx, typename... Args>
typename detail::index_type<idx, Args...>::type* get_if(
    variant<Args...>& v) noexcept {
  if (v.index() == idx) {
    return (typename detail::index_type<idx, Args...>::type*)v._ptr.get();
  }
  return nullptr;
}
template <size_t idx, typename... Args>
const typename detail::index_type<idx, Args...>::type* get_if(
    const variant<Args...>& v) noexcept {
  if (v.index() == idx) {
    return (const typename detail::index_type<idx, Args...>::type*)v._ptr.get();
  }
  return nullptr;
}
template <typename T, typename... Args>
T* get_if(variant<Args...>& v) noexcept {
  static_assert(detail::type_index<T, Args...>::value != (size_t)-1,
                "ill-formed cast");
  if (v.index() == detail::type_index<T, Args...>::value) {
    return (T*)v._ptr.get();
  }
  return nullptr;
}
template <typename T, typename... Args>
const T* get_if(const variant<Args...>& v) noexcept {
  static_assert(detail::type_index<T, Args...>::value != (size_t)-1,
                "ill-formed cast");
  if (v.index() == detail::type_index<T, Args...>::value) {
    return (const T*)v._ptr.get();
  }
  return nullptr;
}
template <size_t idx, typename... Args>
inline auto get(variant<Args...>& v) -> decltype(*get_if<idx>(v)) {
  auto ptr = get_if<idx>(v);
  if (ptr) return *ptr;
  throw bad_variant_access();
}
template <size_t idx, typename... Args>
inline auto get(const variant<Args...>& v) -> decltype(*get_if<idx>(v)) {
  auto ptr = get_if<idx>(v);
  if (ptr) return *ptr;
  throw bad_variant_access();
}
template <typename T, typename... Args>
inline auto get(variant<Args...>& v) -> decltype(*get_if<T>(v)) {
  auto ptr = get_if<T>(v);
  if (ptr) return *ptr;
  throw bad_variant_access();
}
template <typename T, typename... Args>
inline auto get(const variant<Args...>& v) -> decltype(*get_if<T>(v)) {
  auto ptr = get_if<T>(v);
  if (ptr) return *ptr;
  throw bad_variant_access();
}
template <typename T, typename... Args>
inline bool holds_alternative(const variant<Args...>& v) noexcept {
  static_assert(detail::type_index<T, Args...>::value != (size_t)-1,
                "ill-formed cast");
  return v._idx == detail::type_index<T, Args...>::value;
}
#endif
};  // namespace awacorn
#endif
#endif