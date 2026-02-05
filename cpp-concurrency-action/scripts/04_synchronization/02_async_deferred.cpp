#include <future>
#include <iostream>
#include <thread>

int main() {
  // 1. std::launch::async 立即开启新线程执行任务
  std::future<int> f1 = std::async(std::launch::async, [] {
    std::cout << "[T1] Async task executing on thread "
              << std::this_thread::get_id() << "\n";
    return 42;
  });  // 返回 future 对象（用于获取结果）

  // 2. std::launch::deferred 惰性求值，直到调用 get()/wait()
  auto f2 = std::async(std::launch::deferred, [] {
    std::cout << "[T2] Deferred task executing on thread "
              << std::this_thread::get_id() << "\n";
    return 100;
  });

  std::cout << "[Main] Waiting for result..., while [T1] is working.\n";
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  std::cout << "Calling f2.get()...\n";
  int res2 = f2.get();

  std::cout << "[Main] f1 gets " << f1.get() << ", f2 gets " << res2 << "\n";
  return 0;
}
