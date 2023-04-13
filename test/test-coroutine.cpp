#include <cassert>

#include "awacorn/generator.hpp"
int main() {
  // test awacorn::generator<int, int>
  auto test_1 = awacorn::generator<int, int>(
      [](awacorn::generator<int, int>::context* ctx) {
        ctx->yield(1);
        return 1;
      });
  assert((test_1.next().type() == awacorn::result_t<int, int>::Yield));
  assert((test_1.next().type() == awacorn::result_t<int, int>::Return));
  assert((test_1.status() == awacorn::generator<int, int>::Returned));
  // test awacorn::generator<void, int>
  auto test_2 = awacorn::generator<void, int>(
      [](awacorn::generator<void, int>::context* ctx) { ctx->yield(1); });
  assert((test_2.next().type() == awacorn::result_t<void, int>::Yield));
  assert((test_2.next().type() == awacorn::result_t<void, int>::Return));
  assert((test_2.status() == awacorn::generator<void, int>::Returned));
}