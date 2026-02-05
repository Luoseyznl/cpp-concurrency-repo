#include <atomic>
#include <cassert>
#include <iostream>
#include <string>
#include <thread>

std::string data;
std::atomic<bool> ready(false);  // 原子标志

void producer() {
  std::cout << "[Producer] Preparing data...\n";
  data = "Payload";
  // Release (Store): Ensure data is READY.
  ready.store(true, std::memory_order_release);
}

void consumer() {
  std::cout << "[Consumer] Waiting...\n";
  // Acquire (Load): Ensure data is SEEN.
  while (!ready.load(std::memory_order_acquire)) {
    std::this_thread::yield();  // 让出 CPU 时间片（回到就绪队列最后，避免空转）
  }
  assert(data == "Payload");
  std::cout << "[Consumer] Got data: " << data << "\n";
}

int main() {
  std::thread t1(producer);
  std::thread t2(consumer);

  t1.join();
  t2.join();
  return 0;
}
