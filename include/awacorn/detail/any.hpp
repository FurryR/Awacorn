#ifndef _AWACORN_ANY_
#define _AWACORN_ANY_
#if __cplusplus >= 201703L
#include <any>
#else
#include <memory>
#endif
namespace awacorn {
#if __cplusplus >= 201703L
/**
 * @brief awacorn::any::cast 抛出的错误。
 */
using bad_any_cast = std::bad_any_cast;
/**
 * @brief C++ 17 以后对 std::any 的包装。
 * @see https://en.cppreference.com/w/cpp/utility/any
 */
class any {
  std::any data;

 public:
  any() = default;
  /**
   * @brief 由指定对象初始化 any 容器。
   *
   * @tparam T 对象类型。
   * @param val 对象实例。
   */
  template <typename T>
  any(T&& val) : data(std::forward<T>(val)) {}
  any(const any& v) : data(v.data) {}
  any(any&& v) : data(std::move(v.data)) {}
  any& operator=(const any& v) {
    data = v.data;
    return *this;
  }
  any& operator=(any&& v) {
    data = std::move(v.data);
    return *this;
  }
  /**
   * @brief 获得对象的运行时类型信息 (RTTI)。
   * @warning 如果对象未被初始化，返回 typeid(void)。
   *
   * @return const std::type_info& 运行时类型信息。
   */
  inline const std::type_info& type() const { return data.type(); }
  /**
   * @brief 判断此 any 容器是否有值。
   *
   * @return true 有值
   * @return false 无值
   */
  inline bool has_value() const noexcept { return data.has_value(); }
  /**
   * @brief 清空 any 容器。
   */
  inline void reset() noexcept { data.reset(); }
  /**
   * @brief 将此 any 容器转换为 std::any。
   *
   * @return std::any
   * @since C++17
   */
  inline std::any as_any() const { return data; }
  /**
   * @brief 由指定参数列表初始化对象。
   *
   * @tparam T 对象类型。
   * @tparam Args 参数列表类型。
   * @param args 参数列表。
   * @return T& 对象的引用。
   */
  template <typename T, typename... Args>
  inline T& emplace(Args&&... args) {
    return data.emplace<T>(std::forward<Args>(args)...);
  }
  /**
   * @brief 将对象转换为指定类型。
   * @throw std::bad_any_cast (aka awacorn::bad_any_cast)
   *
   * @tparam T 类型。
   * @return T 这个类型的实例（如果允许转换）。
   */
  template <typename T>
  inline T cast() const {
    return std::any_cast<T>(data);
  }
  /**
   * @brief 和另一个对象交换。
   *
   * @param other 要交换的对象。
   */
  inline void swap(any& other) noexcept { std::swap(other.data, data); }
};
#else
/**
 * @brief awacorn::any::cast 抛出的错误。
 */
struct bad_any_cast : public std::bad_cast {
  virtual const char* what() const noexcept { return "bad any_cast"; }
};
/**
 * @brief 类型安全的 C++ 11 any 实现。
 * @see https://en.cppreference.com/w/cpp/utility/any
 */
class any {
  struct _m_base {
    virtual ~_m_base() = default;
    virtual const std::type_info& type() const = 0;
    virtual std::unique_ptr<_m_base> clone() const = 0;
  };
  template <typename T>
  struct _m_derived : _m_base {
    T data;
    std::unique_ptr<_m_base> clone() const override {
      return std::unique_ptr<_m_base>(new _m_derived<T>(data));
    }
    const std::type_info& type() const override { return typeid(T); }
    _m_derived(const T& data) : data(data) {}
  };
  std::unique_ptr<_m_base> ptr;

 public:
  any() = default;
  /**
   * @brief 由指定对象初始化 any 容器。
   *
   * @tparam T 对象类型。
   * @param val 对象实例。
   */
  template <typename T>
  any(T&& val)
      : ptr(std::unique_ptr<_m_base>(
            new _m_derived<typename std::decay<T>::type>(
                std::forward<T>(val)))) {}
  any(const any& val) : ptr(val.ptr ? val.ptr->clone() : nullptr) {}
  any(any&& val) : ptr(std::move(val.ptr)) {}
  any& operator=(const any& rhs) {
    if (rhs.ptr)
      ptr = rhs.ptr->clone();
    else
      ptr = nullptr;
    return *this;
  }
  any& operator=(any&& rhs) { return (ptr = std::move(rhs.ptr)), *this; }
  /**
   * @brief 获得对象的运行时类型信息 (RTTI)。
   * @warning 如果对象未被初始化，返回 typeid(void)。
   *
   * @return const std::type_info& 运行时类型信息。
   */
  inline const std::type_info& type() const {
    return ptr ? ptr->type() : typeid(void);
  }
  /**
   * @brief 判断此 any 容器是否有值。
   *
   * @return true 有值
   * @return false 无值
   */
  inline bool has_value() const noexcept { return !!ptr; }
  /**
   * @brief 清空 any 容器。
   */
  inline void reset() noexcept { ptr.reset(); }
  /**
   * @brief 由指定参数列表初始化对象。
   *
   * @tparam T 对象类型。
   * @tparam Args 参数列表类型。
   * @param args 参数列表。
   * @return T& 对象的引用。
   */
  template <typename T, typename... Args>
  T& emplace(Args&&... args) {
    using Decay = typename std::decay<T>::type;
    reset();
    _m_derived<Decay> t = new _m_derived<Decay>(std::forward<Args>(args)...);
    ptr = std::unique_ptr<_m_base>(t);
    return t->data;
  }
  /**
   * @brief 将对象转换为指定类型。
   * @throw std::bad_any_cast (aka awacorn::bad_any_cast)
   *
   * @tparam T 类型。
   * @return T 这个类型的实例（如果允许转换）。
   */
  template <typename T>
  T cast() const {
    using Decay = typename std::decay<T>::type;
    any::_m_derived<Decay>* _ptr =
        dynamic_cast<any::_m_derived<Decay>*>(ptr.get());
    if (_ptr) return _ptr->data;
    throw bad_any_cast();
  }
  /**
   * @brief 和另一个对象交换。
   *
   * @param other 要交换的对象。
   */
  inline void swap(any& other) noexcept { std::swap(other.ptr, ptr); }
};
#endif
};  // namespace awacorn
#endif