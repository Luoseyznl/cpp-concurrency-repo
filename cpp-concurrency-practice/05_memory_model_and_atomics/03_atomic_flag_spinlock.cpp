/**
 * @file 03_atomic_flag_spinlock.cpp
 * @brief 使用 atomic_flag 实现自旋锁
 */

#include <atomic>
#include <iostream>
#include <thread>
#include <vector>

// 初始化必须用 ATOMIC_FLAG_INIT (C++20之前)
std::atomic_flag lock_stream = ATOMIC_FLAG_INIT;

void f(int n) {
  // 将标志置为 true，返回旧值（true 表示“锁”已被占用，自旋等待）
  while (lock_stream.test_and_set(std::memory_order_acquire)) {
    std::this_thread::yield();  // 让出 CPU 时间片，避免空转
  }
  // ---------- 临界区开始 ----------

  std::cout << "[Thread " << n << "] is in critical section.\n";

  // ---------- 临界区结束 ----------
  lock_stream.clear(std::memory_order_release);  // 解锁，并确保写在解锁前完成
}

int main() {
  std::vector<std::thread> v;  // 批量线程管理
  for (int n = 0; n < 10; ++n) {
    v.emplace_back(f, n);
  }
  for (auto& t : v) {
    t.join();
  }
  return 0;
}
