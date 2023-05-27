#ifndef _AWACORN_CAPTURE_HELPER_
#define _AWACORN_CAPTURE_HELPER_
#if __cplusplus >= 201101L
/**
 * Project awacorn 基于 MIT 协议开源。
 * Copyright(c) 凌 2022.
 */
namespace awacorn {
namespace detail {
/**
 * @brief 负责实现存储完美转发捕获值的工具类。
 *
 * @tparam T 值的类型。
 */
template <typename T>
struct capture_helper {
  capture_helper() = delete;
  /**
   * @brief 由右值构造对象 (移动构造)。
   */
  constexpr capture_helper(T&& val) : val(std::move(val)) {}
  /**
   * @brief 由不可变左值引用构造对象 (拷贝构造)。
   */
  constexpr capture_helper(const T& val) : val(val) {}
  capture_helper(const capture_helper& rhs) = delete;
  /**
   * @brief 使用左值引用进行移动构造。
   */
  constexpr capture_helper(capture_helper& rhs) : val(std::move(rhs.val)) {}
  /**
   * @brief 使用右值引用进行移动构造。
   */
  constexpr capture_helper(capture_helper&& rhs) : val(std::move(rhs.val)) {}
  capture_helper& operator=(const capture_helper& rhs) = delete;
  /**
   * @brief 借用对象的不可变左值引用。
   *
   * @return const T& 对象的不可变左值引用。
   */
  inline const T& borrow() const noexcept { return val; }
  /**
   * @brief 借用对象的左值引用 (可用于移动构造)。
   *
   * @return T& 对象的左值引用。
   */
  inline T& borrow() noexcept { return val; }

 private:
  T val;
};
/**
 * @brief 完美转发值至工具类。
 *
 * @tparam T 值的类型。
 * @param val 值本身，可以是各种引用。
 * @return capture_helper<T> 工具类。
 */
template <typename T>
constexpr capture_helper<T> capture(T&& val) noexcept {
  return capture_helper<T>(std::forward<T>(val));
}
};  // namespace detail
};  // namespace awacorn
#endif
#endif