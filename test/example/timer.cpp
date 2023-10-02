#include <iostream>

#include "event.hpp"
#include "promise.hpp"
#include "async.hpp"
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
  awacorn::async([&ev](awacorn::context& ctx) {
    ctx >> gather::any(sleep(&ev, std::chrono::seconds(2)),
                       sleep(&ev, std::chrono::seconds(1)));
    std::cout << "Hello World!" << std::endl;
  });
  ev.start();
}