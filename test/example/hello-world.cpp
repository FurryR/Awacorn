#include <iostream>

#include "experimental/async.hpp"

using awacorn::async;
using awacorn::context;
using awacorn::promise;
using awacorn::stmt::value;
using awacorn::stmt::var;

value<promise<void>> print(const value<std::string>& v) {
  return value<promise<void>>([v]() {
    promise<promise<void>> x;
    v.apply().then([v]() { return v.get(); }).then([x](const std::string& v) {
      std::cout << v;
      x.resolve(awacorn::resolve());
    });
    return x;
  });
}
value<promise<std::string>> input() {
  return value<promise<std::string>>([]() {
    promise<promise<std::string>> x;
    std::string str;
    std::getline(std::cin, str);
    x.resolve(awacorn::resolve(std::move(str)));
    return x;
  });
}
int main() {
  async<void>([](context<void>& v) {
    var<std::string> a = v.create("a", std::string());
    v << v.await(print("Hello World!\n"));
    v << v.await(print("Please input a string: "));
    v << (a = v.await(input()));
    v << v.await(print("Your input is: " + a + "\n"));
    v << v.cond(a == "awa",
                [](context<void>& v) { v << v.await(print("OK\n")); });
    v << v.ret();
  });
}