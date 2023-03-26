# <center>Awacorn</center>

[![CodeFactor](https://www.codefactor.io/repository/github/furryr/awacorn/badge)](https://www.codefactor.io/repository/github/furryr/awacorn)

<center>Awacorn 是一个 C++ 11 的有栈协程/调度实现，总体偏向 Javascript。</center>

```cpp
int main() {
  Awacorn::EventLoop ev;
  Generator::AsyncGenerator<void>(
      [&](Generator::AsyncGenerator<void>::Context* ctx) -> void {
        std::cout << "Hello World. Input your name" << std::endl;
        std::string name = ctx->await(ev.run(async_input("Your name: ")));
        std::cout << "Welcome, " << name << "!" << std::endl;
      })
      .next();
  ev.start();
}
```

:eyes: 查看 [完整示例](./example/coroutine.cpp)

## 目录

- [Awacorn](#awacorn)
  - [目录](#目录)
  - [区别](#区别)
  - [文档](#文档)
    - [什么是 Awacorn?](#什么是-awacorn)
    - [使用](#使用)
    - [Awacorn::EventLoop](#awacorneventloop)
      - [create](#create)

## 区别

Awacorn 和通常的 C++ 协程有什么区别？

表现为以下形式：

1. [x] Awacorn 的协程不和调度器绑定。
   - :v: 你可以将 `Promise` 和 `Generator` 单独拿出来使用，他们并不依赖 `Awacorn`。
2. [x] Awacorn 是基于 `await`/`async` 的。
   - :x: 一般的协程库会 **劫持** 你的函数调用，这样你就感觉不到你在用协程。比如 **libgo** 或者 **libco**。缺点就是 **channel 泛滥**，非常难看。
   - :white_check_mark: Awacorn 基于 `await`/`async` 模式，且不劫持任何系统调用。你可以自己实现异步接口。
3. [x] Awacorn 活用了 C++ 11 的元编程特性，以及智能指针。
   - :mag: 参见 `Promise` 及 `Generator` 的返回值及 `await` 处理。

## 文档

此篇文档由 **ccw.site** 版裁剪修改而成。

### 什么是 Awacorn?

Awacorn 是一个异步编程库，包含以下内容:

- [x] 事件循环
- [x] 类 Javascript `Promise`
- [x] 有栈协程

依赖关系如下：
| 组件 | 依赖 |
| ---- | ---- |
| Awacorn | (None) |
| Promise | (None) |
| Generator | Promise|

### 使用

Awacorn 可以像其它 cmake 库那样被用于你的项目。在部分平台上，你可能需要 `ucontext` 支持。

你可以使用 `-DAWACORN_BUILD_EXAMPLE=ON` 来同时编译示例。

### Awacorn::EventLoop

`Awacorn::EventLoop` 是事件循环的定义，位于头文件 `awacorn` 中。

下面是一个使用它的示例:

```cpp
#include "awacorn"
// 在 main 函数内
const Awacorn::Task* task = ev.create([](Awacorn::EventLoop* ev, const Awacorn::Event* task) -> void {
  cout << "我不会被执行到" << endl;
}, chrono::seconds(1));
ev.clear(task);

```

#### create

`create` 函数用于创建一个任务。

下面是一个使用它的示例:

```cpp
#include "awacorn"
// 在 main 函数内
ev.create([](Awacorn::EventLoop* ev, const Awacorn::Event* task) -> void {
    cout << "我被执行了！" << endl;
}, chrono::seconds(2));
```

其中:

- 临时函数的第一个参数类型一定为 `Awacorn::EventLoop*`。
- 如果需要设定事件，则临时函数的第二个参数类型应为 `const Awacorn::Event*`。
  - 反之，如果需要设定循环事件，则临时函数的第二个参数类型应为 `const Awacorn::Interval*`。
- 当然，`create` 的返回值就是临时函数第二个参数的类型，**可以用于取消事件**。
  - 请参照 `clear` 函数。

---

Working in progress.
