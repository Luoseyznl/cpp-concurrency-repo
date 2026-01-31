#include <chrono>
#include <iostream>
#include <thread>
#include <vector>

void interruptible_task(std::stop_token stoken) {
  while (!stoken.stop_requested()) {
    std::cout << "Still working...\n";
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
  }
  std::cout << "Received stop signal!\n";
}

int main() {
#if __cplusplus < 202002L  // C++20 检测
  std::cout << "This example requires C++20 compiler support.\n";
  return 0;
#else
  std::cout << __cplusplus << std::endl;
  // 1. 自动汇合
  {
    std::jthread jt([] { std::cout << "Hello from jthread (auto-join)!\n"; });
    // 析构时自动 join
  }

  // 2. 协作式中断
  {
    std::cout << "Starting interruptible thread...\n";
    std::jthread worker(interruptible_task);

    std::this_thread::sleep_for(std::chrono::milliseconds(600));
    std::cout << "Main: Requesting stop...\n";
    // 析构时自动请求停止并 join
  }

  return 0;
#endif
}