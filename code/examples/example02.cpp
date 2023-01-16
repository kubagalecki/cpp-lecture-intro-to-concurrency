#include <iostream>
#include <thread>

int main() {
  int a = 0;
  const auto inc = [&](int i) {
    for(int i = 0; i < 1'000'000; ++i)
      ++a;
  };
  {
    std::jthread t1{inc, 1};
    std::jthread t2{inc, 2};
  }
  std::cout << a << '\n';
}
