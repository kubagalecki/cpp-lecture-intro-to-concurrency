#include <iostream>
#include <thread>

int main() {
  const auto id = [] {
    std::cout << std::this_thread::get_id()
              << '\n';
  };
  std::jthread t1{id};
  std::jthread t2{id};
}
