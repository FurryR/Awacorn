# event

:watch: `event` 是 Awacorn 的事件调度组件，负责实现定时。

## 目录

- [event](#event)
  - [目录](#目录)
  - [`awacorn::event_loop`](#awacornevent_loop)
    - [`event` / `interval`](#event--interval)
    - [`clear`](#clear)
    - [`set_yield`](#set_yield)
    - [`current`](#current)
    - [`start`](#start)
  - [`awacorn::task_t`](#awacorntask_t)

---

## `awacorn::event_loop`

`awacorn::event_loop` 是事件循环的定义，位于头文件 `awacorn/event` 中。

```cpp
#include <iostream>
#include "awacorn/event.hpp"
int main() {
  using namespace awacorn;
  using namespace std;
  event_loop ev;
  task_t task = ev.timeout([]() {
    // dead code
    cout << "不会被执行到" << endl;
  }, chrono::seconds(1));
  ev.clear(task); // 清除任务
  ev.start();
}
```

:triangular_flag_on_post: 不妨自己试试上方的示例!

:memo: 取自 [`clear`](#clear) 的示例。

### `event` / `interval`

:star: `event` `interval` 函数用于创建一个任务。

```cpp
#include <iostream>
#include "awacorn/event.hpp"
int main() {
  using namespace awacorn;
  using namespace std;
  event_loop ev;
  ev.event([]() -> void {
      cout << "我被执行了！" << endl;
  }, chrono::seconds(2));
  ev.interval([]() -> void {
      cout << "我每 2 秒执行一次。" << endl;
  }, chrono::seconds(2));
  ev.start();
}
```

- :arrow_right: 如果希望注册一次性事件，请使用 `event`。
  - :arrows_counterclockwise: 反之，如果需要设定循环事件，则使用`interval`。
- 当然，`event` `interval` 的返回值就是**事件标识**，**可以用于取消事件**。
  - :eyes: 请参照 [`clear`](#clear) 函数。

### `clear`

:fire: `clear` 函数可以清除一个任务。

```cpp
#include <iostream>
#include "awacorn/event.hpp"
int main() {
  using namespace awacorn;
  using namespace std;
  event_loop ev;
  task_t task = ev.event([]() -> void {
    // dead code
    cout << "不会被执行到" << endl;
  }, chrono::seconds(1));
  ev.clear(task); // 清除任务
  ev.start();
}
```

- :fire: `clear` 的第一个参数是 `task`，也即 `event` `interval` 函数的返回值。只需要传进去即可清除事件。
- :recycle: 一个事件就算清除它自身 (`ev.clear(ev.current())`) 也不会导致未定义行为。
  - 相反，如果循环事件要清除自身，`ev.clear(ev.current())` 还是最佳实践。

### `set_yield`

:sleeping: 设置 **yield 实现**。

在加入 `set_yield` 函数以前，兼容 Awacorn 异步 I/O 实现通常是利用一个定时循环事件 (比如 7ms) 中进行一段较短时间的 `poll / epoll` 调用 (比如 7ms)。这样将阻塞整个循环并且拖慢运行速度，同时也不够精确。

在加入 `set_yield` 函数以后，I/O 实现可以通过替换 **yield 实现** 来实现事件循环空闲时进行 I/O 操作，使其变得更加精确且不影响事件循环运行。

**yield 实现** 将接受一个 `const std::chrono::steady_clock::duration&` 作为 **参考** 的睡眠时间，这意味着函数并不需要睡眠那么久，或者可以睡眠更久(这可能会影响精度)。当 **yield 实现** 提前返回(比如读取到 I/O 事件)，则会立刻尝试触发事件。

实际的经过时间并不完全取决于 **参考** 时间，而是取决于 **yield 实现** 函数的运行时间(**实际** 时间)，这样做可以提高事件循环精度。

### `current`

:rainbow: 获取当前正在执行的任务。返回 `awacorn::task_t`，**可用于取消事件**。

```cpp
#include <iostream>
#include "awacorn/event.hpp"
int main() {
  using namespace awacorn;
  using namespace std;
  event_loop ev;
  ev.interval([&]() -> void {
    cout << "实际上只执行一次" << endl;
    ev.clear(ev.current()); // 删除自己。
  }, chrono::seconds(1));
  ev.start();
}
```

### `start`

:hearts: 启动事件循环，然后享受 awacorn。

:warning: 警告: 这是一个阻塞函数，会阻塞到事件循环结束为止。

## `awacorn::task_t`

:dart: 用于标识任务。

- `awacorn::event_loop::current` `awacorn::event_loop::event` `awacorn::event_loop::interval` 都返回 `awacorn::task_t`，用于清除事件。
- `awacorn::event_loop::clear` 可以通过传入这个类型来清除事件。
