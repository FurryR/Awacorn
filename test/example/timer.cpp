#include <iostream>

#include "event.hpp"
#include "promise.hpp"
using namespace awacorn;
template <typename Rep, typename Period>
promise<void> sleep(event_loop* ev,
                    const std::chrono::duration<Rep, Period>& dur) {
  promise<void> pm;
  ev->event([pm]() { pm.resolve(); }, dur);
  return pm;
}
int main() {
  event_loop ev;
  gather::any(sleep(&ev, std::chrono::seconds(2)),
              sleep(&ev, std::chrono::seconds(1)))
      .then(
          [](const awacorn::variant<awacorn::monostate, awacorn::monostate>&) {
            std::cout << "Hello World!" << std::endl;
          });
  ev.start();
}