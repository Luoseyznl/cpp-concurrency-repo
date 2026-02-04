#include <functional>
#include <future>
#include <iostream>
#include <thread>

int main() {
  // 1. std::packaged_task（进阶版 std::function），既能洗状态，又能关联 future
  std::packaged_task<int(int, int)> task([](int a, int b) {
    std::cout << "[Task Thread] Computing " << a << " + " << b << "...\n";
    return a + b;
  });

  std::future<int> f = task.get_future();  // 只建立“结果通道”，需要手动执行任务

  // 2. 手动执行异步任务，模拟线程池入队
  std::thread worker_thread(
      [](std::packaged_task<int(int, int)> t) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        t(10, 20);  // 在工作线程中执行任务，Future （私有）状态变为 Ready
      },
      std::move(task));  // 移动语义传递任务

  std::cout << "[Main] Waiting for result...\n";
  std::cout << "[Main] Packaged Task Result: " << f.get() << "\n";

  worker_thread.join();  // 回收线程资源
  return 0;
}