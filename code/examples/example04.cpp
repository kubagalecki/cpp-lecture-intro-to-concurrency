#include <future>
#include <iostream>
#include <thread>
int some_complex_work() { return 42; }
void some_other_work() {}
void consume(int a) { std::cout << a << '\n'; }
int main() {
  std::future<int> f = std::async(some_complex_work);
  some_other_work();
  consume(f.get());
}
