#include <iostream>

#include "awacorn/event"
#include "awacorn/promise"
using namespace awacorn;
template <typename Rep, typename Period>
promise<std::nullptr_t> sleep(event_loop* ev,
                              const std::chrono::duration<Rep, Period>& dur) {
  promise<std::nullptr_t> pm;
  ev->event([pm]() { pm.resolve(nullptr); }, dur);
  return pm;
}
int main() {
  event_loop ev;
  gather::any(sleep(&ev, std::chrono::seconds(2)),
              sleep(&ev, std::chrono::seconds(1)))
      .then([]() { std::cout << "Hello World!" << std::endl; });
  ev.start();
}