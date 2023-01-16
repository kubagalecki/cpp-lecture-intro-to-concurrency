#include <condition_variable>
#include <iostream>
#include <thread>
int some_complex_work() { return 42; }
void some_other_work() {}
void consume(int a) { std::cout << a << '\n'; }
int main() {
  int a{};
  std::mutex m;
  std::condition_variable cv;
  bool work_done = false;
  std::jthread t1{[&]() {
    int a_local = some_complex_work();
    {
      std::lock_guard<std::mutex> lock{m};
      a = a_local;
      work_done = true;
    }
    cv.notify_one();
  }};
  std::jthread t2{[&]() {
    some_other_work();
    std::unique_lock<std::mutex> lock{m};
    cv.wait(lock, [&] { return work_done; });
    consume(a);
  }};
}
