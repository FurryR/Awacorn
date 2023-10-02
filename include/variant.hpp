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
/**
 * @brief 检查 T 是否包含在 Args 序列中。
 *
 * @tparam T 类型。
 * @tparam Args 类型序列。
 */
template <typename T, typename... Args>
struct contains : std::false_type {};
template <typename T, typename T2, typename... Args>
struct contains<T, T2, Args...> : contains<T, Args...> {};
template <typename T, typename... Args>
struct contains<T, T, Args...> : std::true_type {};
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
/**
 * @brief 去除可变模板类型参数容器中的重复参数。
 *
 * @tparam W 容器类型。
 */
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
/**
 * @brief 等同于 std::variant。
 *
 * @tparam Args 容器参数列表。
 */
template <typename... Args>
using variant = std::variant<Args...>;
/**
 * @brief 等同于 std::variant_npos。
 */
inline constexpr std::size_t variant_npos = std::variant_npos;
/**
 * @brief 去重 variant。
 *
 * @tparam Args 类型参数。
 */
template <typename... Args>
using unique_variant =
    typename detail::make_unique<std::variant<Args...>>::type;
/**
 * @brief 等同于 std::bad_variant_access。
 */
using bad_variant_access = std::bad_variant_access;
/**
 * @brief 用于替代 void 使得 variant 可以默认构造的类。
 */
using monostate = std::monostate;
/**
 * @brief 同 std::get_if，由 variant
 * 的下标获取指针。下标必须小于
 * sizeof...(Args)。如果类型正确，返回对象指针，否则返回 nullptr。
 *
 * @tparam I variant 的下标。
 * @tparam Args variant 的参数列表。
 * @param v variant 对象。
 * @return decltype(auto) std::get_if<I>(v) 的返回值。
 */
template <std::size_t I, typename... Args>
constexpr decltype(auto) get_if(std::variant<Args...>& v) noexcept {
  return std::get_if<I>(v);
}
template <std::size_t I, typename... Args>
constexpr decltype(auto) get_if(const std::variant<Args...>& v) noexcept {
  return std::get_if<I>(v);
}
/**
 * @brief 同 std::get_if，由 variant
 * 中的类型获取指针。如果类型正确，返回对象指针，否则返回 nullptr。
 *
 * @tparam T 对象类型。必须包含在 variant 的模板参数列表中。
 * @tparam Args variant 的参数列表。
 * @param v variant 对象。
 * @return constexpr decltype(auto) std::get_if<T>(v) 的返回值。
 */
template <typename T, typename... Args>
constexpr decltype(auto) get_if(std::variant<Args...>& v) noexcept {
  return std::get_if<T>(v);
}
template <typename T, typename... Args>
constexpr decltype(auto) get_if(const std::variant<Args...>& v) noexcept {
  return std::get_if<T>(v);
}
/**
 * @brief 同 std::get，由 variant
 * 的下标获取对象引用。下标必须小于
 * sizeof...(Args)。如果类型正确，返回对象引用，否则抛出 bad_variant_access
 * 错误。
 *
 * @tparam I variant 的下标。
 * @tparam Args variant 的参数列表。
 * @param v variant 对象。
 * @return decltype(auto) std::get<I>(v) 的返回值。
 */
template <std::size_t I, typename... Args>
constexpr decltype(auto) get(std::variant<Args...>&& v) {
  return std::get<I>(v);
}
template <std::size_t I, typename... Args>
constexpr decltype(auto) get(const std::variant<Args...>& v) {
  return std::get<I>(v);
}
/**
 * @brief 同 std::get，由 variant
 * 中的类型获取对象引用。如果类型正确，返回对象引用，否则抛出 bad_variant_access
 * 错误。
 *
 * @tparam T 对象类型。必须包含在 variant 的模板参数列表中。
 * @tparam Args variant 的参数列表。
 * @param v variant 对象。
 * @return constexpr decltype(auto) std::get<T>(v) 的返回值。
 */
