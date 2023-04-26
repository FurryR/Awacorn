#include <cstddef>
#include <list>
#include <tuple>
#include <typeindex>

namespace awacorn {
namespace detail {
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
/**
 * @brief 内部实现类，用于在编译时将类型转换为下标。
 *
 * @tparam idx 遍历到的下标。
 * @tparam TargetT 目标查找的类型。
 * @tparam Args 多个类型。
 */
template <size_t idx, typename TargetT, typename... Args>
struct __type_index_helper : public std::integral_constant<size_t, (size_t)-1> {
};
template <size_t idx, typename TargetT, typename... Args>
struct __type_index_helper<idx, TargetT, TargetT, Args...>
    : public std::integral_constant<size_t, idx> {};
template <size_t idx, typename TargetT, typename CompareT, typename... Args>
struct __type_index_helper<idx, TargetT, CompareT, Args...>
    : public std::integral_constant<
          size_t, __type_index_helper<idx + 1, TargetT, Args...>::value> {};
template <size_t idx, typename TargetT>
struct __type_index_helper<idx, TargetT>
    : public std::integral_constant<size_t, (size_t)-1> {};
/**
 * @brief 在编译时从 Args 中查找 TargetT
 * 并返回其第一次出现的位置。若找不到则返回 npos (-1)。
 *
 * @tparam TargetT 目标类型。
 * @tparam Args 多个类型。
 */
template <typename TargetT, typename... Args>
struct _type_index
    : public std::integral_constant<
          size_t, __type_index_helper<0, TargetT, Args...>::value> {};
template <size_t current, size_t idx, typename... Args>
struct __index_type_helper;
template <size_t current, size_t idx, typename T, typename... Args>
struct __index_type_helper<current, idx, T, Args...>
    : public std::enable_if<
          current <= idx,
          std::conditional<idx == current, T,
                           __index_type_helper<current, idx, T, Args>>::type>::
          type {};
template <size_t idx, typename... Args>
struct _index_type : public __index_type_helper<0, sizeof...(idx), Args...> {};
};  // namespace detail
template <typename... T>
struct variant {
  static constexpr size_t variant_npos = -1;

  constexpr size_t index() { return _idx; }

 private:
  size_t _idx;
  alignas(detail::_max_alignof<T...>::value) char _data[sizeof(
      detail::_max_sizeof<T...>::value)];
};
};  // namespace awacorn
