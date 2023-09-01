# promise

💫 Awacorn 异步编程的第一步，堂堂登场！

## 目录

- [promise](#promise)
  - [目录](#目录)
  - [概念](#概念)
  - [`awacorn::promise`](#awacornpromise)
    - [`then` / `error` / `finally`](#then--error--finally)
  - [`awacorn::resolve` / `awacorn::reject`](#awacornresolve--awacornreject)
  - [`awacorn::gather`](#awacorngather)
    - [`all`](#all)
    - [`any`](#any)
    - [`race`](#race)
    - [`all_settled`](#all_settled)
  - [`awacorn::variant`](#awacornvariant)
  - [`awacorn::unique_variant`](#awacornunique_variant)

---

## 概念

#️⃣ `Promise` 直译为 **承诺**，而 Awacorn 的 `Promise` 则有点类似于 Javascript `Promise`。

📕 在 C++ 制作好用的 `Promise` 这件事，Awacorn 并不是第一个，比如下面几个库就实现了可以**链式注册回调函数**的功能：

- 💠 [folly](https://github.com/facebook/folly) 在 `folly::makeFuture` 以后支持链式调用 `then`。
- 🍬 [promise-cpp](https://github.com/xhawk18/promise-cpp) 是一个符合 A+ 标准的 C++ Promise 实现。
- 👢 [boost-thread](https://github.com/boostorg/thread) `boost::future` 同样支持 `then`。

## `awacorn::promise`

💎 `awacorn::promise` 可以使你生成一个状态为 `pending` 的 `Promise`。关于如何生成 `rejected` 或者 `fulfilled` 的 `Promise`，请见 [`awacorn::resolve` / `awacorn::reject`](#awacornresolve--awacornreject)。

```cpp
#include "awacorn/promise.hpp"
int main() {
  awacorn::promise<int> pm;
  pm.resolve(10); // 解决 promise
}
```

- 📌 `Promise` 的生命周期是动态的，且以引用传递。
  - 🔰 只要 `Promise` 的拷贝还存在，它就不会被析构。

### `then` / `error` / `finally`

🔧 为一个 `Promise` 注册正常回调、错误回调和**最终**(_无论正常返回还是错误都会触发_)回调。

```cpp
#include "awacorn/promise.hpp"
int main() {
  awacorn::promise<int> pm;
  pm.then([](int i) {
    return i + 1; // 正常返回
  }).then([](int a) {
    return awacorn::reject<void>(awacorn::any()); // 拒绝
  }).error([](const std::exception_ptr& err) {
    return err; // 将拒绝变为解决
  }).then([](const std::exception_ptr&) {
    // error
  }).finally([]() {
    // ...
  });
  pm.resolve(10); // 解决 promise
}
```

- 无论是 `then`、`error` 还是 `finally` 方法都只接受一个回调函数。
  - 回调函数可以返回一个值或者 `Promise`，用于传递给下一个 `then`/`error` (见下)。
    - 返回 `Promise` 表示下一个回调将在**返回的 `Promise` 结束**(_解决或拒绝_)后被执行。
  - 当然，抛出错误可以用 `return awacorn::reject` 或者 `throw 错误`。当然返回也可以 `return awacorn::resolve` 或者 `return value` (使用 `resolve` / `reject` 的方式用于返回 `Promise` 时统一返回类型)。
- 这三个方法都返回一个新的 `Promise` 用于处理返回值。
  - 🔰 比如匿名函数返回 `int` 或者 `promise<int>`，方法就会返回 `promise<int>`。
  - ⚠️ 这三个方法返回的 `Promise` 和之前的 `Promise` 没有任何关系。比如，在注册 `error` 以后再往后注册回调，如果 `error` 前面的 `Promise` 没有被拒绝而是被解决了，那么 `error` 后注册的回调将永远不会被调用。
    - `then` 是一个例外。如果一个 `then` 前面的 `Promise` 被拒绝，则错误会根据调用链一直传递到任意 `error` 函数。
- ✅ 多次注册回调函数是没有问题的，并且调用回调函数时会严格按照注册顺序调用。
- 为节省性能 (防止内存泄漏)，在一个 `Promise` 被解决/拒绝且回调函数执行完毕后，将会析构已注册的全部回调函数。此后，任何注册回调函数都是立即执行的。

## `awacorn::resolve` / `awacorn::reject`

生成一个已经 `fulfilled` 或者 `rejected` 的 `Promise` 对象。

```cpp
#include "awacorn/promise.hpp"
int main() {
  awacorn::promise<int> pm = awacorn::resolve(1);
  pm.then([](int) {

  }); // 立即执行
}
```

- 💡 `resolve` 用于生成 `fulfilled` 的 `Promise`，而 `reject` 则用于生成 `rejected` 的 `Promise`。

## `awacorn::gather`

一个用于**并发 `Promise` 请求**的工具集合。这一部分由于几乎是 Javascript `Promise` 的移植，可以参考 [Mozilla Developer Network](https://developer.mozilla.org/) 的文档。

### `all`

🚚 等待传入的所有 `Promise` 完成。返回一个用于收集结果的 `Promise`。

```cpp
#include <iostream>
#include "awacorn/promise.hpp"
int main() {
  awacorn::promise<int> a;
  awacorn::promise<int> b;
  awacorn::gather::all(a, b).then([](const std::tuple<int, int>& res) {
    std::cout << std::get<0>(res) << std::get<1>(res) << std::endl;
  });
  a.resolve(1);
  b.resolve(2); // 这里执行回调
}
```

- ⚠️ 如果有一个 `Promise` 被拒绝，返回的 `Promise` 即被拒绝，拒绝错误为被拒绝 `Promise` 收到的错误。
- 我们使用 `std::tuple` 来收集结果。

### `any`

请参照 [Promise.any](https://developer.mozilla.org/docs/web/javascript/reference/global_objects/promise/any)。

- ⚠️ 在 C++ 11 下返回值是 `promise<awacorn::unique_variant<...>>`。
- ⚠️ 在全部 `Promise` 都被拒绝的情况下会直接用 `std::array<std::exception_ptr, COUNT>` 来拒绝 `Promise` (长度为总共的 `promise` 数量)，而不是使用 `AggregateError`。

### `race`

请参照 [Promise.race](https://developer.mozilla.org/docs/web/javascript/reference/global_objects/promise/race)。

- ⚠️ 在 C++ 11 下返回值是 `promise<awacorn::unique_variant<...>>`。

### `all_settled`

请参照 [Promise.allSettled](https://developer.mozilla.org/docs/web/javascript/reference/global_objects/promise/allSettled)。

- ⚠️ `all_settled` 使用 `std::tuple` 来承载返回值。
- 对于 `all_settled` 函数，无论传入的 `Promise` 是否错误都会正常返回。

## `awacorn::variant`

用于兼容 C++ 11 的 `variant` 实现。在 C++ 17 下是 `std::variant` 的别名。

实现了一小部分的 `std::variant` 接口，请尽情使用。

⚠️ 注意: 在 C++ 11 上请确保使用 **awacorn::variant** 而不是其它的 variant 实现，因为使用其它的 variant 容器可能导致跟 Awacorn 不兼容。如果实在是要使用指定实现，请**更改 Awacorn 源代码**。

## `awacorn::unique_variant`

用于去除 `variant` 类型中重复的模板参数类型来防止访问参数时谬构。

```cpp
using test = awacorn::unique_variant<int, int>; // awacorn::variant<int>
```
