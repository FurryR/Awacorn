#include <iostream>

#include "async.hpp"
#include "event.hpp"
#include "promise.hpp"
awacorn::promise<void> sleep(awacorn::event_loop* ev) {
  awacorn::promise<void> pm;
  ev->event([pm]() { pm.resolve(); }, std::chrono::nanoseconds(0));
  return pm;
}
awacorn::promise<void> test(awacorn::event_loop* ev) {
  return awacorn::async([ev](awacorn::context& ctx) { ctx >> sleep(ev); },
                        2 * 1024 * 1024);
}
int main() {
  awacorn::event_loop ev;
  for (std::size_t i = 0; i < 1000; i++) {
    test(&ev);
  }
  std::chrono::high_resolution_clock::time_point tm =
      std::chrono::high_resolution_clock::now();
  ev.start();
  std::cout << "1000 coroutines/events/promise objects done ("
            << std::chrono::duration_cast<
                   std::chrono::duration<long double, std::micro>>(
                   std::chrono::high_resolution_clock::now() - tm)
                   .count()
            << "us)" << std::endl;
}