template <typename T, typename... Args>
constexpr decltype(auto) get(std::variant<Args...>&& v) {
  return std::get<T>(v);
}
template <typename T, typename... Args>
constexpr decltype(auto) get(const std::variant<Args...>& v) {
  return std::get<T>(v);
}
/**
 * @brief 判断 variant 是否正持有指定类型 T。
 *
 * @tparam T 指定类型。T 必须在 Args 内。
 * @tparam Args variant 的参数列表。
 * @param v variant 对象。
 * @return true 此 variant 对象正持有该类型。
 * @return false 此 variant 对象未持有该类型。
 */
template <typename T, typename... Args>
constexpr bool holds_alternative(const std::variant<Args...>& v) noexcept {
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
/**
 * @brief 获取 TargetT 在 Args 类型序列中的位置下标（从 0
 * 开始）。如果找不到则返回 -1。
 *
 * @tparam TargetT 类型。
 * @tparam Args 类型序列。
 */
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
/**
 * @brief 当 variant 访问错误时抛出的错误。
 */
struct bad_variant_access : std::exception {
  virtual const char* what() const noexcept { return "bad variant access"; }
};
/**
 * @brief 用于替代 void 使得 variant 可以默认构造的类。
 */
struct monostate {};
/**
 * @brief 表示 variant 下标不可能达到的值，也即当 variant 空值时返回的下标。
 */
static constexpr std::size_t variant_npos = (std::size_t)-1;
/**
 * @brief variant 类用于表示此类型可能是 Args 中的任意一个。
 * @warning variant 不允许指定 void 作为参数，请改用 monostate。
 * @warning 不应指定重复模板参数，即使它可以容许重复。一旦指定重复的模板参数，此
 * variant 将变为缪构。
 *
 * @tparam Args 模板参数列表。
 */
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
  /**
   * @brief 判断此 variant 是否持有值。等同于 index() == variant_npos。
   *
   * @return true 此 variant 持有值。
   * @return false 此 variant 未持有值。
   */
  constexpr bool valueless_by_exception() const noexcept {
    return _idx == variant_npos;
  }
  /**
   * @brief 判断此 variant 持有的类型。
   *
   * @return constexpr std::size_t variant 下标。
   */
  constexpr std::size_t index() const noexcept { return _idx; }
  /**
   * @brief 以指定的对象替换当前 variant 持有的对象。
   *
   * @tparam T 对象类型。
   * @tparam Arg 用于构造对象的参数列表。
   * @param args 参数列表。
   * @return T& 新对象的引用。
   */
  template <typename T, typename... Arg>
  T& emplace(Arg&&... args) {
    static_assert(detail::type_index<T, Args...>::value != (std::size_t)-1,
                  "ill-formed construct");
    _idx = detail::type_index<T, Args...>::value;
    _ptr = std::unique_ptr<void, void (*)(void*)>(
        new T(std::forward<Arg>(args)...), [](void* v) { delete (T*)v; });
    return *((T*)_ptr.get());
  }
  /**
   * @brief 和指定 variant 对象交换。
   *
   * @param v variant 对象。
   */
  inline void swap(variant& v) noexcept {
    std::swap(*this, v);
  }
  constexpr variant() : _manager(nullptr), _idx(variant_npos){};
  /**
   * @brief 由对象创建 variant 引用。
   *
   * @tparam T 原对象的类型。
   * @param v 原对象。对象必须至少允许移动构造。
   */
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
  variant(const variant& v) : _manager(v._manager), _idx(v._idx) {
    if (_idx != variant_npos) {
      _manager(clone, v._ptr, _ptr);
    }
  }
  variant(variant&& v): _manager(v._manager), _idx(v._idx) {
    if (_idx != variant_npos) _manager(move, v._ptr, _ptr);
  }
  variant& operator=(const variant& v) {
    if (_idx != variant_npos) {
      _manager(destroy, nullptr, _ptr);
    }
    _manager = v._manager;
    _idx = v._idx;
    if (_idx != variant_npos) _manager(clone, v._ptr, _ptr);
    return *this;
  }
  variant& operator=(variant&& v) {
    if (_idx != variant_npos) {
      _manager(destroy, nullptr, _ptr);
    }
    _manager = v._manager;
    _idx = v._idx;
    if (_idx != variant_npos) _manager(move, v._ptr, _ptr);
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
/**
 * @brief 同 std::get_if，由 variant
 * 的下标获取指针。下标必须小于
 * sizeof...(Args)。如果类型正确，返回对象指针，否则返回 nullptr。
 *
 * @tparam I variant 的下标。
 * @tparam Args variant 的参数列表。
 * @param v variant 对象。
 * @return decltype(auto) std::get_if<I>(v) 的返回值。
 */
template <std::size_t idx, typename... Args>
typename detail::index_type<idx, Args...>::type* get_if(
    variant<Args...>& v) noexcept {
  static_assert(idx < sizeof...(Args), "ill-formed cast");
  if (v.index() == idx) {
    return (typename detail::index_type<idx, Args...>::type*)v._ptr;
  }
  return nullptr;
}
template <std::size_t idx, typename... Args>
const typename detail::index_type<idx, Args...>::type* get_if(
    const variant<Args...>& v) noexcept {
  static_assert(idx < sizeof...(Args), "ill-formed cast");
  if (v.index() == idx) {
    return (const typename detail::index_type<idx, Args...>::type*)v._ptr;
  }
  return nullptr;
}
/**
 * @brief 同 std::get_if，由 variant
 * 中的类型获取指针。如果类型正确，返回对象指针，否则返回 nullptr。
 *
 * @tparam T 对象类型。必须包含在 variant 的模板参数列表中。
 * @tparam Args variant 的参数列表。
 * @param v variant 对象。
 * @return constexpr decltype(auto) std::get_if<T>(v) 的返回值。
 */
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
/**
 * @brief 同 std::get，由 variant
 * 的下标获取对象引用。下标必须小于
 * sizeof...(Args)。如果类型正确，返回对象引用，否则抛出 bad_variant_access
 * 错误。
 *
 * @tparam I variant 的下标。
 * @tparam Args variant 的参数列表。
 * @param v variant 对象。
 * @return decltype(auto) std::get<I>(v) 的返回值。
 */
template <std::size_t idx, typename... Args>
inline auto get(variant<Args...>& v) -> decltype(*get_if<idx>(v)) {
  static_assert(idx < sizeof...(Args), "ill-formed cast");
  auto ptr = get_if<idx>(v);
  if (ptr) return *ptr;
  throw bad_variant_access();
}
template <std::size_t idx, typename... Args>
inline auto get(const variant<Args...>& v) -> decltype(*get_if<idx>(v)) {
  static_assert(idx < sizeof...(Args), "ill-formed cast");
  auto ptr = get_if<idx>(v);
  if (ptr) return *ptr;
  throw bad_variant_access();
}
/**
 * @brief 同 std::get，由 variant
 * 中的类型获取对象引用。如果类型正确，返回对象引用，否则抛出 bad_variant_access
 * 错误。
 *
 * @tparam T 对象类型。必须包含在 variant 的模板参数列表中。
 * @tparam Args variant 的参数列表。
 * @param v variant 对象。
 * @return constexpr decltype(auto) std::get<T>(v) 的返回值。
 */
template <typename T, typename... Args>
inline auto get(variant<Args...>& v) -> decltype(*get_if<T>(v)) {
  static_assert(detail::type_index<T, Args...>::value != (std::size_t)-1,
                "ill-formed cast");
  auto ptr = get_if<T>(v);
  if (ptr) return *ptr;
  throw bad_variant_access();
}
template <typename T, typename... Args>
inline auto get(const variant<Args...>& v) -> decltype(*get_if<T>(v)) {
  static_assert(detail::type_index<T, Args...>::value != (std::size_t)-1,
                "ill-formed cast");
  auto ptr = get_if<T>(v);
  if (ptr) return *ptr;
  throw bad_variant_access();
}
/**
 * @brief 判断 variant 是否正持有指定类型 T。
 *
 * @tparam T 指定类型。T 必须在 Args 内。
 * @tparam Args variant 的参数列表。
 * @param v variant 对象。
 * @return true 此 variant 对象正持有该类型。
 * @return false 此 variant 对象未持有该类型。
 */
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