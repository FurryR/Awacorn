# experimental/async

💣 Awacorn 中最不稳定的一个组件，负责实现 C++ 11 下的无栈协程，思路来自 DSL。

⚠️ 此组件的目的是为了替代现有有栈协程 `async` 的。有栈协程在创建栈空间时速度较慢，换用无栈协程虽然不能达到 C++ 20 的速度，但可以尽可能省下申请栈带来的成本。

✅ 此组件仅依赖 STL。意味着它不需要 `boost::context` 或者 `ucontext` 支持。即使在单片机上也可以使用！

此头文件中的一切内容都可能因为最新一次 commit 而发生改变或完全被删除。在此组件完全稳定下来之前，请不要使用。

## 目录

- [async-next](#async-next)
  - [目录](#目录)
  - [`awacorn::async`](#awacornasync)
  - [`awacorn::context`](#awacorncontext)
    - [`operator<<`](#operator)

---

## `awacorn::async`

💎 进入异步函数上下文。

```cpp
#include "awacorn/async.hpp"
promise<int> async_operation() {
  return awacorn::async<int>([](awacorn::context<int>& ctx) {
    ctx << ctx.ret(0);
  }); // dev: 需要有一个更好的调用方式，而不是 ctx.call
}
int main() {
  auto pm = awacorn::async<int>([](awacorn::context<int>& ctx) {
    ctx << ctx.ret(ctx.call(async_operation) + 1);
  });
  pm.then([](int i) {
    std::cout << i << std::endl;
  });
}
```

- `async` 内的匿名函数接受一个 `awacorn::context<T>&` 作为上下文参数。
  - `async` 会自动将函数的返回值类型作为 `promise` 的结果类型。
- `async` 需要指定一个模板参数作为异步上下文的返回类型。这同时也会影响到参数的模板类型。
  - 例如 `async` 的模板参数为 `T`，则函数的第一个参数类型应为 `awacorn::context<T>&`。
  - 目前暂时不支持根据函数签名自动推导返回类型，非常抱歉。以 C++ 的能力是无法做到全自动推导的。

## `awacorn::context`

异步上下文。该类需要一个模板参数作为异步上下文**返回值的类型**。

💡 `awacorn::context` 不可被移动、拷贝、或者构造，只能通过 `async` 函数取得。

### `operator<<`

此上下文对象重载了 `operator<<` 以实现向上下文中追加 DSL 协程语句。可用的协程语句一般位于 `ctx` 中。比如，以下的方式将**等待**一个  `promise` 完成并且取得返回值。

```cpp
#include "awacorn/async.hpp"
int main() {
  auto pm = awacorn::async([](awacorn::context& ctx) {
    ctx << ctx.ret(ctx.await(ctx.value(awacorn::resolve(1))) + 1);
  });
  pm.then([](int i) {
    std::cout << i << std::endl;
  });
}
```

- `ctx <<` 后面的对象只能为 `detail::expr`。
