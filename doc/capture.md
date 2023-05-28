# capture

:briefcase: `capture` 是 Awacorn 用于实现在 C++ 11 进行移动构造/完美转发捕获的类型。

## 目录

- [capture](#capture)
  - [目录](#目录)
  - [`awacorn::detail::capture_helper`](#awacorndetailcapture_helper)
    - [`borrow`](#borrow)
  - [`awacorn::detail::capture`](#awacorndetailcapture)
  - [注意事项](#注意事项)

---

## `awacorn::detail::capture_helper`

:briefcase: 用于储存捕获。

```cpp
#include <string>
#include <iostream>
#include "awacorn/detail/capture.hpp"
int main() {
  std::string a = "str";
  awacorn::detail::capture_helper arg_a = awacorn::detail::capture(std::move(a));
  ([arg_a]() mutable {
    std::string b = std::move(arg_a.borrow()); // 移动
  })();
}
```

:memo: 取自 [`borrow`](#borrow) 的示例。

### `borrow`

:open_file_folder: 借用工具类内的对象 (不可变左值引用/左值引用)。

```cpp
#include <string>
#include <iostream>
#include "awacorn/detail/capture.hpp"
int main() {
  std::string a = "str";
  awacorn::detail::capture_helper arg_a = awacorn::detail::capture(std::move(a));
  ([arg_a]() {
    std::string b = arg_a.borrow(); // const std::string& 拷贝
  })();
  ([arg_a]() mutable {
    std::string b = std::move(arg_a.borrow()); // std::string& 移动
  })();
}
```

- :no_entry_sign: 在 `capture_helper` 为不可变的情况下，`borrow` 只会返回不可变左值引用，不能用于移动构造。
- :white_check_mark: 在 `capture_helper` 为可变的情况下，`borrow` 会返回左值引用，可用于移动构造。
  - :beginner: 若需要将捕获值变为可变，请在函数的 `->`(_显式指定返回值_) 或者 `{`(_自动推导返回值_) 前加入 `mutable`。

## `awacorn::detail::capture`

:open_file_folder: 完美转发一个对象到工具类。

```cpp
#include <string>
#include <iostream>
#include "awacorn/detail/capture.hpp"
int main() {
  std::string a = "str";
  awacorn::detail::capture_helper arg_b = awacorn::detail::capture(a); // 复制
  awacorn::detail::capture_helper arg_b = awacorn::detail::capture(std::move(a)); // 移动
}
```

- :beginner: 若想要移动一个对象需要显式指定 `std::move`，否则会拷贝对象。

## 注意事项

:warning: `awacorn::detail` 命名空间内的成员均是 Awacorn 的 **内部类**，擅自使用会导致 **可移植性降低**，并且我们也不推荐这么做。
