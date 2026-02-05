#include <chrono>
#include <iostream>
#include <thread>

int main() {
  using namespace std::chrono_literals;  // 允许使用 ms, s, h 等后缀

  // 1. Duration (时间段)
  auto d1 = 500ms;  // std::chrono::milliseconds
  auto d2 = 2s;     // std::chrono::seconds
  std::chrono::duration<double, std::milli> d3 = d2;  // 大单位向小单位隐式转换

  // 2. Time Point (时间点)
  auto start = std::chrono::steady_clock::now();
  auto timeout_tp = start + 500ms;  // steady_clock 单调递增，适合计算超时

  std::cout << "Sleeping until target time point...\n";
  std::this_thread::sleep_until(timeout_tp);  // 这样不会假唤醒无限等待

  auto end = std::chrono::steady_clock::now();
  std::chrono::duration<double, std::milli> elapsed = end - start;
  std::cout << "Sleeped for " << elapsed.count() << " ms\n";

  return 0;
}
