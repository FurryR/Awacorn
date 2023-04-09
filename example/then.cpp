#include <iostream>

#include "awacorn/promise"
int main() {
  awacorn::resolve(1)
      .then([](int i) {
        std::cout << "Recvived " << i << std::endl;
        return awacorn::resolve(2);
      })
      .then([](int i) { std::cout << "Recvived " << i << std::endl; });
}