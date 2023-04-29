# async

:beginner: 最简单的部分，实现 `async/await` 的头文件。

## 目录

- [async](#async)
  - [目录](#目录)
  - [`awacorn::async`](#awacornasync)
  - [`awacorn::context`](#awacorncontext)
    - [`operator>>`](#operator)

---

## `awacorn::async`

:gem: 进入异步函数上下文。

```cpp
#include "awacorn/async.hpp"
promise<int> async_operation() {
  return awacorn::async([](awacorn::context&) {
    return 1;
  });
}
int main() {
  auto pm = awacorn::async([](awacorn::context& ctx) {
    return (ctx >> async_operation()) + 1;
  });
  pm.then([](int i) {
    std::cout << i << std::endl;
  });
}
```

- `async` 内的匿名函数接受一个 `awacorn::context&` 作为上下文参数。
  - `async` 会自动将函数的返回值类型作为 `promise` 的结果类型。
- `async` 还支持于第二个参数指定**栈大小**(如果可用)。

## `awacorn::context`

异步上下文。v3.1.0 以后的 `async` 相比旧版本的 `async_generator`(`AsyncGenerator`) 简便、清爽很多。

:bulb: awacorn::context 不可被移动、拷贝、或者构造，只能通过 `async` 函数取得。

### `operator>>`

此上下文对象重载了 `operator>>` 以实现和 `await` 相近的语法。比如，以下的方式将**等待**一个 `promise` 完成并且取得返回值。

```cpp
#include "awacorn/async.hpp"
#define await ctx >>
int main() {
  auto pm = awacorn::async([](awacorn::context& ctx) {
    return (await awacorn::resolve(1)) + 1;
  });
  pm.then([](int i) {
    std::cout << i << std::endl;
  });
}
```

- `ctx >>` 的后面是一个 `promise` 对象。