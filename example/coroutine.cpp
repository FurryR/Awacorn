#include <iostream>

#include "awacorn"
#include "generator"

Awacorn::AsyncFn<Promise::Promise<std::string>> async_input(
    const std::string& str) {
  return [str](Awacorn::EventLoop* ev) {
    Promise::Promise<std::string> pm;
    ev->create(
        [str, pm](const Awacorn::EventLoop*, const Awacorn::Event*) -> void {
          std::string input;
          std::cout << str << std::flush;
          std::getline(std::cin, input);
          pm.resolve(input);
        },
        std::chrono::nanoseconds(0));
    return pm;
  };
}
int main() {
  Awacorn::EventLoop ev;
  Generator::AsyncGenerator<void>(
      [&](Generator::AsyncGenerator<void>::Context* ctx) -> void {
        std::cout << "Hello World. Input your name" << std::endl;
        std::string name = ctx->await(ev.run(async_input("Your name: ")));
        std::cout << "Welcome, " << name << "!" << std::endl;
      })
      .next();
  ev.start();
}