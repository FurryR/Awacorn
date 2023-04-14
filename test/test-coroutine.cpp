#include <cassert>
#include <iostream>

#include "awacorn/generator.hpp"
int main() {
  // test awacorn::generator<int, int>
  {
    auto x = awacorn::generator<int, int>(
        [](awacorn::generator<int, int>::context* ctx) {
          try {
            ctx->yield(1);
          } catch (const awacorn::cancel_error&) {
            std::cout << "awacorn::generator<int, int> cancel test passed"
                      << std::endl;
            throw;
          }
          return 1;
        });
    assert((x.next().type() == awacorn::result_t<int, int>::Yield));
    assert((x.status() == awacorn::generator<int, int>::Yielded));
  }
  // test awacorn::generator<void, int>
  {
    auto x = awacorn::generator<void, int>(
        [](awacorn::generator<void, int>::context* ctx) {
          try {
            ctx->yield(1);
          } catch (const awacorn::cancel_error&) {
            std::cout << "awacorn::generator<void, int> cancel test passed"
                      << std::endl;
            throw;
          }
        });
    assert((x.next().type() == awacorn::result_t<void, int>::Yield));
    assert((x.status() == awacorn::generator<void, int>::Yielded));
  }
  // test awacorn::async_generator<int, int>
  {
    auto x = awacorn::async_generator<int, int>(
        [](awacorn::async_generator<int, int>::context* ctx) {
          ctx->yield(1);
          ctx->await(awacorn::resolve());
          try {
            ctx->await(awacorn::promise<int>());  // never resolves
          } catch (const awacorn::cancel_error&) {
            std::cout << "awacorn::async_generator<int, int> cancel test passed"
                      << std::endl;
            throw;
          };
          return 1;
        });
    x.next().then([&](const awacorn::result_t<int, int>& res) {
      assert((res.type() == awacorn::result_t<int, int>::Yield));
      assert((x.status() == awacorn::async_generator<int, int>::Yielded));
    });
    x.next();
  }
  // test awacorn::async_generator<void, int>
  {
    auto x = awacorn::async_generator<void, int>(
        [](awacorn::async_generator<void, int>::context* ctx) {
          ctx->yield(1);
          ctx->await(awacorn::resolve());
          try {
            ctx->await(awacorn::promise<int>());  // never resolves
          } catch (const awacorn::cancel_error&) {
            std::cout << "awacorn::async_generator<void, int> cancel test passed"
                      << std::endl;
            throw;
          };
        });
    x.next().then([&](const awacorn::result_t<void, int>& res) {
      assert((res.type() == awacorn::result_t<void, int>::Yield));
      assert((x.status() == awacorn::async_generator<void, int>::Yielded));
    });
    x.next();
  }
  // test awacorn::async_generator<int, void>
  {
    auto x = awacorn::async_generator<int, void>(
        [](awacorn::async_generator<int, void>::context* ctx) {
          ctx->await(awacorn::resolve());
          ctx->await(awacorn::resolve(1));
          return 1;
        });
    assert((x.status() == awacorn::async_generator<int, void>::Pending));
    x.next().then([&](int) {
      std::cout << "awacorn::async_generator<int, void> resolve test passed"
                << std::endl;
    });
  }
  // test awacorn::async_generator<void, void>
  {
    auto x = awacorn::async_generator<void, void>(
        [](awacorn::async_generator<void, void>::context* ctx) {
          ctx->await(awacorn::resolve());
          // ctx->await(awacorn::resolve(1));
        });
    assert((x.status() == awacorn::async_generator<void, void>::Pending));
    x.next().then([&]() {
      std::cout << "awacorn::async_generator<void, void> resolve test passed"
                << std::endl;
    });
  }
}