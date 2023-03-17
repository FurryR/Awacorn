# Awacorn

Awacorn 是一个 C++ 11 的有栈协程/调度实现，总体偏向 Javascript。

```cpp
Awacorn::EventLoop* ev; // = ...
AsyncGenerator<void>([ev](AsyncGenerator<void>::Context* ctx) -> void {
  std::cout << "Hello World. Input your name" << std::endl;
  std::string name = ctx->await(ev->run(async_input("Your name: ")));
  std::cout << "Welcome, " << name << "!" << std::endl;
});
```

---

## 区别

Awacorn 和通常的 C++ 协程有什么区别？

表现为以下形式：

1. Awacorn 的协程不和调度器绑定。
   - 你可以将 `Promise` 和 `Generator` 单独拿出来使用，他们并不依赖 `Awacorn`。
2. Awacorn 是基于 `await`/`async` 的。
   - :x: 一般的协程库会 **劫持** 你的函数调用，这样你就感觉不到你在用协程。比如 **libgo** 或者 **libco**。缺点就是 **channel 泛滥**，非常难看。
   - :white_check_mark: Awacorn 基于 `await`/`async` 模式，且不劫持任何系统调用。你可以自己实现异步接口。
3. Awacorn 活用了 C++ 11 的元编程特性，以及智能指针。
   - 参见 `Promise` 及 `Generator` 的返回值及 `await` 处理。

## 文档

Working in progress.