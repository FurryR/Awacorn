#include <iostream>

#include "awacorn"
int main() {
  Awacorn::EventLoop ev;
  ev.create(
      [](Awacorn::EventLoop*, const Awacorn::Event*) {
        std::cout << "Hello World!" << std::endl;
      },
      std::chrono::seconds(2));
  ev.start();
}