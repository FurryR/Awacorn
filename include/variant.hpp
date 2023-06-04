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
#include <variant>
#else
#include <exception>
#include <memory>
#endif
namespace awacorn {
#if __cplusplus >= 201703L
template <typename... Args>
using variant = std::variant<Args...>;
inline constexpr std::size_t variant_npos = std::variant_npos;
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
template <std::size_t I, typename... Args>
constexpr auto get_if(std::variant<Args...>& v) noexcept
    -> decltype(std::get_if<I>(v)) {
  return std::get_if<I>(v);
}
template <std::size_t I, typename... Args>
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
template <std::size_t I, typename... Args>
constexpr auto get(std::variant<Args...>& v) -> decltype(std::get_if<I>(v)) {
  return std::get<I>(v);
}
template <std::size_t I, typename... Args>
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
template <std::size_t cur, typename TargetT, typename... Args>
struct _idx_helper : std::integral_constant<std::size_t, (std::size_t)-1> {};
template <std::size_t cur, typename TargetT, typename ResultT, typename... Args>
struct _idx_helper<cur, TargetT, ResultT, Args...>
    : _idx_helper<cur + 1, TargetT, Args...> {};
template <std::size_t cur, typename TargetT, typename... Args>
struct _idx_helper<cur, TargetT, TargetT, Args...>
    : std::integral_constant<std::size_t, cur> {};
template <typename TargetT, typename... Args>
struct type_index : _idx_helper<0, TargetT, Args...> {};
template <std::size_t idx, typename... Args>
struct index_type;
template <std::size_t idx, typename T, typename... Args>
struct index_type<idx, T, Args...> : index_type<idx - 1, Args...> {};
template <typename T, typename... Args>
struct index_type<0, T, Args...> {
  using type = T;
};
/**
 * @brief 编译时计算给定类型中大小最大的一个。
 *
 * @tparam Args 多个类型。
 */
template <typename... Args>
struct _max_sizeof : public std::integral_constant<size_t, 0> {};
template <typename T>
struct _max_sizeof<T> : public std::integral_constant<size_t, sizeof(T)> {};
template <typename T, typename T2, typename... Args>
struct _max_sizeof<T, T2, Args...>
    : public std::integral_constant<
          size_t, (sizeof(T) > sizeof(T2) ? _max_sizeof<T, Args...>::value
                                          : _max_sizeof<T2, Args...>::value)> {
};
/**
 * @brief 编译时计算给定类型中对齐大小最大的一个。
 *
 * @tparam Args 多个类型。
 */
template <typename... Args>
struct _max_alignof : public std::integral_constant<size_t, 0> {};
template <typename T>
struct _max_alignof<T> : public std::integral_constant<size_t, alignof(T)> {};
template <typename T, typename T2, typename... Args>
struct _max_alignof<T, T2, Args...>
    : public std::integral_constant<size_t,
                                    (alignof(T) > alignof(T2)
                                         ? _max_alignof<T, Args...>::value
                                         : _max_alignof<T2, Args...>::value)> {
};
};  // namespace detail
struct bad_variant_access : std::exception {
  virtual const char* what() const noexcept { return "bad variant access"; }
};
struct monostate {};
static constexpr std::size_t variant_npos = (std::size_t)-1;
template <typename... Args>
class variant {
  enum op { clone = 0, move = 1, destroy = 2 };
  template <typename T>
  struct manager {
    static void apply(op operation, const void* ptr, void* ptr2) {
      switch (operation) {
        case clone: {
          new (ptr2) T(*((T*)ptr));
          break;
        }
        case move: {
          new (ptr2) T(std::move(*((T*)ptr)));
          break;
        }
        case destroy: {
          ((T*)ptr2)->~T();
          break;
        }
      }
    }
  };
  alignas(detail::_max_alignof<
          Args...>::value) char _ptr[detail::_max_sizeof<Args...>::value];
  void (*_manager)(op, const void*, void*);
  std::size_t _idx;

