#include <iostream>

#include "promise.hpp"
int main() {
  awacorn::promise<void> pm;  // 生成一个临时的 promise 对象
  std::shared_ptr<int> ptr =
      std::make_shared<int>(1);  // 探究 promise 对引用计数的影响

  pm.then([ptr]() {
    std::cout << "ptr.use_count() in then #1: " << ptr.use_count() << std::endl;
    std::cout << "Done #1" << std::endl;
  });
  pm.then([ptr]() {
    std::cout << "ptr.use_count() in then #2: " << ptr.use_count() << std::endl;
    std::cout << "Done #2" << std::endl;
  });
  std::cout << "ptr.use_count() before resolve: " << ptr.use_count()
            << std::endl;
  pm.resolve();
  awacorn::gather::all(pm).then([](const std::tuple<awacorn::monostate>&) {
    std::cout << "all" << std::endl;
  });
  awacorn::gather::any(pm).then(
      [](const awacorn::variant<awacorn::monostate>&) {
        std::cout << "any" << std::endl;
      });
  awacorn::gather::race(pm).then(
      [](const awacorn::variant<awacorn::monostate>&) {
        std::cout << "race" << std::endl;
      });
  awacorn::gather::all_settled(pm).then(
      [](const std::tuple<awacorn::promise<void>>&) {
        std::cout << "all_settled" << std::endl;
      });
  std::cout << "ptr.use_count() after resolve: " << ptr.use_count()
            << std::endl;
}