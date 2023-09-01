#include <iostream>

#include "experimental/async.hpp"

using awacorn::async;
using awacorn::context;
using awacorn::promise;

template <typename Ctx>
typename context<Ctx>::template expr<promise<void>> print(
    const typename context<Ctx>::template expr<std::string>& v) {
  return typename context<Ctx>::template expr<promise<void>>(
      [v](context<Ctx>& ctx) {
        promise<promise<void>> x;
        v.apply(ctx).then([x](const std::string& v) {
          std::cout << v;
          x.resolve(awacorn::resolve());
        });
        return x;
      });
}
template <typename Ctx>
typename context<Ctx>::template expr<promise<std::string>> input() {
  return typename context<Ctx>::template expr<promise<std::string>>([](context<Ctx>&) {
    promise<promise<std::string>> x;
    std::string str;
    std::getline(std::cin, str);
    x.resolve(awacorn::resolve(std::move(str)));
    return x;
  });
}
struct A {
  int a;
  int b;
};
template <typename T>
using expr = context<void>::expr<T>;
int main() {
  async([](context<void>& v) {
    auto a = v.create("a", std::string());
    v << (print<void>("Hello World!\n"));
    v << (print<void>("Please input a string: "));
    v << (a = v.await(input<void>()));
    v << v.await(print<void>("Your input is: " + a + "\n"));
    v << v.cond(a == "awa",
                v.await(print<void>("OK\n")), v.await(print<void>("Fail\n")));
    v << v.ret();
  });
}