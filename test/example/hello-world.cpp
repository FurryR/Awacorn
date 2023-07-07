#include <iostream>

#include "event.hpp"
#include "experimental/async.hpp"

awacorn::stmt::value<awacorn::promise<void>> print(
    const awacorn::stmt::value<std::string>& v) {
  return awacorn::stmt::value<awacorn::promise<void>>(
      [v]() -> awacorn::promise<awacorn::promise<void>> {
        awacorn::promise<awacorn::promise<void>> x;
        v.apply()
            .then([v]() { return v.get(); })
            .then([x](const std::string& v) {
              std::cout << v << std::endl;
              x.resolve(awacorn::resolve());
            });
        return x;
      });
}
int main() {
  awacorn::async<void>([](awacorn::context<void>& v) {
    v << v.ret(v.await(print("Hello World")));
  });
}