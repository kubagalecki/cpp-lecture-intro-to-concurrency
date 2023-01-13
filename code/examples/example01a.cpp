#include <iostream>
#include <thread>

int main() {
  const auto id = [](int i) {
    std::cout << i << '\n';
  };
  std::thread t1{id, 1};
  std::thread t2{id, 2};
  t1.join();
  t2.join();
}
