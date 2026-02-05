#include <chrono>
#include <iostream>
#include <mutex>
#include <thread>

std::mutex g_io_mutex;  // 全局互斥锁，保护 IO 操作

std::unique_lock<std::mutex> get_locked_io() {
  std::unique_lock<std::mutex> lk(g_io_mutex);
  std::cout << "[Factory] Acquired lock for IO\n";
  return lk;  // NRVO 返回锁对象
}

void flexible_task(int id) {
  // 1. 延迟锁定（等到真正需要时才锁定）
  std::unique_lock<std::mutex> lk(g_io_mutex, std::defer_lock);

  std::this_thread::sleep_for(std::chrono::milliseconds(10));

  // 2. try_lock: 尝试获取，失败不阻塞 (自旋或放弃)
  if (lk.try_lock()) {
    std::cout << "Thread " << id << " got lock via try_lock.\n";
    lk.unlock();  // 3. 提前解锁
    std::cout << "Thread " << id << " released lock early.\n";
  } else {
    std::cout << "Thread " << id << " failed to get lock, skipping work.\n";
  }

  // 4. 接收所有权（重新获取锁）
  if (!lk.owns_lock()) {  // 在上述 try_lock 失败后，重新获取锁
    std::unique_lock<std::mutex> lk2 = get_locked_io();
    std::cout << "Thread " << id << " owns lock via factory.\n";
  }
}

int main() {
  std::thread t1(flexible_task, 1);
  std::thread t2(flexible_task, 2);

  t1.join();
  t2.join();
  return 0;
}