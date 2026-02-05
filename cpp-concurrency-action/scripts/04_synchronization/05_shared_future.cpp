/**
 * @file 05_shared_future.cpp
 * @brief 普通的 future 只能被获取值一次，而 shared_future
 * 可以被多个线程共享获取值。 本质上各个线程获取到的是同一个值的副本，future
 * 状态只会变为 ready 一次。
 */

#include <future>
#include <iostream>
#include <thread>
#include <vector>

int main() {
  std::promise<int> p;

  std::shared_future<int> sf = p.get_future().share();

  auto wait_func = [](std::shared_future<int> f, int id) {
    std::cout << "[Thread " << id << "] Waiting for result...\n";
    int result = f.get();  // 多个线程可以调用 get()
    std::cout << "[Thread " << id << "] Result: " << result << "\n";
  };

  std::thread t1(wait_func, sf, 1);
  std::thread t2(wait_func, sf, 2);
  std::thread t3(wait_func, sf, 3);

  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  std::cout << "Main: Setting value to 888 (Broadcast)...\n";
  p.set_value(888);  // 一处设置，三处同时唤醒

  t1.join();
  t2.join();
  t3.join();
  return 0;
}
