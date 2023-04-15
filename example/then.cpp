#include <iostream>

#include "awacorn/promise.hpp"
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
  std::cout << "ptr.use_count() after resolve: " << ptr.use_count()
            << std::endl;
}