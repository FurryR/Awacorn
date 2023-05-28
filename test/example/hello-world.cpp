#include <iostream>

#include "awacorn/async.hpp"
#include "awacorn/event.hpp"

awacorn::promise<std::string> async_input(awacorn::event_loop* ev,
                                          const std::string& str) {
  awacorn::promise<std::string> pm;
  ev->event(
      [str, pm]() -> void {
        std::string input;
        std::cout << str << std::flush;
        std::getline(std::cin, input);
        pm.resolve(std::move(input));
      },
      std::chrono::nanoseconds(0));
  return pm;
}
int main() {
  awacorn::event_loop ev;
  awacorn::async([&](awacorn::context& ctx) {
    std::cout << "Hello World. Input your name" << std::endl;
    std::string name = ctx >> async_input(&ev, "Your name: ");
    std::cout << "Welcome, " << name << "!" << std::endl;
  });
  ev.start();
}