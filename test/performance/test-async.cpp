#include <iostream>

#include "event.hpp"
#include "experimental/async.hpp"
#include "promise.hpp"
template <typename Ctx>
typename awacorn::asyncfn<Ctx>::template expr<awacorn::promise<void>> sleep(
    awacorn::event_loop* ev) {
  return typename awacorn::asyncfn<Ctx>::template expr<awacorn::promise<void>>(
      [ev](awacorn::context<Ctx>&) {
        awacorn::promise<awacorn::promise<void>> x;
        ev->event([x]() { x.resolve(awacorn::resolve()); },
                  std::chrono::nanoseconds(0));
        return x;
      });
}
awacorn::promise<void> test(awacorn::event_loop* ev) {
  return awacorn::async(
      [ev](awacorn::asyncfn<void>& ctx) { ctx << ctx.await(sleep<void>(ev)); });
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