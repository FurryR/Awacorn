# <p align="center"> awacorn </p>

[![CodeFactor](https://www.codefactor.io/repository/github/furryr/awacorn/badge)](https://www.codefactor.io/repository/github/furryr/awacorn)

<p align="center">Awacorn 是一个 C++ 11 的有栈协程/调度实现，总体偏向 Javascript。</p>

```cpp
int main() {
  awacorn::event_loop ev;
  awacorn::async(
      [&](awacorn::context& ctx) {
        std::cout << "Hello World. Input your name" << std::endl;
        std::string name = ctx >> async_input(&ev, "Your name: ");
        std::cout << "Welcome, " << name << "!" << std::endl;
      });
  ev.start();
}
```

👀 查看 [完整示例](./test/example/hello-world.cpp)

## 目录

- [ awacorn ](#-awacorn-)
  - [目录](#目录)
  - [什么是 awacorn](#什么是-awacorn)
  - [区别](#区别)
  - [编译](#编译)
  - [文档](#文档)

---

## 什么是 awacorn

Awacorn 是一个异步编程库，包含以下内容:

- [x] 事件循环
- [x] 类 Javascript `Promise`
- [x] 有栈协程

📝 更详细的信息请跳转至 [文档](#文档)。

## 区别

Awacorn 和通常的 C++ 协程有什么区别？

表现为以下形式：

1. [x] Awacorn 的协程不和调度器绑定。
   - ✌️ 你可以将 `promise` 和 `async` 单独拿出来使用，他们并不依赖 `event`。
2. [x] Awacorn 是基于 `await`/`async` 的。
   - ❌ 一般的协程库会 **劫持** 你的函数调用，这样你就感觉不到你在用协程。比如 **libgo** 或者 **libco**。缺点就是 **channel 泛滥**，非常难看。
   - ✅ Awacorn 基于 `await`/`async` 模式，且不劫持任何系统调用。你可以自己实现异步接口。
3. [x] Awacorn 活用了 C++ 11 的元编程特性，以及智能指针。
   - 🔍 参见 `promise` 及 `async` 的返回值及 `await` 处理。

## 编译

以下是 Awacorn 支持的编译选项。

| 选项                    | 描述                                                  | 要求                       |
| ----------------------- | ----------------------------------------------------- | -------------------------- |
| -DAWACORN_BUILD_EXAMPLE | 💚 构建所有示例程序和测试，这将导致额外的编译时间。   | N/A                        |
| -DAWACORN_USE_BOOST     | 🚧 使用 `boost::context::continuation` 作为协程实现。 | `boost_context`            |
| -DAWACORN_USE_UCONTEXT  | 🚧 使用 `ucontext_t` 作为协程实现。                   | `ucontext.h` (libucontext) |

💡 提示: 当 `-DAWACORN_USE_BOOST` 和 `-DAWACORN_USE_UCONTEXT` 均未被指定时，awacorn 将自动指定最优实现。

⚠️ 警告: 当 `boost` 和 `ucontext` 均无法使用时，编译将失败。请安装 `libucontext` 或 `libboost`。

- [boost::context](https://github.com/boostorg/context):
  - 🐧 Debian: `apt install libboost-context-dev`
  - 📱 Termux: `apt install boost-headers`
- [libucontext](https://github.com/kaniini/libucontext):
  - ✅ 几乎所有 `i386/x86_64` Linux 发行版都包含 `ucontext` 支持。
  - 📱 Termux: `apt install libucontext`

## 文档

以下是 Awacorn 的组件大览。

| 组件名               | 描述                                            | 依赖                                | 文档                                          |
| -------------------- | ----------------------------------------------- | ----------------------------------- | --------------------------------------------- |
| `event`              | Awacorn 的事件循环，负责调度定时事件。          | void                                | 🐯<br>[event](doc/event.md)                   |
| `promise`            | 类似于 Javascript 的 Promise，低成本 & 强类型。 | void                                | 🐺<br>[promise](doc/promise.md)               |
| `async`              | `async/await` 有栈协程。                        | (`boost` \| `ucontext`) & `promise` | 🐱<br>[async](doc/async.md)                   |
| `function`           | Awacorn 采用的内部 `std::function` 实现。       | void                                | 🐻<br>[function](doc/function.md)             |
| `capture`            | Awacorn 采用的内部万能捕获实现。                | void                                | 🐂<br>[capture](doc/capture.md)               |
| `experimental/async` | Awacorn 最新的无栈协程。                        | void                                | 🐱<br>[experimental/async](doc/async-next.md) |

🔰 点击 **文档** 即可查看组件相关的 **详细文档**。

---

_`<p align="center">` Awacorn version 3.1.0. `</p>`_
_`<p align="center">` This program was under the [MIT](./LICENSE) license. `</p>`_
