# promise

:dizzy: Awacorn 异步编程的第一步，堂堂登场！

## 目录

- [promise](#promise)
  - [目录](#目录)
  - [概念](#概念)
  - [`awacorn::promise`](#awacornpromise)

---

## 概念

`Promise` 直译为 **承诺**，而 Awacorn 的 `Promise` 则有点类似于 Javascript `Promise`。

在 C++ 制作好用的 `Promise` 这件事，Awacorn 并不是第一个，比如下面几个库就实现了可以**链式注册回调函数** 的功能：

- :diamond_shape_with_a_dot_inside: [folly](https://github.com/facebook/folly) 在 `folly::makeFuture` 以后支持链式调用 `then`。
- :candy: [promise-cpp](https://github.com/xhawk18/promise-cpp) 是一个符合 A+ 标准的 C++ Promise 实现。
- :boot: [boost-thread](https://github.com/boostorg/thread) `boost::future` 同样支持 `then`。

那么 Awacorn 为什么要重新造一遍轮子呢？以下是这三个库的不同点对比。

| Awacorn                                 | folly                               | promise-cpp                           | boost-thread                                 |
| --------------------------------------- | ----------------------------------- | ------------------------------------- | -------------------------------------------- |
| :x: 不适用                              | :x: 不适用                          | :white_check_mark: 符合 A+ 标准(~~伪~~) | :x: 不适用                                   |
| :white_check_mark: 编译时强类型支持     | :white_check_mark: 编译时强类型支持 | :x: 弱类型(伪强类型)支持              | :white_check_mark: 编译时强类型支持          |
| :x: 弱类型异常处理                      | :white_check_mark: 强类型异常处理   | :x: 弱类型异常处理                    | :x: 基于 `std::exception_ptr` 的基本异常处理 |
| :white_check_mark: 支持多次设定回调函数 | :x: 不支持多次设定回调函数          | :x: 不支持多次设定回调函数            | :x: 不支持多次设定回调函数                   |
| :x: 线程不安全                          | :white_check_mark: 线程安全         | :x: 线程不安全                        | :white_check_mark: 线程安全                  |
| :white_check_mark: 不绑定事件循环       | :white_check_mark: 不绑定事件循环   | :white_check_mark: 不绑定事件循环     | :white_check_mark: 不绑定事件循环            |
| :white_check_mark: 完全并发组件支持     | :x: 部分并发组件支持                | :x: 部分并发组件支持                  | :x: 部分并发组件支持                         |
| :white_check_mark: 回调函数生命周期良构 | :x: 未定义                          | :x: 未定义                            | :x: 未定义                                   |
| :white_check_mark: 可中断               | :white_check_mark: 可中断           | :x: 不可中断                          | :white_check_mark: 可中断                    |
| :white_check_mark: Header-only          | :x: 不适用                          | :white_check_mark: Header-only        | :x: 不适用                                   |

综合来看，Awacorn `Promise` 最适用于当前场景，并且十分容易上手，故 Awacorn 采用 `Promise`。

## `awacorn::promise`

`awacorn::promise` 可以使你生成一个状态为 `Pending` 的 `Promise`。

---

Working in progress.