 public:
  template <std::size_t idx, typename... Arg>
  friend typename detail::index_type<idx, Arg...>::type* get_if(
      variant<Arg...>&) noexcept;
  template <std::size_t idx, typename... Arg>
  friend const typename detail::index_type<idx, Arg...>::type* get_if(
      const variant<Arg...>&) noexcept;
  template <typename T, typename... Arg>
  friend T* get_if(variant<Arg...>&) noexcept;
  template <typename T, typename... Arg>
  friend const T* get_if(const variant<Arg...>&) noexcept;
  template <typename T, typename... Arg>
  friend constexpr bool holds_alternative(const variant<Arg...>&) noexcept;
  constexpr bool valueless_by_exception() const noexcept {
    return _idx == variant_npos;
  }
  constexpr std::size_t index() const noexcept { return _idx; }
  template <typename T, typename... Arg>
  T& emplace(Arg&&... args) {
    static_assert(detail::type_index<T, Args...>::value != (std::size_t)-1,
                  "ill-formed construct");
    _idx = detail::type_index<T, Args...>::value;
    _ptr = std::unique_ptr<void, void (*)(void*)>(
        new T(std::forward<Arg>(args)...), [](void* v) { delete (T*)v; });
    return *((T*)_ptr.get());
  }
  inline void swap(variant& v) noexcept {
    std::swap(v._idx, _idx);
    std::swap(v._manager, _manager);
    std::swap(v._ptr, _ptr);
  }
  constexpr variant() : _manager(nullptr), _idx(variant_npos){};
  template <typename T,
            typename = typename std::enable_if<!std::is_same<
                typename std::decay<T>::type, variant<Args...>>::value>::type>
  variant(T&& v)
      : _manager(manager<typename std::decay<T>::type>::apply),
        _idx(detail::type_index<typename std::decay<T>::type, Args...>::value) {
    static_assert(
        detail::type_index<typename std::decay<T>::type, Args...>::value !=
            (std::size_t)-1,
        "ill-formed construct");
    new (_ptr) typename std::decay<T>::type(std::forward<T>(v));
  }
  variant(const variant& v) : _idx(v._idx), _manager(v._manager) {
    if (_idx != variant_npos) {
      _manager(clone, v._ptr, _ptr);
    }
  }
  variant(variant&& v) : _idx(v._idx), _manager(v._manager) {
    if (_idx == variant_npos) {
      _manager(move, v._ptr, _ptr);
    }
  }
  variant& operator=(const variant& v) {
    if (_idx != variant_npos) {
      _manager(destroy, nullptr, _ptr);
    }
    _idx = v._idx;
    _manager = v._manager;
    _manager(clone, v._ptr, _ptr);
    return *this;
  }
  variant& operator=(variant&& v) {
    if (_idx != variant_npos) {
      _manager(destroy, nullptr, _ptr);
    }
    _idx = v._idx;
    _manager = v._manager;
    _manager(move, v._ptr, _ptr);
    return *this;
  }
  ~variant() {
    if (_idx != variant_npos) {
      _manager(destroy, nullptr, _ptr);
    }
  }
};
/**
 * @brief 去重 variant。
 *
 * @tparam Args 类型参数。
 */
template <typename... Args>
using unique_variant = typename detail::make_unique<variant<Args...>>::type;
template <std::size_t idx, typename... Args>
typename detail::index_type<idx, Args...>::type* get_if(
    variant<Args...>& v) noexcept {
  if (v.index() == idx) {
    return (typename detail::index_type<idx, Args...>::type*)v._ptr;
  }
  return nullptr;
}
template <std::size_t idx, typename... Args>
const typename detail::index_type<idx, Args...>::type* get_if(
    const variant<Args...>& v) noexcept {
  if (v.index() == idx) {
    return (const typename detail::index_type<idx, Args...>::type*)v._ptr;
  }
  return nullptr;
}
template <typename T, typename... Args>
T* get_if(variant<Args...>& v) noexcept {
  static_assert(detail::type_index<T, Args...>::value != (std::size_t)-1,
                "ill-formed cast");
  if (v.index() == detail::type_index<T, Args...>::value) {
    return (T*)v._ptr;
  }
  return nullptr;
}
template <typename T, typename... Args>
const T* get_if(const variant<Args...>& v) noexcept {
  static_assert(detail::type_index<T, Args...>::value != (std::size_t)-1,
                "ill-formed cast");
  if (v.index() == detail::type_index<T, Args...>::value) {
    return (const T*)v._ptr;
  }
  return nullptr;
}
template <std::size_t idx, typename... Args>
inline auto get(variant<Args...>& v) -> decltype(*get_if<idx>(v)) {
  auto ptr = get_if<idx>(v);
  if (ptr) return *ptr;
  throw bad_variant_access();
}
template <std::size_t idx, typename... Args>
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
constexpr bool holds_alternative(const variant<Args...>& v) noexcept {
  static_assert(detail::type_index<T, Args...>::value != (std::size_t)-1,
                "ill-formed cast");
  return v._idx == detail::type_index<T, Args...>::value;
}
#endif
};  // namespace awacorn
#endif
#endif