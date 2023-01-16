#include <iostream>
#include <thread>
int main() {
  int a = 0;
  std::mutex m;
  const auto inc = [&] {
    std::lock_guard<std::mutex> lock{m};
    ++a;
  };
  {
    std::jthread t1{inc};
    std::jthread t2{inc};
  }
  std::cout << a << '\n';
}
