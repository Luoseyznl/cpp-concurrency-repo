#include <chrono>
#include <condition_variable>
#include <iostream>
#include <mutex>
#include <thread>

std::mutex m;
std::condition_variable cv;
bool ready = false;  // 共享状态，用作为条件变量的谓词

void wait_cv() {
  std::unique_lock<std::mutex> lk(m);
  std::cout << "[T1] Waiting for signal...\n";
  cv.wait(lk, [] { return ready; });
  // ...
  std::cout << "[T1] Signal received.\n";
}

void signal_worker() {
  std::this_thread::sleep_for(std::chrono::milliseconds(500));
  {
    std::lock_guard<std::mutex> lk(m);
    ready = true;
    std::cout << "[T2] Signal sent (ready=true).\n";
  }
  cv.notify_all();  // 惊群（在锁外通知）
}

int main() {
  std::thread t1(wait_cv);
  std::thread t2(signal_worker);
  t1.join();
  t2.join();
  return 0;
}
