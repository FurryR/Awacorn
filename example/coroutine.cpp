#include <iostream>

#include "awacorn/event"
#include "awacorn/generator"

awacorn::promise<std::string> async_input(awacorn::event_loop* ev,
                                          const std::string& str) {
  awacorn::promise<std::string> pm;
  ev->create(
      [str, pm](const awacorn::event*) -> void {
        std::string input;
        std::cout << str << std::flush;
        std::getline(std::cin, input);
        pm.resolve(input);
      },
      std::chrono::nanoseconds(0));
  return pm;
}
int main() {
  awacorn::event_loop ev;
  awacorn::async_generator<void>(
      [&](awacorn::async_generator<void>::context* ctx) -> void {
        std::cout << "Hello World. Input your name" << std::endl;
        std::string name = ctx->await(async_input(&ev, "Your name: "));
        std::cout << "Welcome, " << name << "!" << std::endl;
      })
      .next();
  ev.start();
}